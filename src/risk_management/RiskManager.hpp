#pragma once

#include "interfaces/IRiskManager.hpp"
#include "risk_management/RiskRules.hpp"
#include <map>
#include <string>
#include <mutex> // For protecting shared data structures
#include <atomic> // For potential atomic counters if needed, though mutex is primary here

namespace hpts {
namespace risk {

class RiskManager : public interfaces::IRiskManager {
public:
    RiskManager(); // Constructor can initialize with default config
    explicit RiskManager(const RiskConfig& initial_config);
    ~RiskManager() override = default;

    RiskManager(const RiskManager&) = delete;
    RiskManager& operator=(const RiskManager&) = delete;
    RiskManager(RiskManager&&) = delete;
    RiskManager& operator=(RiskManager&&) = delete;

    void load_configuration(const RiskConfig& config) override;

    interfaces::RiskCheckResult check_order_pre_send(
        const oms::Order& order,
        const oms::Position& current_instrument_position) override;

    void update_on_fill(const oms::ExecutionReport& fill_report, oms::OrderSide side) override;

private:
    RiskConfig m_config;
    std::map<std::string, InstrumentRiskState> m_instrument_states;
    long long m_total_open_contracts_all_instruments; // Sum of abs(net_position) for each instrument

    std::mutex m_risk_mutex; // Protects m_config, m_instrument_states, m_total_open_contracts_all_instruments
};

} // namespace risk
} // namespace hpts
