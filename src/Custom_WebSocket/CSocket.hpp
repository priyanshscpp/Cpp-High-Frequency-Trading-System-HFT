#include "utility.hpp"
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <openssl/rand.h> 
#include <iomanip>
#include "CParser.hpp"


class CSocket{
public:
	// Constructor
	CSocket();
	// Destructor
	~CSocket();
	
	int switch_to_ws();
	std::string generate_websocket_key();
	int ws_request(const std::string& msg);
private:
	const std::string HOST = "test.deribit.com";
	const int PORT = 443; // HTTPS Port
	SSL_CTX* m_ctx;
	SSL* m_ssl;
	int m_sock;
	CParser cparser;
};
