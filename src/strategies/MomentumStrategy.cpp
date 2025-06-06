#include "MomentumStrategy.hpp"
#include <sstream>   // For client_order_id generation
#include <iomanip>   // For std::setprecision for logging

namespace hpts {
namespace strategies {

MomentumStrategy::MomentumStrategy(
    const std::string& strategy_name,
    const std::string& instrument_id,
    int short_ma_window,
    int long_ma_window,
    long long order_qty)
    : m_strategy_name(strategy_name),
      m_instrument_id(instrument_id),
      m_short_ma_window(short_ma_window),
      m_long_ma_window(long_ma_window),
      m_order_qty(order_qty),
      m_order_counter(0),
      m_is_active(false),
      m_has_open_position(false),
      m_prev_short_sma(0.0),
      m_prev_long_sma(0.0) {
    m_client_order_id_base = strategy_name + "_" + instrument_id + "_";
    if (m_short_ma_window >= m_long_ma_window) {
        // This is a common error, throw or log heavily.
        std::cerr << "[" << m_strategy_name << "] CRITICAL ERROR: Short MA window (" << m_short_ma_window
                  << ") must be less than Long MA window (" << m_long_ma_window << ")." << std::endl;
        // Potentially throw std::invalid_argument or set a flag to prevent start.
    }
    std::cout << "[" << m_strategy_name << "] Created for " << m_instrument_id
              << " with ShortMA=" << m_short_ma_window
              << ", LongMA=" << m_long_ma_window
              << ", Qty=" << m_order_qty << std::endl;
}

std::string MomentumStrategy::get_name() const {
    return m_strategy_name;
}

void MomentumStrategy::init(interfaces::IOrderManager* order_manager, interfaces::IMarketDataSource* market_data_source) {
    m_order_manager = order_manager;
    m_market_data_source = market_data_source;
    std::cout << "[" << m_strategy_name << "] Initialized." << std::endl;
}

void MomentumStrategy::start() {
    if (!m_market_data_source || !m_order_manager) {
        std::cerr << "[" << m_strategy_name << "] Error: Not initialized before starting." << std::endl;
        return;
    }
     if (m_short_ma_window >= m_long_ma_window) {
        std::cerr << "[" << m_strategy_name << "] Error: Invalid MA windows. Cannot start." << std::endl;
        return;
    }
    m_is_active = true;
    m_market_data_source->subscribe(m_instrument_id);
    // Reset previous SMAs on start in case of multiple start/stop cycles
    m_prev_short_sma = 0.0;
    m_prev_long_sma = 0.0;
    std::cout << "[" << m_strategy_name << "] Started and subscribed to " << m_instrument_id << std::endl;
}

void MomentumStrategy::stop() {
    m_is_active = false;
    if (m_market_data_source) {
        m_market_data_source->unsubscribe(m_instrument_id);
    }
    std::cout << "[" << m_strategy_name << "] Stopped and unsubscribed from " << m_instrument_id << std::endl;
}

double MomentumStrategy::calculate_sma(const std::deque<double>& prices) const {
    if (prices.empty()) return 0.0;
    double sum = std::accumulate(prices.begin(), prices.end(), 0.0);
    return sum / prices.size();
}

void MomentumStrategy::on_market_data(const interfaces::Tick& tick) {
    if (!m_is_active || tick.instrument_id != m_instrument_id || m_order_manager == nullptr) {
        return;
    }

    // Temporary log to see all received ticks for the subscribed instrument
    if (tick.instrument_id == m_instrument_id) { // Log all ticks for our instrument
       std::cout << "[" << get_name() << "] RX Tick for " << tick.instrument_id << " Type: " << static_cast<int>(tick.type) << " Px: " << tick.price << std::endl;
    }

    if (tick.type != interfaces::Tick::UpdateType::TRADE || tick.price <= 0) {
        return;
    }

    m_price_history_short.push_back(tick.price);
    if (m_price_history_short.size() > static_cast<size_t>(m_short_ma_window)) {
        m_price_history_short.pop_front();
    }

    m_price_history_long.push_back(tick.price);
    if (m_price_history_long.size() > static_cast<size_t>(m_long_ma_window)) {
        m_price_history_long.pop_front();
    }

    // Log deque sizes for SPY before checking if window is full
    if (tick.instrument_id == "SPY") { // Check specifically for SPY for this log
       std::cout << "[" << get_name() << "] SPY Deque Sizes: Short=" << m_price_history_short.size()
                 << ", Long=" << m_price_history_long.size() << std::endl;
    }


    if (m_price_history_long.size() < static_cast<size_t>(m_long_ma_window) ||
        m_price_history_short.size() < static_cast<size_t>(m_short_ma_window) ) {
        return; // Not enough data for both MAs
    }

    double current_short_sma = calculate_sma(m_price_history_short);
    double current_long_sma = calculate_sma(m_price_history_long);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[" << get_name() << "] " << m_instrument_id
              << " Px=" << tick.price
              << " SMA_S(" << m_short_ma_window << ")=" << current_short_sma
              << " SMA_L(" << m_long_ma_window << ")=" << current_long_sma
              << " PosAct=" << m_has_open_position
              << " ActOID=" << m_active_order_cloid
              << std::endl;

    if (!m_active_order_cloid.empty()) { // Don't act if an order is already pending
        return;
    }

    // Trading Logic (Crossover)
    if (m_prev_short_sma > 0.0001 && m_prev_long_sma > 0.0001) { // Avoid trading on first few data points where prev_smas are 0
        if (!m_has_open_position) {
            if (m_prev_short_sma <= m_prev_long_sma && current_short_sma > current_long_sma) { // Bullish crossover
                std::cout << "[" << get_name() << "] Signal: Bullish Crossover. Sending BUY order." << std::endl;
                send_simple_market_order(oms::OrderSide::BUY, m_order_qty);
            } else if (m_prev_short_sma >= m_prev_long_sma && current_short_sma < current_long_sma) { // Bearish crossover
                std::cout << "[" << get_name() << "] Signal: Bearish Crossover. Sending SELL order." << std::endl;
                send_simple_market_order(oms::OrderSide::SELL, m_order_qty);
            }
        } else { // Has an open position, look for exit (counter-crossover)
            if (m_current_position_side == oms::OrderSide::BUY && current_short_sma < current_long_sma && m_prev_short_sma >= m_prev_long_sma) {
                std::cout << "[" << get_name() << "] Signal: Bearish Crossover. Closing LONG position." << std::endl;
                send_simple_market_order(oms::OrderSide::SELL, m_order_qty);
            } else if (m_current_position_side == oms::OrderSide::SELL && current_short_sma > current_long_sma && m_prev_short_sma <= m_prev_long_sma) {
                std::cout << "[" << get_name() << "] Signal: Bullish Crossover. Closing SHORT position." << std::endl;
                send_simple_market_order(oms::OrderSide::BUY, m_order_qty);
            }
        }
    }

    m_prev_short_sma = current_short_sma;
    m_prev_long_sma = current_long_sma;
}

void MomentumStrategy::on_execution_report(const oms::ExecutionReport& report) {
    if (report.client_order_id.rfind(m_client_order_id_base, 0) != 0) {
        return;
    }

    std::cout << "[" << get_name() << "] Received ExecReport for ClOID=" << report.client_order_id
              << " Status=" << static_cast<int>(report.status)
              << " FilledQty=" << report.filled_quantity
              << " AvgPx=" << report.average_filled_price << std::endl;

    if (report.client_order_id == m_active_order_cloid) {
        if (report.status == oms::OrderStatus::FILLED || report.status == oms::OrderStatus::PARTIALLY_FILLED) {
            if (report.cumulative_filled_quantity >= m_order_qty) { // Assuming full fill for simplicity
                if (m_has_open_position) { // This fill was to close the position
                    m_has_open_position = false;
                    std::cout << "[" << get_name() << "] Position closed for " << m_instrument_id << std::endl;
                } else { // This fill was to open the position
                    m_has_open_position = true;
                    // m_current_position_side was set when order was sent
                     std::cout << "[" << get_name() << "] Position opened for " << m_instrument_id << ". Side: "
                              << (m_current_position_side == oms::OrderSide::BUY ? "BUY" : "SELL") << std::endl;
                }
                m_active_order_cloid = "";
            } else if (report.status == oms::OrderStatus::PARTIALLY_FILLED) {
                 if (!m_has_open_position) m_has_open_position = true; // Partially opened
                 std::cout << "[" << get_name() << "] Order " << m_active_order_cloid << " PARTIALLY FILLED. Active ClOID cleared (simplification)." << std::endl;
                 m_active_order_cloid = ""; // Simplified: treat partial as done for now for new signals
            }
        } else if (report.status == oms::OrderStatus::REJECTED ||
                   report.status == oms::OrderStatus::CANCELLED) {
             std::cout << "[" << get_name() << "] Order " << m_active_order_cloid << " "
                      << (report.status == oms::OrderStatus::REJECTED ? "REJECTED" : "CANCELLED")
                      << ". Reason: " << report.reject_reason << std::endl;
            m_active_order_cloid = "";
        }
    }
}

void MomentumStrategy::send_simple_market_order(oms::OrderSide side, long long quantity) {
    if (!m_order_manager || quantity <= 0) return;

    if (!m_active_order_cloid.empty()) {
        std::cout << "[" << get_name() << "] Cannot send order. Active order exists: " << m_active_order_cloid << std::endl;
        return;
    }

    oms::Order order;
    order.client_order_id = m_client_order_id_base + std::to_string(m_order_counter++);
    order.instrument_id = m_instrument_id;
    order.side = side;
    order.type = oms::OrderType::MARKET;
    order.quantity = quantity;

    std::cout << "[" << get_name() << "] Sending order: ClOID=" << order.client_order_id
              << " Side=" << (side == oms::OrderSide::BUY ? "BUY" : "SELL")
              << " Qty=" << quantity << " for " << m_instrument_id << std::endl;

    m_active_order_cloid = order.client_order_id;
    if (!m_has_open_position) { // This order is to OPEN a position
        m_current_position_side = side;
    }
    // If order is to CLOSE, m_current_position_side already reflects the open position.
    // The order's side will be opposite.

    m_order_manager->send_order(order);
}

} // namespace strategies
} // namespace hpts
