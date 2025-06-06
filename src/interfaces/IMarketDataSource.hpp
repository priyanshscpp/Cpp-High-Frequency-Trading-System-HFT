#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono> // For timestamp

namespace hpts {
namespace interfaces {

struct Tick {
    std::string instrument_id;
    std::chrono::system_clock::time_point timestamp;
    double bid_price;
    double ask_price;
    double last_price; // Optional, could be inferred or explicitly set
    long long volume;  // Volume associated with last_price or total volume at top of book

    // Order book event types (simplified)
    enum class UpdateType { BID, ASK, TRADE };
    UpdateType type;
    double price;
    long long quantity;
};

// Callback type for market data updates
using MarketDataCallback = std::function<void(const Tick&)>;

class IMarketDataSource {
public:
    virtual ~IMarketDataSource() = default;

    // Register a callback to receive market data ticks
    virtual void set_market_data_callback(MarketDataCallback callback) = 0;

    // Start publishing data (e.g., for a live feed or simulator)
    virtual void start() = 0;

    // Stop publishing data
    virtual void stop() = 0;

    // Subscribe to specific instruments
    virtual void subscribe(const std::string& instrument_id) = 0;
    virtual void unsubscribe(const std::string& instrument_id) = 0;
    // virtual void subscribe(const std::vector<std::string>& instrument_ids) = 0; // Alternative
};

} // namespace interfaces
} // namespace hpts
