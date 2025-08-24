#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include "Socket.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class BSocket: public Socket{
public:
	BSocket(); // Constructor
	~BSocket(); // Destructor
	void switch_to_ws() override;
	[[nodiscard]] std::pair<int, std::string> ws_request(const std::string& msg) override;
private:
	websocket::stream<beast::ssl_stream<tcp::socket>>* m_ws;
};
