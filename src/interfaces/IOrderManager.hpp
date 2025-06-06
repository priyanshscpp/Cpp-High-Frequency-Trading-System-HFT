#pragma once
#include "oms/OrderTypes.hpp" // Adjust path as needed
#include <functional>
#include <string>
#include <vector> // For potential bulk operations later

namespace hpts {
namespace interfaces {

// Callback for execution reports
using ExecutionReportCallback = std::function<void(const oms::ExecutionReport&)>;

class IOrderManager {
public:
    virtual ~IOrderManager() = default;

    // Called by strategies to send a new order
    // Returns true if the order is accepted for processing, false otherwise (e.g., invalid parameters before risk check)
    virtual bool send_order(oms::Order& order) = 0; // Pass by non-const ref to allow OrderManager to assign order_id

    // Called by strategies to cancel an order
    // client_order_id can be used if internal order_id is not yet known by strategy
    virtual bool cancel_order(const std::string& order_id, const std::string& client_order_id = "") = 0;

    // Register a callback to receive execution reports
    virtual void set_execution_report_callback(ExecutionReportCallback callback) = 0;

    // (Optional) For interaction with the exchange simulator / market data
    // This might be used if the OrderManager needs to react to market events for conditional orders,
    // or if the exchange interaction is more complex.
    // For now, the MockExchange will be passive, and OrderManager will simulate fills.
    // virtual void on_market_tick(const hpts::interfaces::Tick& tick) = 0;
};

} // namespace interfaces
} // namespace hpts
