#include "MockMarketDataSource.hpp"

#include <iostream> // For logging/debugging
#include <random>   // For pseudo-random data generation
#include <thread>   // For std::this_thread::sleep_for

namespace hpts {
namespace market_data {

MockMarketDataSource::MockMarketDataSource(double tick_rate_hz_per_instrument) {
    if (tick_rate_hz_per_instrument <= 0) {
        m_tick_interval_seconds = 1.0; // Default to 1 Hz
    } else {
        m_tick_interval_seconds = 1.0 / tick_rate_hz_per_instrument;
    }
    std::cout << "MockMarketDataSource initialized with tick interval: "
              << m_tick_interval_seconds << "s per instrument." << std::endl;
}

MockMarketDataSource::~MockMarketDataSource() {
    if (m_running.load()) {
        stop();
    }
}

void MockMarketDataSource::set_market_data_callback(interfaces::MarketDataCallback callback) {
    m_market_data_callback = callback;
}

void MockMarketDataSource::subscribe(const std::string& instrument_id) {
    m_subscribed_instruments.insert(instrument_id);
    // Initialize some basic state for the instrument
    if (m_instrument_states.find(instrument_id) == m_instrument_states.end()) {
        // Example initial prices; can be made more configurable
        if (instrument_id == "SPY") {
            m_instrument_states[instrument_id] = {100.00, 100.05, 100.02};
        } else if (instrument_id == "AAPL") {
            m_instrument_states[instrument_id] = {150.00, 150.05, 150.03};
        } else {
            m_instrument_states[instrument_id] = {50.00, 50.05, 50.02}; // Default
        }
    }
    std::cout << "Subscribed to: " << instrument_id << std::endl;
}

void MockMarketDataSource::unsubscribe(const std::string& instrument_id) {
    m_subscribed_instruments.erase(instrument_id);
    m_instrument_states.erase(instrument_id); // Remove state
    std::cout << "Unsubscribed from: " << instrument_id << std::endl;
}

void MockMarketDataSource::start() {
    if (m_running.load()) {
        std::cout << "Simulation already running." << std::endl;
        return;
    }
    m_running.store(true);
    m_simulation_thread = std::thread(&MockMarketDataSource::simulation_loop, this);
    std::cout << "MockMarketDataSource simulation started." << std::endl;
}

void MockMarketDataSource::stop() {
    if (!m_running.load()) {
        std::cout << "Simulation not running." << std::endl;
        return;
    }
    m_running.store(false);
    if (m_simulation_thread.joinable()) {
        m_simulation_thread.join();
    }
    std::cout << "MockMarketDataSource simulation stopped." << std::endl;
}

void MockMarketDataSource::simulation_loop() {
    std::random_device rd;
    std::mt19937 gen(rd());
    // Price change distribution (small changes)
    std::uniform_real_distribution<> price_dis(-0.01, 0.01);
    // Volume distribution
    std::uniform_int_distribution<> vol_dis(1, 100);
    // Type distribution (0: BID, 1: ASK, 2: TRADE)
    std::uniform_int_distribution<> type_dis(0, 2);


    while (m_running.load()) {
        for (const auto& instrument_id : m_subscribed_instruments) {
            if (!m_running.load()) break; // Check before processing each instrument

            generate_tick_for_instrument(instrument_id);

            // Sleep per instrument to achieve per-instrument tick rate
            // This is a simplified model; a more complex scheduler might be needed for many instruments
            std::this_thread::sleep_for(std::chrono::duration<double>(m_tick_interval_seconds));
        }
        if (m_subscribed_instruments.empty()) {
             // Avoid busy-looping if no instruments are subscribed
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void MockMarketDataSource::generate_tick_for_instrument(const std::string& instrument_id) {
    if (!m_market_data_callback) return;

    interfaces::Tick tick;
    tick.instrument_id = instrument_id;
    tick.timestamp = std::chrono::system_clock::now();

    // Simplified random tick generation
    // In a real mock, this would involve more sophisticated LOB simulation
    // For now, just slightly adjust previous prices and generate a random event type

    auto& state = m_instrument_states[instrument_id]; // Assumes instrument exists

    // Simulate price changes
    // TODO: Consider making the random number generator a member of the class, seeded once, for better performance and randomness.
    std::random_device rd_device;
    std::mt19937 gen(rd_device());

    // Make changes more significant for testing strategy logic
    std::uniform_real_distribution<> price_change_factor(0.98, 1.02); // +/- 2% for mid-price movement
    std::uniform_real_distribution<> price_spread_factor(0.001, 0.005); // Spread around mid (0.1% to 0.5%)
    std::uniform_int_distribution<> qty_change(1, 10);
    // Increase trade frequency: BID 0 (10%), ASK 1 (10%), TRADE 2-9 (80%)
    std::uniform_int_distribution<> type_dist(0, 9);

    double mid_price_estimate = state.last_trade_price;
    // Initialize with some sensible defaults if state is new or prices are zero
    if (mid_price_estimate <= 0.0001) mid_price_estimate = (state.bid_price + state.ask_price) / 2.0;
    if (mid_price_estimate <= 0.0001) { // Still zero or invalid
        mid_price_estimate = (instrument_id == "AAPL" ? 150.0 : (instrument_id == "SPY" ? 500.0 : 100.0));
    }

    double new_mid = mid_price_estimate * price_change_factor(gen);
    double spread = new_mid * price_spread_factor(gen);
    if (spread < 0.01) spread = 0.01; // Ensure a minimum spread like 1 cent

    tick.bid_price = new_mid - (spread / 2.0);
    tick.ask_price = new_mid + (spread / 2.0);

    // Ensure ask is always greater than bid after calculations
    if (tick.ask_price <= tick.bid_price) {
        tick.ask_price = tick.bid_price + 0.01;
    }

    int event_type_val = type_dist(gen);
    if (event_type_val == 0) { // BID update (10% chance)
        tick.type = interfaces::Tick::UpdateType::BID;
        tick.price = tick.bid_price;
        tick.quantity = qty_change(gen) * 10;
        state.bid_price = tick.bid_price;
    } else if (event_type_val == 1) { // ASK update (10% chance)
        tick.type = interfaces::Tick::UpdateType::ASK;
        tick.price = tick.ask_price;
        tick.quantity = qty_change(gen) * 10;
        state.ask_price = tick.ask_price;
    } else { // TRADE (80% chance)
        tick.type = interfaces::Tick::UpdateType::TRADE;
        // Simulate trade price: could be at bid, at ask, or between/near mid.
        std::uniform_real_distribution<> trade_px_selector(0.0, 1.0);
        if (trade_px_selector(gen) < 0.25) { // Trade at bid
             tick.price = tick.bid_price;
        } else if (trade_px_selector(gen) < 0.5) { // Trade at ask
             tick.price = tick.ask_price;
        } else { // Trade near mid
            tick.price = new_mid + (new_mid * std::uniform_real_distribution<>(-0.0005, 0.0005)(gen)); // +/- 0.05% around mid
        }
        tick.quantity = qty_change(gen);
        state.last_trade_price = tick.price;
        // After a trade, bid/ask might shift. For simplicity, let's update them based on last trade.
        state.bid_price = tick.price * (1.0 - price_spread_factor(gen)/1.5) ; // Tighter spread after trade
        state.ask_price = tick.price * (1.0 + price_spread_factor(gen)/1.5) ;
        if (state.ask_price <= state.bid_price) state.ask_price = state.bid_price + 0.01; // Ensure validity
    }

    tick.last_price = state.last_trade_price; // Report last known trade price
    tick.volume = tick.quantity; // For simplicity, event volume is tick quantity

    m_market_data_callback(tick);
}

} // namespace market_data
} // namespace hpts
