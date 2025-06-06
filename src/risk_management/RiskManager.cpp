#include "RiskManager.hpp"
#include <iostream> // For logging
#include <numeric>  // For std::accumulate
#include <cmath>    // For std::abs

namespace hpts {
namespace risk {

RiskManager::RiskManager() : m_total_open_contracts_all_instruments(0) {
    // Initialize with default RiskConfig (already done by m_config default constructor)
    std::cout << "RiskManager initialized with default configuration." << std::endl;
}

RiskManager::RiskManager(const RiskConfig& initial_config)
    : m_config(initial_config), m_total_open_contracts_all_instruments(0) {
    std::cout << "RiskManager initialized with provided configuration." << std::endl;
    // Potentially pre-populate m_instrument_states if config has per-instrument initial states
}

void RiskManager::load_configuration(const RiskConfig& config) {
    std::lock_guard<std::mutex> lock(m_risk_mutex);
    m_config = config;
    // Resetting states based on new config might be needed if instruments change etc.
    // For now, keep existing states but new orders will use new config.
    // A more robust implementation might clear m_instrument_states or update them.
    std::cout << "Risk configuration loaded." << std::endl;
    std::cout << "Max Order Size: " << m_config.max_order_size << std::endl;
    std::cout << "Max Open Contracts/Instrument: " << m_config.max_open_contracts_per_instrument << std::endl;
    if (!m_config.allowed_instruments.empty()) {
        std::cout << "Allowed instruments set." << std::endl;
    }
}

interfaces::RiskCheckResult RiskManager::check_order_pre_send(
    const oms::Order& order,
    const oms::Position& current_instrument_position_from_oms) { // Renamed for clarity
    std::lock_guard<std::mutex> lock(m_risk_mutex);

    // 1. Max Order Size
    if (order.quantity > m_config.max_order_size) {
        return interfaces::RiskCheckResult::REJECTED_MAX_ORDER_SIZE;
    }

    // 2. Allowed Instruments
    if (!m_config.allowed_instruments.empty() &&
        m_config.allowed_instruments.find(order.instrument_id) == m_config.allowed_instruments.end()) {
        return interfaces::RiskCheckResult::REJECTED_INSTRUMENT_NOT_ALLOWED;
    }

    // Retrieve or create current risk state for the instrument
    InstrumentRiskState& instr_state = m_instrument_states[order.instrument_id]; // Creates if not exists (for daily_traded_volume and RM's own net_pos tracking)

    // 3. Max Daily Volume per Instrument
    if ((instr_state.daily_traded_volume + order.quantity) > m_config.max_daily_volume_per_instrument) {
        return interfaces::RiskCheckResult::REJECTED_MAX_DAILY_VOLUME_INSTRUMENT;
    }

    // Calculate potential new net position for this instrument based on OrderManager's view passed in
    long long potential_instr_net_pos = current_instrument_position_from_oms.quantity;
    if (order.side == oms::OrderSide::BUY) {
        potential_instr_net_pos += order.quantity;
    } else { // SELL
        potential_instr_net_pos -= order.quantity;
    }

    // 4. Max Open Contracts per Instrument
    if (std::abs(potential_instr_net_pos) > m_config.max_open_contracts_per_instrument) {
        return interfaces::RiskCheckResult::REJECTED_MAX_OPEN_CONTRACTS_INSTRUMENT;
    }

    // 5. Max Total Contracts Across All Instruments
    // Calculate current total based on RiskManager's view, then potential new total
    // This requires RiskManager to maintain its own consistent view of net positions.
    // The current_total_open_contracts is m_total_open_contracts_all_instruments (which is sum of RM's view)
    // The change is: new total = old_total - abs(RM_current_instr_pos) + abs(RM_potential_instr_pos_after_this_order)

    long long rm_potential_instr_net_pos_after_order = instr_state.net_position; // RM's view
    if (order.side == oms::OrderSide::BUY) {
        rm_potential_instr_net_pos_after_order += order.quantity;
    } else { // SELL
        rm_potential_instr_net_pos_after_order -= order.quantity;
    }

    long long potential_total_open_contracts = m_total_open_contracts_all_instruments
                                             - std::abs(instr_state.net_position) // Subtract old abs value for this instrument
                                             + std::abs(rm_potential_instr_net_pos_after_order); // Add new abs value for this instrument

    if (potential_total_open_contracts > m_config.max_total_contracts_across_all_instruments) {
        return interfaces::RiskCheckResult::REJECTED_MAX_OPEN_CONTRACTS_TOTAL;
    }

    // All checks passed
    return interfaces::RiskCheckResult::APPROVED;
}

void RiskManager::update_on_fill(const oms::ExecutionReport& fill_report, oms::OrderSide side) {
    std::lock_guard<std::mutex> lock(m_risk_mutex);

    if (fill_report.status == oms::OrderStatus::FILLED ||
        fill_report.status == oms::OrderStatus::PARTIALLY_FILLED) {

        if (fill_report.filled_quantity == 0) return; // No change if zero quantity fill

        InstrumentRiskState& instr_state = m_instrument_states[fill_report.instrument_id];

        // Update daily traded volume
        instr_state.daily_traded_volume += fill_report.filled_quantity;

        // Update net position for the instrument based on the side of the trade
        long long position_delta = 0;
        if (side == oms::OrderSide::BUY) {
            position_delta = fill_report.filled_quantity;
        } else { // SELL
            position_delta = -fill_report.filled_quantity;
        }
        instr_state.net_position += position_delta;

        // Recalculate total open contracts (sum of absolute net positions)
        m_total_open_contracts_all_instruments = 0;
        for(const auto& pair : m_instrument_states) {
            m_total_open_contracts_all_instruments += std::abs(pair.second.net_position);
        }

        std::cout << "[RiskManager] Updated on fill: Instr=" << fill_report.instrument_id
                  << ", FilledQty=" << fill_report.filled_quantity
                  << ", Side=" << (side == oms::OrderSide::BUY ? "BUY" : "SELL")
                  << ", NewNetPos=" << instr_state.net_position
                  << ", DailyVol=" << instr_state.daily_traded_volume
                  << ", TotalOpenContracts=" << m_total_open_contracts_all_instruments << std::endl;
    }
}

} // namespace risk
} // namespace hpts
