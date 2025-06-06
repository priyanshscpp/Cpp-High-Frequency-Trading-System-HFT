#pragma once
#include "interfaces/IStrategy.hpp"
#include "interfaces/IOrderManager.hpp"
#include "interfaces/IMarketDataSource.hpp" // For Tick definition, though IStrategy includes it
#include "oms/OrderTypes.hpp" // For OrderSide, OrderType, ExecutionReport

#include <deque>
#include <string>
#include <vector>
#include <numeric> // For std::accumulate
#include <cmath>   // For std::sqrt, std::pow
#include <iostream> // For logging

namespace hpts {
namespace strategies {

class MeanReversionStrategy : public interfaces::IStrategy {
public:
    MeanReversionStrategy(const std::string& strategy_name,
                          const std::string& instrument_id,
                          int moving_avg_window,
                          double num_std_devs,
                          long long order_qty);
    ~MeanReversionStrategy() override = default;

    std::string get_name() const override;
    void init(interfaces::IOrderManager* order_manager, interfaces::IMarketDataSource* market_data_source) override;
    void on_market_data(const interfaces::Tick& tick) override;
    void on_execution_report(const oms::ExecutionReport& report) override;
    void start() override;
    void stop() override;

private:
    void send_simple_market_order(oms::OrderSide side, long long quantity);
    double calculate_sma() const;
    double calculate_std_dev() const; // Corrected: should not be const if it modifies member (it does not) or ensure it's truly const.

    std::string m_strategy_name;
    std::string m_instrument_id;
    int m_moving_avg_window;
    double m_num_std_devs;
    long long m_order_qty;
    std::string m_client_order_id_base;
    uint64_t m_order_counter = 0;

    interfaces::IOrderManager* m_order_manager = nullptr;
    interfaces::IMarketDataSource* m_market_data_source = nullptr;

    std::deque<double> m_price_history;
    bool m_is_active = false;
    bool m_has_open_position = false;
    oms::OrderSide m_current_position_side = oms::OrderSide::BUY; // Arbitrary default, updated on fill
    std::string m_active_order_cloid = ""; // Track ClientOrderID of our active strategy order
};

} // namespace strategies
} // namespace hpts
