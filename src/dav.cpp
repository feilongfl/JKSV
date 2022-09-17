#include <condition_variable>
#include <curl/curl.h>
#include <json-c/json.h>
#include <mutex>
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

/* file method,
 * special thanks for: https://code.blogs.iiidefix.net/posts/webdav-with-curl/
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
