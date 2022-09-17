#include <condition_variable>
#include <curl/curl.h>
#include <mutex>
#include <regex>
#include <stdio.h>
#include <string>
#include <vector>

#include "curlfuncs.h"
#include "dav.h"
#include "fs.h"
#include "util.h"

#define userAgent "JKSV"
#define DRIVE_UPLOAD_BUFFER_SIZE 0x8000
#define DRIVE_DOWNLOAD_BUFFER_SIZE 0xC00000

bool drive::dav::SetPath(std::string url, std::string user, std::string pass) {
  this->davUrl = url;
  this->davUser = user;
  this->davPass = pass;

  return false;
}

static inline void writeCurlError(const std::string &_function, int _cerror) {
  fs::logWrite("[DAVDrive](%s): CURL returned error %i\n", _function.c_str(),
               _cerror);
}

static inline void writeXmlError(const std::string &_function,
                                 const std::string &msg) {
  fs::logWrite("[DAVDrive](%s): XML fault %s", _function.c_str(), msg.c_str());
}

/* file method,
 * special thanks for:
 *  https://code.blogs.iiidefix.net/posts/webdav-with-curl/
 *  https://stackoverflow.com/questions/26429904/list-files-folders-in-owncloud-with-php-curl
 */

bool drive::dav::dirExists(const std::string &_dirName) {
  // todo: check folder exists
  return false;
}

bool drive::dav::createDir(const std::string &_dirName) {
  CURL *curl = curl_easy_init();
  std::string davPath(this->davUrl);
  char *_dirNameCode =
      curl_easy_escape(curl, _dirName.c_str(), _dirName.length());
  davPath.append("/").append(_dirNameCode);
  curl_free(_dirNameCode);

  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "MKCOL");
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
  curl_easy_setopt(curl, CURLOPT_URL, davPath.c_str());
  curl_easy_setopt(curl, CURLOPT_USERNAME, this->davUser.c_str());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, this->davPass.c_str());
  fs::logWrite("[DAVDrive](%s): CURL Mkdir %s, user %s, pass %s\n", __func__,
               davPath.c_str(), this->davUser.c_str(), this->davPass.c_str());

  int error = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (error == CURLE_OK)
    return true;
  else
    writeCurlError(__func__, error);

  return false;
}

bool drive::dav::listDirReq(const std::string &_dirName, std::string *xmlResp) {
  CURL *curl = curl_easy_init();
  std::string davPath(this->davUrl);
  char *_dirNameCode =
      curl_easy_escape(curl, _dirName.c_str(), _dirName.length());
  davPath.append("/").append(_dirNameCode);
  curl_free(_dirNameCode);

  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
  curl_easy_setopt(curl, CURLOPT_URL, davPath.c_str());
  curl_easy_setopt(curl, CURLOPT_USERNAME, this->davUser.c_str());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, this->davPass.c_str());

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, xmlResp);

  fs::logWrite("[DAVDrive](%s): CURL Mkdir %s, user %s, pass %s\n", __func__,
               davPath.c_str(), this->davUser.c_str(), this->davPass.c_str());

  int error = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (error == CURLE_OK) {
    return true;
  } else
    writeCurlError(__func__, error);

  return false;
}

bool drive::dav::xmlGetItemByPath(std::string &xml, xmlChar *xpath,
                                  std::vector<std::string> &_out) {
  xmlDocPtr doc = xmlParseMemory(xml.c_str(), xml.size());
  xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
  if (!ctxt) {
    xmlFreeDoc(doc);
    writeXmlError(__func__, "xmlXPathNewContext error");

    return false;
  }
  xmlXPathRegisterNs(ctxt, BAD_CAST "D", BAD_CAST "DAV:");
  xmlXPathObjectPtr res = xmlXPathEvalExpression(xpath, ctxt);
  if (!res) {
    xmlXPathFreeContext(ctxt);
    xmlFreeDoc(doc);
    writeXmlError(__func__, "xmlXPathNewContext error");

    return false;
  }

  xmlNodeSetPtr nodeset = res->nodesetval;
  if (!nodeset) {
    xmlXPathFreeContext(ctxt);
    xmlFreeDoc(doc);
    writeXmlError(__func__, "xmlXPathNewContext error");

    return false;
  }

  for (int i = 0; i < nodeset->nodeNr; i++) {
    xmlNodePtr node = nodeset->nodeTab[i];
    xmlChar *data = xmlNodeGetContent(node);
    _out.push_back(std::string((char *)data));
  }

  xmlXPathFreeObject(res);
  xmlXPathFreeContext(ctxt);
  xmlFreeDoc(doc);
  return true;
}

