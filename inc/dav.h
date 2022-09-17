#pragma once

#include <curl/curl.h>
#include <json-c/json.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "curlfuncs.h"

namespace drive {
class dav {
public:
  // method for config
  bool SetPath(std::string url, std::string user, std::string pass);

  // method for files
  bool dirExists(const std::string &_dirName);
  bool createDir(const std::string &_dirName);
  bool fileExists(const std::string &_filename);
  void uploadFile(const std::string &_filename, const std::string &_title,
                  curlFuncs::curlUpArgs *_upload);

private:
  std::string davUrl, davUser, davPass;
};
} // namespace drive
