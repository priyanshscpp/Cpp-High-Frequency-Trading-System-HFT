#pragma once
#include "interfaces/IStrategy.hpp"
#include "interfaces/IOrderManager.hpp"
#include "interfaces/IMarketDataSource.hpp" // For Tick
#include "oms/OrderTypes.hpp" // For OrderSide etc.

#include <deque>
#include <string>
#include <vector>
#include <numeric> // For std::accumulate
#include <iostream> // For logging

namespace hpts {
namespace strategies {

class MomentumStrategy : public interfaces::IStrategy {
public:
    MomentumStrategy(const std::string& strategy_name,
                       const std::string& instrument_id,
                       int short_ma_window,
                       int long_ma_window,
                       long long order_qty);
    ~MomentumStrategy() override = default;

    std::string get_name() const override;
    void init(interfaces::IOrderManager* order_manager, interfaces::IMarketDataSource* market_data_source) override;
    void on_market_data(const interfaces::Tick& tick) override;
    void on_execution_report(const oms::ExecutionReport& report) override;
    void start() override;
    void stop() override;

private:
    void send_simple_market_order(oms::OrderSide side, long long quantity);
    double calculate_sma(const std::deque<double>& prices) const;

    std::string m_strategy_name;
    std::string m_instrument_id;
    int m_short_ma_window;
    int m_long_ma_window;
    long long m_order_qty;
    std::string m_client_order_id_base;
    uint64_t m_order_counter = 0;

    interfaces::IOrderManager* m_order_manager = nullptr;
    interfaces::IMarketDataSource* m_market_data_source = nullptr;

    std::deque<double> m_price_history_short;
    std::deque<double> m_price_history_long;
    bool m_is_active = false;
    bool m_has_open_position = false;
    oms::OrderSide m_current_position_side = oms::OrderSide::BUY; // Default, updated on fill
    std::string m_active_order_cloid = ""; // Track client ID of active strategy order

    double m_prev_short_sma = 0.0;
    double m_prev_long_sma = 0.0;
};
} // namespace strategies
} // namespace hpts
