#include "OrderManager.hpp"
#include <iostream> // For logging
#include <random>   // For simulated fills
#include <sstream>  // For generating order IDs

namespace hpts {
namespace oms {

OrderManager::OrderManager(interfaces::IMarketDataSource* market_data_source,
                           interfaces::IRiskManager* risk_manager)
    : m_next_order_id_counter(1),
      m_market_data_source(market_data_source),
      m_risk_manager(risk_manager) {
    std::cout << "OrderManager initialized." << std::endl;
    if (!m_market_data_source) {
        std::cout << "Warning: OrderManager initialized without a MarketDataSource. Market order fills will use dummy prices." << std::endl;
    }
    if (!m_risk_manager) {
        std::cout << "CRITICAL WARNING: OrderManager initialized WITHOUT a RiskManager. No risk checks will be performed." << std::endl;
    }
}

void OrderManager::set_execution_report_callback(interfaces::ExecutionReportCallback callback) {
    m_exec_report_callback = callback;
}

bool OrderManager::send_order(Order& order) {
    // Basic validation (can be expanded)
    if (order.instrument_id.empty() || order.quantity <= 0) {
        send_exec_report(order, OrderStatus::REJECTED, "Invalid parameters: instrument or quantity");
        return false;
    }
    if (order.type == OrderType::LIMIT && order.price <= 0) {
        send_exec_report(order, OrderStatus::REJECTED, "Invalid parameters: price for LIMIT order");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_orders_mutex);
        order.order_id = m_next_order_id_counter++; // Assign internal uint64_t order ID
        order.status = OrderStatus::NEW; // Or PENDING_NEW if async
        order.timestamp = std::chrono::system_clock::now();
        order.filled_quantity = 0; // Ensure filled quantity starts at 0

        m_active_orders[order.order_id] = order; // Use uint64_t as key
    }

    std::cout << "Order received: ClOID=" << order.client_order_id << " OID=" << order.order_id
              << " Instr=" << order.instrument_id << " Qty=" << order.quantity
              << " Px=" << order.price << std::endl;

    // For now, process synchronously. In a real system, this might go to a queue / separate thread.
    // Make a copy for processing to avoid holding lock for too long / allow modifications.

    // --- Risk Check ---
    if (m_risk_manager) {
        oms::Position current_pos; // Create default / empty position
        { // Scope for position mutex lock
            std::lock_guard<std::mutex> pos_lock(m_positions_mutex);
            if (m_positions.count(order.instrument_id)) {
                current_pos = m_positions.at(order.instrument_id);
            } else {
                current_pos.instrument_id = order.instrument_id; // Ensure instrument ID is set
            }
        }

        interfaces::RiskCheckResult risk_result = m_risk_manager->check_order_pre_send(order, current_pos);
        if (risk_result != interfaces::RiskCheckResult::APPROVED) {
            std::string reason = interfaces::to_string(risk_result);
            std::cout << "Order REJECTED by RiskManager: OID=" << order.order_id << " Reason: " << reason << std::endl;
            // Update order status in m_active_orders (important to do this under lock)
            {
                 std::lock_guard<std::mutex> lock(m_orders_mutex);
                 if(m_active_orders.count(order.order_id)) {
                    m_active_orders.at(order.order_id).status = OrderStatus::REJECTED;
                 }
            }
            send_exec_report(order, OrderStatus::REJECTED, reason);
            return false; // Order rejected by risk
        }
        std::cout << "Order PASSED RiskManager checks: OID=" << order.order_id << std::endl;
    } else {
        std::cout << "WARNING: No RiskManager configured. Order proceeding without risk checks: OID=" << order.order_id << std::endl;
    }

    process_order(order);

