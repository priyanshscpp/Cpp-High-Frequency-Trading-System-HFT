# High-Performance Trading Backend in C++

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-CMake-brightgreen.svg)](CMakeLists.txt)

A high-frequency trading (HFT) system built in C++ for ultra-low latency order execution targeting cryptocurrency derivatives markets via Deribit's exchange APIs. Engineered to achieve microsecond-level latency for critical path operations while maintaining robustness under extreme market volatility (10,000+ orders/sec).

📺 **Watch the Project Overview**: [High-Performance Trading Backend Demo](https://www.youtube.com/watch?v=LjLToXJV6lk)

## 📋 Project Overview

This High-Frequency Trading (HFT) System represents a sophisticated C++ implementation designed for institutional-grade cryptocurrency derivatives trading. The system is built from the ground up to address the unique challenges of modern electronic trading:

### 🎯 **Core Mission**
- **Ultra-Low Latency**: Achieve microsecond-level performance for critical trading operations
- **High Throughput**: Handle 10,000+ orders per second under extreme market conditions
- **Institutional Grade**: Provide the reliability and performance expected by professional trading firms
- **Real-Time Processing**: Deliver sub-millisecond market data and order execution

### 🏛️ **Target Markets**
- **Spot Trading**: Direct cryptocurrency purchases and sales
- **Futures Contracts**: Leveraged derivatives with sophisticated risk management
- **Options Trading**: Complex derivative instruments with advanced pricing models
- **Cross-Exchange Arbitrage**: Multi-venue trading opportunities

### 🔬 **Technical Innovation**
- **Custom WebSocket Implementation**: Built from scratch for maximum performance
- **Lock-Free Data Structures**: Eliminate contention in high-frequency scenarios
- **Hardware-Accelerated Crypto**: Leverage Intel AES-NI and ARMv8 crypto instructions
- **Zero-Copy Message Processing**: Minimize memory allocation overhead

### 🚀 **Performance Characteristics**
- **Order Placement Latency**: <100 microseconds end-to-end
- **Market Data Processing**: Real-time tick processing with <1ms latency
- **Risk Calculation**: Sub-millisecond position and PnL updates
- **Connection Resilience**: Automatic failover and reconnection handling

## 🚀 Features

- **Ultra-Low Latency**: Microsecond-level performance for critical trading operations
- **Multi-Protocol Support**: REST and WebSocket interfaces for order management and real-time market data
- **Comprehensive Coverage**: Support for spot, futures, and options instruments
- **High Throughput**: Designed to handle 10,000+ orders per second
- **CLI-Based Interface**: Command-line tools for trading operations and system management
- **Real-Time Market Data**: Custom WebSocket server for streaming market information
- **Robust Risk Management**: Built-in position and risk controls
- **Performance Benchmarking**: Comprehensive latency testing and optimization tools

## 🏗️ Architecture Overview

### System Flow Diagram

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Client Layer  │    │    API Layer    │    │  Core Engine    │
│                 │    │                 │    │                 │
│ • CLI/GUI       │───▶│ • REST API      │───▶│ • Order Manager │
│ • Algo Engines  │    │ • WebSocket API │    │ • Risk Manager  │
└─────────────────┘    └─────────────────┘    │ • Execution     │
                                              │ • Market Data   │
                                              └─────────────────┘
                                                       │
                                                       ▼
                                              ┌─────────────────┐
                                              │  Event Bus &    │
                                              │    Caches       │
                                              │                 │
                                              │ • In-Memory Bus │
                                              │ • Time-Series   │
                                              └─────────────────┘
                                                       │
                                                       ▼
                                              ┌─────────────────┐
                                              │   Deribit       │
                                              │  Connectivity   │
                                              │                 │
                                              │ • REST Client   │
                                              │ • WebSocket     │
                                              └─────────────────┘
```

### Core Components

#### 1. **Client Layer**
- **CLI/GUI**: Interactive user tools for manual trading
- **Algo Engines**: Automated trading strategies
- **Order Submission**: HTTP/JSON via REST API or WebSocket

#### 2. **API Layer**
- **REST API Server**: Synchronous HTTP requests for order management
- **WebSocket API Server**: Real-time market data and order updates
- **JSON-RPC Parsing**: Efficient payload processing

#### 3. **Core Engine**
- **Order Manager**: Order validation and routing
- **Risk & Position Manager**: Real-time PnL and limit enforcement
- **Execution Engine**: Deribit API integration
- **Market Data Distributor**: Real-time tick data processing

#### 4. **Event Bus & Caches**
- **In-Memory Event Bus**: High-throughput, lock-free pub/sub
- **Time-Series Cache**: Lock-free ring buffers for recent data

#### 5. **Deribit Connectivity**
- **REST Client**: cURL/Boost.Beast for synchronous operations
- **WebSocket Client**: Boost.Beast/Websocket++ for streaming

## 📁 Codebase Structure

```
Frequency_Check_Algo/
├── src/
│   ├── Api.cpp/hpp              # Main interface layer
│   ├── Trader.cpp/hpp           # Trading logic engine
│   ├── main.cpp                 # Application entry point
│   ├── Boost_WebSocket/         # Boost.Asio WebSocket client
│   ├── Custom_WebSocket/        # Custom socket implementation
│   ├── WebSocketpp/             # websocketpp library client
│   ├── interfaces/              # Abstract interfaces
│   ├── market_data/             # Market data handling
│   ├── oms/                     # Order management system
│   ├── risk_management/         # Risk controls
│   └── strategies/              # Trading strategies
├── test/
│   └── test_latency/           # Performance testing
├── CMakeLists.txt              # Build configuration
└── README.md                   # This file
```

### Key Components

#### ✅ **Api.hpp & Api.cpp** — Main Interface Layer
- **Responsibilities**: JSON RPC serialization/deserialization
- **Features**: Client authentication, clean abstraction layer
- **Methods**: Order placement, cancellation, modification

#### ✅ **BSocket.hpp & BSocket.cpp** — Boost WebSocket Client
- **Technology**: Boost.Asio/Beast implementation
- **Features**: Production-grade error handling, async I/O
- **Use Case**: Fallback and test environments

#### ✅ **Socketpp.hpp & Socketpp.cpp** — websocketpp Library Client
- **Technology**: websocketpp::config::asio_tls_client
- **Features**: TLS encryption, thread-safe async handling
- **Performance**: Sub-millisecond latency optimization

#### ✅ **Trader.hpp & Trader.cpp** — Trading Engine
- **Order Management**: Place, cancel, modify orders
- **Market Data**: Order book, positions, open orders
- **Integration**: API and socket layer coordination

## 🔧 Performance & Optimization

### Latency Optimization
- **WebSocket Protocol**: RFC 6455 with persistent TCP connections
- **Zero-Copy Message Retrieval**: Direct in-memory buffer processing
- **Condition Variable Wakeup**: Immediate thread activation
- **Result**: Sub-millisecond round-trip latency

### Threading & Concurrency
- **Mutex-Guarded Queues**: Race condition prevention
- **Dedicated Worker Threads**: I/O decoupling from business logic
- **Producer-Consumer Pattern**: High-throughput message processing

### TLS Performance
- **Asynchronous Handshake**: Non-blocking TLS setup
- **Session Reuse**: Connection and TCP session optimization
- **Hardware Acceleration**: Intel AES-NI and ARMv8 crypto support
- **Result**: <10% latency overhead with optimized ciphers

### Scaling Considerations
- **Vertical Scaling**: Efficient CPU core utilization
- **Horizontal Scaling**: Stateless request model for microservices
- **Connection Pooling**: Extensible for high-load scenarios

## 🧪 Testing & Benchmarking

### Performance Testing Suite
Located in `test/test_latency/` directory:

- **Latency Measurement**: Round-trip timing for all operations
- **Throughput Testing**: Orders per second capacity
- **Protocol Comparison**: WebSocket++ vs Boost vs Custom implementations
- **Async vs Sync**: Performance benchmarking of different approaches

### Test Results
- **WebSocket++**: Outperforms Boost Beast implementation
- **Async Calls**: Better performance than synchronous counterparts
- **Latency**: Sub-millisecond performance under optimal conditions

## 🚀 Getting Started

### Prerequisites
- C++17 compatible compiler
- CMake 3.10+
- Boost libraries
- OpenSSL development libraries

### Building the Project
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running Tests
```bash
cd test/test_latency
make
./test_latency
```

## 🔮 Future Improvements

### 1. **Modular Plugin System**
- **Purpose**: Easy adaptation to multiple data sources
- **Implementation**: Interface-based architecture with runtime loading
- **Benefits**: Protocol flexibility (Binance, Kraken, custom APIs)

### 2. **Caching & Batching**
- **Message Batching**: Periodic chunk transmission
- **LRU Cache**: Response caching with eviction policies
- **Benefits**: Reduced downstream load and bandwidth

### 3. **AI-Powered Data Filters**
- **ML Anomaly Detection**: Filter abnormal market data
- **NLP Intent Recognition**: Smart command-based interactions
- **Benefits**: Intelligent, context-aware backend

## 📊 Monitoring & Observability

### Logging & Metrics
- **Structured Logs**: JSON format for easy parsing
- **Prometheus Integration**: Latency, error rate, fill rate metrics
- **Audit Trails**: Complete order and execution history

### Health Monitoring
- **Grafana Dashboards**: Real-time system performance
- **Alertmanager Rules**: Automated issue detection
- **Key Metrics**: Latency spikes, risk breaches, connection drops

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**Priyanshu Yadav**  
Final Year BTech ECE  
📧 priyanshs.ece@gmail.com

## 🙏 Acknowledgments

- **Deribit**: Exchange API integration
- **Boost Libraries**: High-performance C++ components
- **WebSocket++**: WebSocket protocol implementation
- **Open Source Community**: Tools and libraries that made this possible

---

⭐ **Star this repository if you find it helpful!**

For questions about architecture, optimizations, or low-latency C++ design, feel free to reach out!
