#pragma once
#include "oms/OrderTypes.hpp" // For hpts::oms::Order
#include "risk_management/RiskRules.hpp" // For hpts::risk::RiskConfig
#include <string>
#include <vector>

namespace hpts {
namespace interfaces {

enum class RiskCheckResult {
    APPROVED,
    REJECTED_MAX_ORDER_SIZE,
    REJECTED_MAX_OPEN_CONTRACTS_INSTRUMENT,
    REJECTED_MAX_OPEN_CONTRACTS_TOTAL, // Total across all instruments
    REJECTED_MAX_DAILY_VOLUME_INSTRUMENT,
    REJECTED_INSTRUMENT_NOT_ALLOWED,
    REJECTED_SELF_TRADING, // Optional: if system prevents matching own orders
    REJECTED_INSUFFICIENT_MARGIN, // Placeholder for future margin checks
    REJECTED_UNKNOWN
};

// Function to convert RiskCheckResult to string for logging
inline std::string to_string(RiskCheckResult result) {
    switch (result) {
        case RiskCheckResult::APPROVED: return "APPROVED";
        case RiskCheckResult::REJECTED_MAX_ORDER_SIZE: return "REJECTED_MAX_ORDER_SIZE";
        case RiskCheckResult::REJECTED_MAX_OPEN_CONTRACTS_INSTRUMENT: return "REJECTED_MAX_OPEN_CONTRACTS_INSTRUMENT";
        case RiskCheckResult::REJECTED_MAX_OPEN_CONTRACTS_TOTAL: return "REJECTED_MAX_OPEN_CONTRACTS_TOTAL";
        case RiskCheckResult::REJECTED_MAX_DAILY_VOLUME_INSTRUMENT: return "REJECTED_MAX_DAILY_VOLUME_INSTRUMENT";
        case RiskCheckResult::REJECTED_INSTRUMENT_NOT_ALLOWED: return "REJECTED_INSTRUMENT_NOT_ALLOWED";
        case RiskCheckResult::REJECTED_SELF_TRADING: return "REJECTED_SELF_TRADING";
        case RiskCheckResult::REJECTED_INSUFFICIENT_MARGIN: return "REJECTED_INSUFFICIENT_MARGIN";
        case RiskCheckResult::REJECTED_UNKNOWN: return "REJECTED_UNKNOWN";
        default: return "REJECTED_UNDEFINED_REASON";
    }
}


class IRiskManager {
public:
    virtual ~IRiskManager() = default;

    // Checks a new order against defined risk rules.
    // The OrderManager is expected to use the result to decide whether to reject the order.
    // current_position is the current net position of the instrument from OrderManager's perspective.
    virtual RiskCheckResult check_order_pre_send(const hpts::oms::Order& order, const hpts::oms::Position& current_instrument_position) = 0;

    // Updates risk state after an order is filled (or partially filled).
    // ExecutionReport should contain information about the fill (instrument, quantity, side of the trade).
    virtual void update_on_fill(const hpts::oms::ExecutionReport& fill_report, hpts::oms::OrderSide side) = 0;


    // (Optional) Updates risk state if an order is cancelled or otherwise removed from market
    // virtual void update_on_order_inactive(const hpts::oms::Order& order) = 0;

    virtual void load_configuration(const risk::RiskConfig& config) = 0;
};

} // namespace interfaces
} // namespace hpts
