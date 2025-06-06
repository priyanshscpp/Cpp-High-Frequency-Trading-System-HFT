#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iomanip>

#include <utility.hpp>  // Changed to <>
#include "CParser.hpp"

class CSocket {
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
  const int PORT = 443;  // HTTPS Port
  SSL_CTX* m_ctx;
  SSL* m_ssl;
  int m_sock;
  CParser cparser;
};
