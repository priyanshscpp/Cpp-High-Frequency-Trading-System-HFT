#include "BSocket.hpp"

// Destructor
BSocket::~BSocket(){
	// Close the WebSocket connection
	try {
    		m_ws -> close(boost::beast::websocket::close_code::normal);
		std::cout << "Connection Closed\n";
		delete m_ws;
	} catch (const std::exception& e) {
    		std::cerr << "Error during WebSocket close: " << e.what() << '\n';
	}
}

// Constructor
BSocket::BSocket(){
	// This context is required for all I/O
	net::io_context ioc;

	// The SSL context is required, and holds certificates
	ssl::context ctx{ssl::context::tlsv12_client};
	
	// These objects perform our I/O
        tcp::resolver resolver{ioc};
       	m_ws = new websocket::stream<beast::ssl_stream<tcp::socket>>(ioc, ctx); 

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
	net::connect(m_ws -> next_layer().next_layer(), results.begin(), results.end());

	m_ws -> next_layer().handshake(ssl::stream_base::client);

        std::cout << "BSocket Intialized!\n";
}

[[nodiscard]] std::pair<int, std::string> BSocket::ws_request(const std::string& message){
	 // Send the message
        m_ws -> write(net::buffer(message));

	// This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message into our buffer
        m_ws -> read(buffer);

	std::string resp = beast::buffers_to_string(buffer.data());

	return make_pair(0, resp);
}

void BSocket::switch_to_ws(){
	 // Perform the websocket handshake
        m_ws -> handshake(host, "/ws/api/v2");
	std::cout << "Moved to WebSocket Connection\n";
}
