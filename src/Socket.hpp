#pragma once
#include <utility>
#include <memory>
#include <string>

class Socket{
public:
	virtual ~Socket();
	virtual void switch_to_ws() = 0;
	[[nodiscard]] virtual std::pair<int, std::string> ws_request(const std::string& msg) = 0;
protected:
	const std::string host = "test.deribit.com";
	const std::string port = "443";
};
