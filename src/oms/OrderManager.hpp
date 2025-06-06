#pragma once
#include "interfaces/IOrderManager.hpp"
#include "oms/OrderTypes.hpp"
#include "interfaces/IMarketDataSource.hpp"
#include "interfaces/IRiskManager.hpp" // Added
#include <map>
#include <string>
#include <atomic>
#include <mutex> // For protecting shared data structures
#include <vector> // Required by IOrderManager.hpp (included by OrderTypes.hpp but good to be explicit if used directly)

namespace hpts {
namespace oms {

class OrderManager : public interfaces::IOrderManager {
public:
    explicit OrderManager(interfaces::IMarketDataSource* market_data_source,
                          interfaces::IRiskManager* risk_manager); // Added IRiskManager
    ~OrderManager() override = default;

    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;
    OrderManager(OrderManager&&) = delete;
    OrderManager& operator=(OrderManager&&) = delete;


    bool send_order(Order& order) override;
    bool cancel_order(const std::string& order_id, const std::string& client_order_id = "") override;
    void set_execution_report_callback(interfaces::ExecutionReportCallback callback) override;

private:
    void process_order(Order order_copy); // Pass order by value to process asynchronously or copy
    void update_position(const ExecutionReport& report);
    // Changed signature to pass a more complete Order object to build the report
    void send_exec_report(const Order& order_state_for_report, OrderStatus report_status, const std::string& reject_reason = "", double last_filled_price = 0.0, long long last_filled_qty = 0);


    std::atomic<uint64_t> m_next_order_id_counter;
    std::map<uint64_t, Order> m_active_orders; // internal_order_id (uint64_t) -> Order
    std::map<std::string, Position> m_positions;  // instrument_id -> Position
    interfaces::ExecutionReportCallback m_exec_report_callback;
    interfaces::IMarketDataSource* m_market_data_source; // Not owned
    interfaces::IRiskManager* m_risk_manager;         // Not owned

    std::mutex m_orders_mutex;    // Mutex for m_active_orders
    std::mutex m_positions_mutex; // Mutex for m_positions
};

} // namespace oms
} // namespace hpts