    return true;
}

void OrderManager::process_order(Order order_copy) { // Process a copy
    // Simulate fill logic
    double simulated_fill_price = 0.0;
    long long filled_this_event = 0;
    OrderStatus final_status = order_copy.status; // Start with current status

    // --- Simulate Exchange Interaction / Fill ---
    // This is highly simplified. A real matching engine or exchange link would be here.
    // Also, risk checks would occur before this.

    if (order_copy.type == OrderType::MARKET) {
        // For market orders, try to get a price from market data source if available
        // This part is a placeholder as IMarketDataSource doesn't have a direct price query method.
        // We'd typically need the last trade price or current bid/ask from the Tick data.
        // For simplicity, using hardcoded or slightly randomized prices.
        if (order_copy.instrument_id == "AAPL") {
            simulated_fill_price = (order_copy.side == OrderSide::BUY) ? 150.10 : 149.90;
        } else if (order_copy.instrument_id == "SPY") {
            simulated_fill_price = (order_copy.side == OrderSide::BUY) ? 500.10 : 499.90;
        } else {
            simulated_fill_price = (order_copy.side == OrderSide::BUY) ? 101.0 : 99.0; // Default
        }
        filled_this_event = order_copy.quantity; // Assume full fill for market order
        final_status = OrderStatus::FILLED;
        std::cout << "Market order " << order_copy.order_id << " for " << order_copy.instrument_id
                  << " filled at (simulated) " << simulated_fill_price << std::endl;

    } else if (order_copy.type == OrderType::LIMIT) {
        // Simple limit order fill simulation:
        // Assume aggressive limit orders fill immediately at their limit price.
        // A real system would place it on a book or match against existing orders.
        // Example: If BUY limit price is >= current ask, or SELL limit price <= current bid.
        // For now, just assume it fills if price is "reasonable" (e.g. not zero).
        if (order_copy.price > 0) {
            simulated_fill_price = order_copy.price;
            filled_this_event = order_copy.quantity; // Assume full fill
            final_status = OrderStatus::FILLED;
            std::cout << "Limit order " << order_copy.order_id << " for " << order_copy.instrument_id
                      << " filled at limit price " << simulated_fill_price << std::endl;
        } else {
            final_status = OrderStatus::REJECTED; // Or just ACKNOWLEDGED if it's on book
            std::cout << "Limit order " << order_copy.order_id << " for " << order_copy.instrument_id
                      << " REJECTED (or would be booked - simplified)" << std::endl;
            send_exec_report(order_copy, final_status, "Invalid limit price or not aggressive enough (simulated)");
            return; // No further processing for this rejected/booked order
        }
    }

    // Update order state in the main map
    ExecutionReport report_for_position_update; // Create a temp report for update_position
    {
        std::lock_guard<std::mutex> lock(m_orders_mutex);
        if (m_active_orders.count(order_copy.order_id)) { // count with uint64_t key
            Order& original_order_ref = m_active_orders.at(order_copy.order_id); // .at() with uint64_t key
            original_order_ref.status = final_status;
            original_order_ref.filled_quantity += filled_this_event; // Increment cumulative
            // Average filled price would need to be calculated if multiple partial fills

            // Prepare data for the execution report based on the original order's possibly updated state
            order_copy = original_order_ref; // Refresh order_copy with latest state for the report

            // For the report about THIS event
            report_for_position_update.order_id = original_order_ref.order_id;
            report_for_position_update.client_order_id = original_order_ref.client_order_id;
            report_for_position_update.instrument_id = original_order_ref.instrument_id;
            report_for_position_update.status = final_status;
            report_for_position_update.filled_quantity = filled_this_event; // Qty of THIS fill
            report_for_position_update.filled_price = simulated_fill_price; // Px of THIS fill
            report_for_position_update.cumulative_filled_quantity = original_order_ref.filled_quantity;
            if (original_order_ref.filled_quantity > 0) { // Avoid division by zero
                 // Simple average for now, assumes all fills at same price if only one event
                report_for_position_update.average_filled_price =
                    ((original_order_ref.filled_quantity - filled_this_event) * order_copy.price + // Assuming order_copy.price was old avg_px placeholder
                     filled_this_event * simulated_fill_price) / original_order_ref.filled_quantity;
                 // A more robust avg_price would be stored and updated in the Order object itself.
            } else {
                report_for_position_update.average_filled_price = 0;
            }


        } else {
            std::cerr << "Error: Order " << order_copy.order_id << " not found during processing." << std::endl;
            return; // Order disappeared?
        }
    }

    // Send the execution report for this event
    // order_copy here still has its original order_id which is uint64_t
    send_exec_report(order_copy, final_status, "", simulated_fill_price, filled_this_event);


    // Update position if there was a fill
    if (filled_this_event > 0) {
        // Update RiskManager's state based on the fill
        if (m_risk_manager) {
            // report_for_position_update contains details of THIS fill event
            m_risk_manager->update_on_fill(report_for_position_update, order_copy.side);
        }
        update_position(report_for_position_update);
    }
}


void OrderManager::send_exec_report(const Order& order_state_for_report, OrderStatus report_status, const std::string& reject_reason, double last_filled_price, long long last_filled_qty) {
    if (!m_exec_report_callback) return;

    ExecutionReport report;
    report.order_id = order_state_for_report.order_id; // This is uint64_t
    report.client_order_id = order_state_for_report.client_order_id;
    report.instrument_id = order_state_for_report.instrument_id;
    report.status = report_status;

    report.filled_quantity = last_filled_qty; // Quantity from THIS fill event
    report.filled_price = last_filled_price;   // Price from THIS fill event

    report.cumulative_filled_quantity = order_state_for_report.filled_quantity; // Total filled for the order
    // Calculate average filled price (simplified, assumes Order object has it or can derive)
    if (order_state_for_report.filled_quantity > 0) {
         // This should ideally be calculated and stored more robustly with the Order object over time
        report.average_filled_price = ((order_state_for_report.filled_quantity - last_filled_qty) * order_state_for_report.price + // old avg placeholder
                                      last_filled_qty * last_filled_price) / order_state_for_report.filled_quantity;
        if (last_filled_qty == order_state_for_report.filled_quantity) { // If this is the first fill
             report.average_filled_price = last_filled_price;
        }
    } else {
        report.average_filled_price = 0;
    }


    report.timestamp = std::chrono::system_clock::now();
    report.reject_reason = reject_reason;

    m_exec_report_callback(report);
    std::cout << "ExecReport sent: OID=" << report.order_id << " Status=" << static_cast<int>(report.status)
              << " FilledQty=" << report.filled_quantity << " FilledPx=" << report.filled_price
              << " CumQty=" << report.cumulative_filled_quantity << " AvgPx=" << report.average_filled_price
              << " Reason='" << report.reject_reason << "'" << std::endl;
}


bool OrderManager::cancel_order(const std::string& order_id_str, const std::string& client_order_id) {
    std::lock_guard<std::mutex> lock(m_orders_mutex);

    uint64_t internal_order_id_to_cancel = 0;
    bool found_by_internal_id = false;

    if (!order_id_str.empty()) {
        try {
            internal_order_id_to_cancel = std::stoull(order_id_str);
            found_by_internal_id = true;
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not convert order_id_str '" << order_id_str << "' to uint64_t for cancellation: " << e.what() << std::endl;
            // Proceed to check client_order_id if provided
        }
    }

    auto it = m_active_orders.end();
    if (found_by_internal_id) {
        it = m_active_orders.find(internal_order_id_to_cancel);
    }

    // If not found by internal_id (or if internal_id_str was invalid/empty) and client_order_id is provided, search by client_order_id
    if (it == m_active_orders.end() && !client_order_id.empty()) {
        for (auto const& [map_key_oid, order_val] : m_active_orders) { // map_key_oid is uint64_t
            if (order_val.client_order_id == client_order_id) {
                it = m_active_orders.find(map_key_oid); // use map_key_oid (uint64_t)
                break;
            }
        }
    }

    if (it != m_active_orders.end()) {
        Order& order_to_cancel = it->second;
        // Check if order is in a cancelable state (e.g., NEW, ACKNOWLEDGED, PARTIALLY_FILLED)
        // For this simplified version, we'll allow cancel if not fully FILLED or already REJECTED/CANCELLED
        if (order_to_cancel.status != OrderStatus::FILLED &&
            order_to_cancel.status != OrderStatus::REJECTED &&
            order_to_cancel.status != OrderStatus::CANCELLED) {

            order_to_cancel.status = OrderStatus::CANCELLED;
            order_to_cancel.timestamp = std::chrono::system_clock::now();

            std::cout << "Order cancelled: OID=" << order_to_cancel.order_id
                      << " ClOID=" << order_to_cancel.client_order_id << std::endl;
            send_exec_report(order_to_cancel, OrderStatus::CANCELLED);
            return true;
        } else {
            std::cout << "Order OID=" << order_to_cancel.order_id << " not in cancelable state. Status: "
                      << static_cast<int>(order_to_cancel.status) << std::endl;
            // Optionally send a "CANCEL_REJECTED" execution report
            // send_exec_report(order_to_cancel, OrderStatus::REJECTED, "Cancel rejected: Order not in cancelable state");
            return false;
        }
    }
    std::cout << "Cancel request failed: Order not found with OID=" << order_id_str
              << " or ClOID=" << client_order_id << std::endl;
    // send_exec_report with status REJECTED if an order definition is needed for it.
    // For now, just return false.
    return false;
}

void OrderManager::update_position(const ExecutionReport& report) {
    std::lock_guard<std::mutex> lock(m_positions_mutex);

    if (report.filled_quantity == 0) return; // No change to position

    Position& pos = m_positions[report.instrument_id]; // Creates if not exists
    if (pos.instrument_id.empty()) {
        pos.instrument_id = report.instrument_id;
    }

    long long old_qty = pos.quantity;
    double old_avg_price = pos.average_entry_price;

    // Determine side of the fill for position update
    // This needs the original order's side. The report itself doesn't have it.
    // For simplicity, assume we can deduce side or the report would include it.
    // Here, we'll infer: if new total quantity has greater magnitude, it's an entry/add.
    // If lesser magnitude, it's a close/reduce. This is imperfect without order side.
    // Let's assume the ExecutionReport would contain the OrderSide of the trade.
    // For now, a placeholder: if report.filled_quantity > 0, it's a buy, else sell.
    // This is wrong. We need the original order's side.
    // We should fetch the order from m_active_orders to get its side. (report.order_id is uint64_t)

    OrderSide trade_side;
    {
        std::lock_guard<std::mutex> order_lock(m_orders_mutex); // Need to lock orders briefly
        if(m_active_orders.count(report.order_id)) { // count with uint64_t key
            trade_side = m_active_orders.at(report.order_id).side; // .at() with uint64_t key
        } else {
            std::cerr << "Position update error: Original order " << report.order_id << " not found." << std::endl;
            return;
        }
    }


    if (trade_side == OrderSide::BUY) {
        // If old position was short, calculate realized PnL
        if (old_qty < 0) {
            long long closed_qty = std::min(report.filled_quantity, -old_qty);
            pos.realized_pnl += (old_avg_price - report.filled_price) * closed_qty;
        }
        pos.average_entry_price = (old_avg_price * old_qty + report.filled_price * report.filled_quantity) / (old_qty + report.filled_quantity);
        pos.quantity += report.filled_quantity;
    } else { // OrderSide::SELL
        // If old position was long, calculate realized PnL
        if (old_qty > 0) {
            long long closed_qty = std::min(report.filled_quantity, old_qty);
            pos.realized_pnl += (report.filled_price - old_avg_price) * closed_qty;
        }
         pos.average_entry_price = (old_avg_price * old_qty - report.filled_price * report.filled_quantity) / (old_qty - report.filled_quantity);
        pos.quantity -= report.filled_quantity;
    }

    // Ensure avg price is 0 if position is flat
    if (pos.quantity == 0) {
        pos.average_entry_price = 0.0;
    }


    std::cout << "Position Updated: Instr=" << pos.instrument_id
              << " Qty=" << pos.quantity
              << " AvgPx=" << pos.average_entry_price
              << " RealizedPnL=" << pos.realized_pnl << std::endl;
}


} // namespace oms
} // namespace hpts
