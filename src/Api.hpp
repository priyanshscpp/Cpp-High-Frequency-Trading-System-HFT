#pragma once
#include "utility.hpp"
#include "BSocket.hpp"
#include "Socketpp.hpp"
#include <nlohmann/json.hpp>

class Api{
public:
	// Constructor
	Api();
	// Destructor
	~Api();
	// Methods
	[[nodiscard]] std::pair<int, std::string> api_public(const std::string& msg);
	[[nodiscard]] std::pair<int, std::string> api_private(const std::string& msg);
	[[nodiscard]] int Authenticate();
private:
	Socket* m_socket;
	const std::string auth_msg = R"({
  	"jsonrpc": "2.0",
	"id": 9929,
  	"method": "public/auth",
  	"params": {
    		"grant_type": "client_credentials",
    		"client_id": "vO3I_xM-",
    		"client_secret": "urHDx-2Kd6PXic3zKmmyMHyHDP0JYZBTsq_9b_ojE98"
  	}})";

};
