#include "CSocket.hpp"
#include "CParser.hpp"

// Constructor
CSocket::CSocket() : m_ctx(nullptr), m_ssl(nullptr), m_sock(-1) {
    // Initialize OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // Create SSL context
    m_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ctx) {
        std::cerr << "Failed to create SSL context.\n";
        return;
    }

    // Resolve hostname to IP
    struct hostent* server = gethostbyname(HOST.c_str());
    if (!server) {
        std::cerr << "Failed to resolve hostname.\n";
        SSL_CTX_free(m_ctx);
        return;
    }

    // Create a socket
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0) {
        std::cerr << "Failed to create socket.\n";
        SSL_CTX_free(m_ctx);
        return;
    }

    // Set up server address
    struct sockaddr_in server_addr{};
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Connect to the server
    if (connect(m_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to " << HOST << ":" << PORT << ".\n";
        close(m_sock);
        SSL_CTX_free(m_ctx);
        return;
    }
    std::cout << "Connected to " << HOST << ":" << PORT << ".\n";

    // Create SSL structure and bind it to the socket
    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) {
        std::cerr << "Failed to create SSL structure.\n";
        close(m_sock);
        SSL_CTX_free(m_ctx);
        return;
    }
    SSL_set_fd(m_ssl, m_sock);

    // Perform SSL handshake
    if (SSL_connect(m_ssl) <= 0) {
        std::cerr << "SSL handshake failed.\n";
        ERR_print_errors_fp(stderr);
        SSL_free(m_ssl);
        close(m_sock);
        SSL_CTX_free(m_ctx);
        return;
    }
    std::cout << "SSL handshake successful.\n";
}

// Destructor
CSocket::~CSocket() {
    std::cout << "Closing socket and cleaning up resources.\n";
    if (m_ssl) SSL_free(m_ssl);
    if (m_sock >= 0) close(m_sock);
    if (m_ctx) SSL_CTX_free(m_ctx);
}

// Generate a random WebSocket key
std::string CSocket::generate_websocket_key() {
    unsigned char key[16];
    RAND_bytes(key, sizeof(key));
    std::ostringstream oss;
    for (unsigned char c : key) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

// Send WebSocket upgrade request
int CSocket::switch_to_ws() {
    // Generate a random WebSocket key
    std::string websocket_key = generate_websocket_key();

    // Construct WebSocket upgrade request
    std::string request =
        "GET /ws/api/v2 HTTP/1.1\r\n"
        "Host: " + HOST + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + websocket_key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";

    if (SSL_write(m_ssl, request.c_str(), request.size()) <= 0) {
        std::cerr << "Failed to send request.\n";
        return 1;
    }
    std::cout << "WebSocket upgrade request sent.\n";

    // Read response
    size_t rBytes = 0;
    char buffer[1024] = {0};
    if (SSL_read_ex(m_ssl, buffer, sizeof(buffer) - 1, &rBytes) <= 0) {
        std::cerr << "Failed to read response.\n";
        return 1;
    }
    std::cout << "Response from server:\n" << buffer << "\nRead " << rBytes << "\n";

    // Check for successful handshake
    if (strstr(buffer, "101 Switching Protocols") == nullptr) {
        std::cerr << "WebSocket handshake failed.\n";
        return 1;
    }

    std::cout << "WebSocket handshake successful.\n";
    return 0;
}

// Send a WebSocket Request
int CSocket::ws_request(const std::string& message) {
	if (SSL_write(m_ssl, message.c_str(), message.size()) <= 0) {
        	std::cerr << "Failed to send request.\n";
        	return 1;
    	}

	std::cout << "Message sent: " << message << "\n";
 	// Read response
	size_t rBytes = 0;
    	char buffer[1024] = {0};
    	if (SSL_read_ex(m_ssl, buffer, sizeof(buffer) - 1, &rBytes) <= 0) {
       		std::cerr << "Failed to read response.\n";
        	return 1;
    	}
    	std::cout << "Response from server:\n" << buffer << "\nRead " << rBytes << "\n";
    	return 0;
}
