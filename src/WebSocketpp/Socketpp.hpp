#pragma once 
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <mutex>
#include <websocketpp/client.hpp> 
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/config/asio_client.hpp>
#include "Socket.hpp"
#include <condition_variable>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
class connection_metadata {
public:
	typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;
	connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri) : m_id(id)
      	, m_hdl(hdl)
      	, m_status("Connecting")
      	, m_uri(uri)
      	, m_server("N/A") {}
	connection_metadata() {}
    
	void on_open(client * c, websocketpp::connection_hdl hdl) {
        	m_status = "Open";
		std::cout << "Connection opened!\n";
        	client::connection_ptr con = c->get_con_from_hdl(hdl);
        	m_server = con->get_response_header("Server");
    	}
 
    	void on_fail(client * c, websocketpp::connection_hdl hdl) {
        	m_status = "Failed";
 	       	client::connection_ptr con = c->get_con_from_hdl(hdl);
        	m_server = con->get_response_header("Server");
        	m_error_reason = con->get_ec().message();
 		std::cout << "Connection failed: " << m_error_reason << "\n"; // Log failure reason
    	}

	void on_message(websocketpp::connection_hdl /*hdl*/, client::message_ptr msg){
		{
        		std::lock_guard<std::mutex> lock(m_mutex);
        		msg_queue.push_back(msg->get_payload());
    		}
    		m_cv.notify_one(); // Notify one waiting thread
	}
public:
    	int m_id;
    	websocketpp::connection_hdl m_hdl;
    	std::string m_status;
    	std::string m_uri;
    	std::string m_server;
    	std::string m_error_reason;
    	std::vector<std::string> msg_queue;
	std::mutex m_mutex;
	std::condition_variable m_cv;
};

class Socketpp: public Socket{
public:
	Socketpp(); // Constructor
	~Socketpp(); // Destructor
	void switch_to_ws() override;
	[[nodiscard]] std::pair<int, std::string> ws_request(const std::string& msg) override;
private:
	client m_endpoint;
	connection_metadata::ptr con_metadata;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_ws;
};
