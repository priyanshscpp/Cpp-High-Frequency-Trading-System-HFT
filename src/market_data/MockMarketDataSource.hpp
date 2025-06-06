#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "interfaces/IMarketDataSource.hpp" // Adjust path as needed if not in include path

namespace hpts {
namespace market_data {

// Forward declaration if OrderBook becomes complex, otherwise define here or include
// struct OrderBook;

class MockMarketDataSource : public interfaces::IMarketDataSource {
public:
    // Using simplified order book representation directly in Tick for now
    // For a more complex LOB simulation, an OrderBook struct/class would be used here.
    // struct OrderBook {
    //     std::map<double, long long, std::greater<double>> bids; // Price -> Quantity
    //     std::map<double, long long, std::less<double>> asks;    // Price -> Quantity
    //     double current_best_bid = 0.0;
    //     double current_best_ask = 0.0;
    // };

    explicit MockMarketDataSource(double tick_rate_hz_per_instrument = 1.0);
    ~MockMarketDataSource() override;

    // Disable copy/move semantics for simplicity with threads and atomics
    MockMarketDataSource(const MockMarketDataSource&) = delete;
    MockMarketDataSource& operator=(const MockMarketDataSource&) = delete;
    MockMarketDataSource(MockMarketDataSource&&) = delete;
    MockMarketDataSource& operator=(MockMarketDataSource&&) = delete;

    void set_market_data_callback(interfaces::MarketDataCallback callback) override;
    void start() override;
    void stop() override;
    void subscribe(const std::string& instrument_id) override;
    void unsubscribe(const std::string& instrument_id) override;

private:
    void simulation_loop();
    void generate_tick_for_instrument(const std::string& instrument_id);

    std::set<std::string> m_subscribed_instruments;
    // std::map<std::string, OrderBook> m_order_books; // For more complex simulation

    // Simplified: Store last known prices per instrument for tick generation
    struct InstrumentState {
        double bid_price;
        double ask_price;
        double last_trade_price;
    };
    std::map<std::string, InstrumentState> m_instrument_states;


    interfaces::MarketDataCallback m_market_data_callback;
    std::thread m_simulation_thread;
    std::atomic<bool> m_running{false};
    double m_tick_interval_seconds; // Derived from tick_rate_hz
};

} // namespace market_data
} // namespace hpts