void drive::dav::listDir(const std::string &_parent,
                         std::vector<drive::davItem> &_out) {
  _out.clear();
  std::string *xmlResp = new std::string;

  if (!this->listDirReq(_parent, xmlResp)) {
    writeXmlError(__func__, "listDirReq error");
    return;
  }

  xmlChar *xpHref = BAD_CAST
      "//D:response[*][D:propstat[1]/D:prop[1]/D:getcontenttype[1]]/D:href";
  xmlChar *xpName =
      BAD_CAST "//D:response[*]/D:propstat[1]/D:prop[1][D:getcontenttype[1]]/"
               "D:displayname";
  xmlChar *xpSize = BAD_CAST "//D:response[*]/D:propstat[1]/"
                             "D:prop[D:getcontenttype[1]]/D:getcontentlength";

  std::vector<std::string> vHref;
  std::vector<std::string> vName;
  std::vector<std::string> vSize;

  xmlGetItemByPath(*xmlResp, xpHref, vHref);
  xmlGetItemByPath(*xmlResp, xpName, vName);
  xmlGetItemByPath(*xmlResp, xpSize, vSize);

  if (vHref.size() != vName.size() || vHref.size() != vSize.size()) {
    writeXmlError(__func__, "vHref, vName, vSize size mismatch");
    return;
  }

  for (size_t i = 0; i < vHref.size(); i++) {
    davItem item;
    item.did = DriveID;
    item.name = vName[i];
    item.path = vHref[i];
    item.size = std::stoi(vSize[i]);
    _out.push_back(item);
  }
}

bool drive::dav::fileExists(const std::string &_filename) {
  // todo: check if file exists
  return false;
}

void drive::dav::uploadFile(const std::string &_filename,
                            const std::string &_title,
                            curlFuncs::curlUpArgs *_upload) {
  CURL *curl = curl_easy_init();
  std::string davPath(this->davUrl);
  char *_titleCode = curl_easy_escape(curl, _title.c_str(), _title.length());
  char *_filenameCode =
      curl_easy_escape(curl, _filename.c_str(), _filename.length());
  davPath.append("/").append(_titleCode).append("/").append(_filenameCode);
  curl_free(_titleCode);
  curl_free(_filenameCode);

  fs::logWrite("[DAVDrive](%s): CURL upload: %s\n", __func__, davPath.c_str());

  curl_easy_setopt(curl, CURLOPT_PUT, 1);
  curl_easy_setopt(curl, CURLOPT_URL, davPath.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
  curl_easy_setopt(curl, CURLOPT_USERNAME, this->davUser.c_str());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, this->davPass.c_str());
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
  curl_easy_setopt(curl, CURLOPT_READDATA, _upload);
  curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, DRIVE_UPLOAD_BUFFER_SIZE);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
  int error = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (error != CURLE_OK) {
    writeCurlError(__func__, error);
  }
}

std::string drive::dav::getServerHostname(const std::string &_url) {
  std::regex r("https?:\\/\\/.*?\\/");
  std::smatch sm;
  std::string ret;

  if (regex_search(_url, sm, r)) {
    ret = sm.str();
    ret.pop_back();
  }

  return ret;
}

void drive::dav::deleteFile(const std::string &_filename) {
  CURL *curl = curl_easy_init();
  std::string davPath = getServerHostname(this->davUrl);
  fs::logWrite("[DAVDrive](%s): CURL DELETE HOST: %s\n", __func__, davPath.c_str());
  if (davPath.empty())
    return;

  davPath.append(_filename);

  fs::logWrite("[DAVDrive](%s): CURL DELETE: %s\n", __func__, davPath.c_str());

  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  curl_easy_setopt(curl, CURLOPT_URL, davPath.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
  curl_easy_setopt(curl, CURLOPT_USERNAME, this->davUser.c_str());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, this->davPass.c_str());

  int error = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (error != CURLE_OK) {
    writeCurlError(__func__, error);
  }
}
