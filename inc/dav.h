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
  bool SetPath(std::string url, std::string user, std::string pass);

private:
  std::string davUrl, davUser, davPass;
};
} // namespace drive
