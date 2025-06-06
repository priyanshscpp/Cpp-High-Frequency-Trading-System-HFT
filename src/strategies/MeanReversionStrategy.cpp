#include "MeanReversionStrategy.hpp"
#include <sstream> // For client_order_id generation
#include <iomanip> // For std::setprecision with SMA/StdDev logging

namespace hpts {
namespace strategies {

MeanReversionStrategy::MeanReversionStrategy(
    const std::string& strategy_name,
    const std::string& instrument_id,
    int moving_avg_window,
    double num_std_devs,
    long long order_qty)
    : m_strategy_name(strategy_name),
      m_instrument_id(instrument_id),
      m_moving_avg_window(moving_avg_window),
      m_num_std_devs(num_std_devs),
      m_order_qty(order_qty),
      m_order_counter(0),
      m_is_active(false),
      m_has_open_position(false) {
    m_client_order_id_base = strategy_name + "_" + instrument_id + "_";
    std::cout << "[" << m_strategy_name << "] Created for " << m_instrument_id
              << " with window=" << m_moving_avg_window
              << ", stddev_mult=" << m_num_std_devs
              << ", qty=" << m_order_qty << std::endl;
}

std::string MeanReversionStrategy::get_name() const {
    return m_strategy_name;
}

void MeanReversionStrategy::init(interfaces::IOrderManager* order_manager, interfaces::IMarketDataSource* market_data_source) {
    m_order_manager = order_manager;
    m_market_data_source = market_data_source;
    std::cout << "[" << m_strategy_name << "] Initialized with OrderManager and MarketDataSource." << std::endl;
}

void MeanReversionStrategy::start() {
    if (!m_market_data_source || !m_order_manager) {
        std::cerr << "[" << m_strategy_name << "] Error: Not initialized before starting." << std::endl;
        return;
    }
    m_is_active = true;
    m_market_data_source->subscribe(m_instrument_id);
    std::cout << "[" << m_strategy_name << "] Started and subscribed to " << m_instrument_id << std::endl;
}

void MeanReversionStrategy::stop() {
    m_is_active = false;
    if (m_market_data_source) { // Unsubscribe if possible
        m_market_data_source->unsubscribe(m_instrument_id);
    }
    std::cout << "[" << m_strategy_name << "] Stopped and unsubscribed from " << m_instrument_id << std::endl;
}

void MeanReversionStrategy::on_market_data(const interfaces::Tick& tick) {
    if (!m_is_active || tick.instrument_id != m_instrument_id || m_order_manager == nullptr) {
        return;
    }

    // Temporary log to see all received ticks for the subscribed instrument
    if (tick.instrument_id == m_instrument_id) { // Log all ticks for our instrument
       std::cout << "[" << get_name() << "] RX Tick for " << tick.instrument_id << " Type: " << static_cast<int>(tick.type) << " Px: " << tick.price << std::endl;
    }


    // We are interested in trade ticks to build our price history
    if (tick.type != interfaces::Tick::UpdateType::TRADE || tick.price <= 0) {
        return; // Or use mid-price from bid/ask if available and preferred
    }

    // Log relevant trade ticks
    if (tick.instrument_id == m_instrument_id) { // Redundant check, already guarded, but for clarity
        // std::cout << "[" << get_name() << "] Processing TRADE Tick for " << m_instrument_id << ": Px=" << tick.price << std::endl;
    }


    m_price_history.push_back(tick.price);
    if (m_price_history.size() > static_cast<size_t>(m_moving_avg_window)) {
        m_price_history.pop_front();
    }

    if (m_price_history.size() < static_cast<size_t>(m_moving_avg_window)) {
        return; // Not enough data yet
    }

    double sma = calculate_sma();
    double std_dev = calculate_std_dev();

    if (std_dev == 0) return; // Avoid division by zero or static prices

    double upper_band = sma + m_num_std_devs * std_dev;
    double lower_band = sma - m_num_std_devs * std_dev;

    if (m_price_history.size() == static_cast<size_t>(m_moving_avg_window)) { // Only log/trade when window is full
        std::cout << std::fixed << std::setprecision(2); // For cleaner price logging
        std::cout << "[" << get_name() << "] " << m_instrument_id
                  << " Px=" << tick.price
                  << " SMA=" << sma << " SD=" << std_dev
                  << " UB=" << upper_band << " LB=" << lower_band
                  << " PosAct=" << m_has_open_position
                  << " ActOID=" << m_active_order_cloid
                  << std::endl;
    }

    // Only consider trading if there isn't an active order already placed by this strategy instance
    if (!m_active_order_cloid.empty()) {
        // std::cout << "[" << get_name() << "] Holding off new signals, active order exists: " << m_active_order_cloid << std::endl;
        return;
    }

    if (!m_has_open_position) { // If no open position, look for entry signals
        if (tick.price > upper_band) {
            std::cout << "[" << get_name() << "] Signal: Price " << tick.price << " > UpperBand " << upper_band << ". Sending SELL order." << std::endl;
            send_simple_market_order(oms::OrderSide::SELL, m_order_qty);
        } else if (tick.price < lower_band) {
            std::cout << "[" << get_name() << "] Signal: Price " << tick.price << " < LowerBand " << lower_band << ". Sending BUY order." << std::endl;
            send_simple_market_order(oms::OrderSide::BUY, m_order_qty);
        }
    } else { // Has an open position, look to close (revert to SMA)
        if (m_current_position_side == oms::OrderSide::SELL && tick.price <= sma) {
            std::cout << "[" << get_name() << "] Signal: Price " << tick.price << " <= SMA " << sma << ". Closing SHORT position." << std::endl;
            send_simple_market_order(oms::OrderSide::BUY, m_order_qty);
        } else if (m_current_position_side == oms::OrderSide::BUY && tick.price >= sma) {
            std::cout << "[" << get_name() << "] Signal: Price " << tick.price << " >= SMA " << sma << ". Closing LONG position." << std::endl;
            send_simple_market_order(oms::OrderSide::SELL, m_order_qty);
        }
    }
}

void MeanReversionStrategy::on_execution_report(const oms::ExecutionReport& report) {
    if (report.client_order_id.rfind(m_client_order_id_base, 0) != 0) {
        return;
    }

    std::cout << "[" << get_name() << "] Received ExecReport for ClOID=" << report.client_order_id
              << " Status=" << static_cast<int>(report.status)
              << " FilledQty=" << report.filled_quantity
              << " AvgPx=" << report.average_filled_price << std::endl;

    if (report.client_order_id == m_active_order_cloid) {
        if (report.status == oms::OrderStatus::FILLED || report.status == oms::OrderStatus::PARTIALLY_FILLED) {
            // Assuming full fill for simplicity of m_has_open_position state
            if (report.cumulative_filled_quantity >= m_order_qty) { // Check if the order that was active is now fully filled
                if (m_has_open_position) { // This fill was to close the position
                    m_has_open_position = false; // Position is now closed
                    std::cout << "[" << get_name() << "] Position closed for " << m_instrument_id << std::endl;
                } else { // This fill was to open the position
                    m_has_open_position = true;
                    // m_current_position_side was set when order was sent
                    std::cout << "[" << get_name() << "] Position opened for " << m_instrument_id << ". Side: "
                              << (m_current_position_side == oms::OrderSide::BUY ? "BUY" : "SELL") << std::endl;
                }
                m_active_order_cloid = ""; // Order completed (either fully filled opening or fully filled closing)
            } else if (report.status == oms::OrderStatus::PARTIALLY_FILLED) {
                // Logic for partial fills:
                // If it was an order to open, a position is now partially open.
                if (!m_has_open_position) {
                    m_has_open_position = true; // Mark that we have some position
                    // m_current_position_side was already set when order was sent
                     std::cout << "[" << get_name() << "] Position PARTIALLY opened for " << m_instrument_id << ". Side: "
                              << (m_current_position_side == oms::OrderSide::BUY ? "BUY" : "SELL")
                              << " Filled: " << report.cumulative_filled_quantity << "/" << m_order_qty << std::endl;
                } else {
                    // If it was an order to close a position, it's now partially closed.
                    // The m_has_open_position remains true.
                    std::cout << "[" << get_name() << "] Position PARTIALLY closed for " << m_instrument_id
                              << ". Remaining " << (m_order_qty - report.cumulative_filled_quantity) << " on this leg." << std::endl;
                }
                // For partial fills, m_active_order_cloid might remain if the order is still considered active in the market.
                // However, for this simplified model, if an ExecutionReport comes for an active order,
                // and it's a partial fill, we might still clear m_active_order_cloid to allow new decisions,
                // or the strategy needs to track remaining quantity for the active order.
                // For now, let's assume a partial fill means the order is no longer actively managed by new signals,
                // until it's fully closed or a new distinct decision is made. This is a simplification.
                // m_active_order_cloid = ""; // Or manage remaining quantity.
                 std::cout << "[" << get_name() << "] Order " << m_active_order_cloid << " PARTIALLY FILLED. Active ClOID cleared (simplification)." << std::endl;
                 m_active_order_cloid = "";


            }
        } else if (report.status == oms::OrderStatus::REJECTED ||
                   report.status == oms::OrderStatus::CANCELLED) {
            std::cout << "[" << get_name() << "] Order " << m_active_order_cloid << " "
                      << (report.status == oms::OrderStatus::REJECTED ? "REJECTED" : "CANCELLED")
                      << ". Reason: " << report.reject_reason << std::endl;
            // If the order that was meant to open/close a position failed,
            // m_has_open_position should reflect the state *before* this order was attempted.
            // This means if it was an order to open, m_has_open_position was false and should remain false.
            // If it was an order to close, m_has_open_position was true and should remain true.
            // The m_current_position_side also remains unchanged.
            m_active_order_cloid = ""; // Order is no longer active
        }
    }
}

void MeanReversionStrategy::send_simple_market_order(oms::OrderSide side, long long quantity) {
    if (!m_order_manager || quantity <= 0) return;

    // Prevent sending new orders if one is already outstanding for this strategy instance
    // This is a critical check to prevent multiple orders before fill confirmation.
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
    // OrderManager will set order_id and timestamp

    std::cout << "[" << get_name() << "] Sending order: ClOID=" << order.client_order_id
              << " Side=" << (side == oms::OrderSide::BUY ? "BUY" : "SELL")
              << " Qty=" << quantity << " for " << m_instrument_id << std::endl;

    m_active_order_cloid = order.client_order_id; // Track the latest sent order
    // Important: store the intended side of the position this order will create/affect
    if (!m_has_open_position) { // This order is to OPEN a position
        m_current_position_side = side;
    }
    // If this order is to CLOSE a position, m_current_position_side already reflects the open position's side.
    // The order's side will be opposite.

    m_order_manager->send_order(order);
}

double MeanReversionStrategy::calculate_sma() const {
    if (m_price_history.empty()) return 0.0;
    double sum = std::accumulate(m_price_history.begin(), m_price_history.end(), 0.0);
    return sum / m_price_history.size();
}

double MeanReversionStrategy::calculate_std_dev() const {
    if (m_price_history.size() < 2) return 0.0; // Std dev not meaningful for <2 points
    double sma = calculate_sma();
    double sq_sum_diff = 0.0;
    for (double price : m_price_history) {
        sq_sum_diff += std::pow(price - sma, 2);
    }
    return std::sqrt(sq_sum_diff / m_price_history.size()); // Population standard deviation
}

} // namespace strategies
} // namespace hpts
