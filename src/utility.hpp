#pragma once
#include <iostream>
#include <cstdint>
#include <memory>
#include <functional>
#include <sstream>

struct Json{
	std::string method;
	std::string params;
	std::string id;
	const std::string jsonrpc = "2.0";
};
