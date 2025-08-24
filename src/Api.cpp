#include "Api.hpp"

// Destructor
Api::~Api(){
	std::cout << "Destroying Api Instance\n";
	delete m_socket;
}

// Constructor
Api::Api(){
	// Create an instance of mysockets
	
	// Boost Socket Implementation
	m_socket = new BSocket();

	// WebSocket++ Implementation
	//m_socket = new Socketpp();
	

	// Custom Implementation
	// m_socket = new CSocket();



	// Switch to WebSockets
	m_socket -> switch_to_ws();
	// Authenticate
	int status = Authenticate();
	if(status) {
		std::cout << "Authentication failed!\n";
		return;
	} else{
		std::cout << "Authentication Successful!\n";
	}
}

[[nodiscard]] std::pair<int, std::string> Api::api_public(const std::string& message){
	auto resp = m_socket -> ws_request(message);
	return resp;
}

[[nodiscard]] std::pair<int, std::string> Api::api_private(const std::string& message){
	auto resp = m_socket -> ws_request(message);
	return resp;
}

[[nodiscard]] int Api::Authenticate(){
	std::cout << "Logging in with client credentials\n";
	std::pair<int, std::string> pr  = api_private(auth_msg);
	return pr.first;
}
