#pragma once
#include "interfaces/IMarketDataSource.hpp" // For hpts::interfaces::Tick
#include "oms/OrderTypes.hpp"          // For hpts::oms::Order, hpts::oms::ExecutionReport
#include <string>
#include <chrono> // For on_timer, if added

namespace hpts {
namespace interfaces {

class IOrderManager; // Forward declaration

class IStrategy {
public:
    virtual ~IStrategy() = default;

    virtual std::string get_name() const = 0;

    // Called by the system to provide necessary interfaces
    virtual void init(IOrderManager* order_manager, IMarketDataSource* market_data_source) = 0;

    // Event handlers
    virtual void on_market_data(const Tick& tick) = 0;
    virtual void on_execution_report(const oms::ExecutionReport& report) = 0;
    // virtual void on_timer(const std::chrono::system_clock::time_point& current_time) = 0; // Optional for time-based logic

    // Control methods
    virtual void start() = 0; // Called when the strategy should begin processing
    virtual void stop() = 0;  // Called when the strategy should cease operations
};

} // namespace interfaces
} // namespace hpts
