#pragma once

#include <curl/curl.h>
#include <libxml/xpathInternals.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "curlfuncs.h"

namespace drive {

typedef struct {
  size_t did;
  std::string name, path;
  unsigned int size;
} davItem;

class dav {
public:
  static const size_t DriveID = 2;

  // method for config
  bool SetPath(std::string url, std::string user, std::string pass);

  // method for files
  bool dirExists(const std::string &_dirName);
  bool createDir(const std::string &_dirName);
  void listDir(const std::string &_dirName, std::vector<drive::davItem> &_out);
  bool fileExists(const std::string &_filename);
  void uploadFile(const std::string &_filename, const std::string &_title,
                  curlFuncs::curlUpArgs *_upload);

private:
  // configs
  std::string davUrl, davUser, davPass;

  // method for files
  bool listDirReq(const std::string &_dirName, std::string *xmlResp);

  // method for parse xml
  bool xmlGetItemByPath(std::string &xml, xmlChar *xpath,
                        std::vector<std::string> &_out);
}; // class dav

} // namespace drive
