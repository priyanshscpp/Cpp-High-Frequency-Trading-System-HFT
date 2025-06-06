#pragma once
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

struct Json {
  std::string method;
  std::string params;
  std::string id;
  const std::string jsonrpc = "2.0";
};
