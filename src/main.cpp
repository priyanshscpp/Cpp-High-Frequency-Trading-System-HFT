#include <iostream>
#include <memory>
#include <thread> // For std::this_thread::sleep_for

#include "Trader.hpp"
#include <iostream> // Already present, but good to ensure
#include <thread>   // Already present
#include <chrono>   // For std::chrono::system_clock::now()

#include "market_data/MockMarketDataSource.hpp"
#include "oms/OrderManager.hpp"
#include "oms/OrderTypes.hpp"
#include "risk_management/RiskManager.hpp"
#include "risk_management/RiskRules.hpp"
#include "strategies/MeanReversionStrategy.hpp"
#include "strategies/MomentumStrategy.hpp"      // Added
#include <vector>
#include <memory>

int main(int argc, char* argv[]) {
  (void)argc; // Mark as unused
  (void)argv; // Mark as unused
  // Original Trader logic (can be commented out or run alongside)
  /*
  Trader trader = Trader();
  trader.Run();
  std::cout << "Exiting Trader part...\n";
  */

  std::cout << "--- HPTS Application Start ---" << std::endl;

  // 1. Initialize Market Data Source
  hpts::market_data::MockMarketDataSource mock_mds(2.0); // Tick rate 2 Hz for faster ticks
  mock_mds.set_market_data_callback(
      [](const hpts::interfaces::Tick& /*tick*/) { // Mark tick as unused
        // Optional: print ticks if needed for debugging OM interaction with market prices
        // std::cout << "MDS Tick: " << tick.instrument_id << " Px: " << tick.price << " Bid: " << tick.bid_price << " Ask: " << tick.ask_price << std::endl;
      });
  mock_mds.subscribe("AAPL");
  mock_mds.subscribe("SPY");
  std::cout << "[Main] Starting Market Data Source..." << std::endl;
  mock_mds.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give MDS a moment to start

  // 2. Initialize Order Manager
  std::cout << "[Main] Initializing Risk Manager..." << std::endl;
  hpts::risk::RiskManager risk_manager;
  hpts::risk::RiskConfig config;
  config.max_order_size = 1000;
  config.max_open_contracts_per_instrument = 500;
  config.max_daily_volume_per_instrument = 2000;
  config.allowed_instruments = {"AAPL", "SPY", "MSFT"}; // Example whitelist
  // config.max_total_contracts_across_all_instruments = 1000; // Deferring this complex one for now
  risk_manager.load_configuration(config);

  std::cout << "[Main] Initializing Order Manager..." << std::endl;
  hpts::oms::OrderManager order_manager(&mock_mds, &risk_manager);

  // Strategy Container
  std::vector<std::unique_ptr<hpts::interfaces::IStrategy>> strategies;

  // Create Strategies
  auto mean_rev_aapl = std::make_unique<hpts::strategies::MeanReversionStrategy>(
      "MeanRevAAPL", "AAPL", 20, 2.0, 10);
  auto momentum_spy = std::make_unique<hpts::strategies::MomentumStrategy>(
      "MomentumSPY", "SPY", 10, 30, 5); // name, instrument, short_win, long_win, qty

  // Initialize strategies
  mean_rev_aapl->init(&order_manager, &mock_mds);
  momentum_spy->init(&order_manager, &mock_mds);

  strategies.push_back(std::move(mean_rev_aapl));
  strategies.push_back(std::move(momentum_spy));

  // Modify MarketDataCallback to dispatch to strategies
  mock_mds.set_market_data_callback(
      [&](const hpts::interfaces::Tick& tick) {
        // Optional: print raw ticks
        // std::cout << "MDS Tick Main: " << tick.instrument_id << " Px: " << tick.price << std::endl;
        for (auto& strategy : strategies) {
            strategy->on_market_data(tick);
        }
    });

  // Modify ExecutionReportCallback to dispatch to strategies
  order_manager.set_execution_report_callback(
      [&](const hpts::oms::ExecutionReport& report) {
        std::cout << "[Main ExecReport] OID=" << report.order_id << ", ClOID=" << report.client_order_id
                  << ", Status=" << static_cast<int>(report.status) << " FilledQty=" << report.filled_quantity
                  << " AvgPx=" << report.average_filled_price
                  << " LastPx=" << report.filled_price << " LastQty=" << report.filled_quantity
                  << " CumQty=" << report.cumulative_filled_quantity
                  << " Reason='" << report.reject_reason << "'"
                  << std::endl;
        for (auto& strategy : strategies) {
            strategy->on_execution_report(report);
        }
    });

  // Start strategies
  for (auto& strategy : strategies) {
      strategy->start(); // This will also make them subscribe to market data
  }

  std::cout << "[Main] Market Data Source already started. Strategies are running." << std::endl;
  // mock_mds.start(); // MDS is already started before OM init. Ensure it is.

  // 3. Test orders from previous step can be removed or commented out
  //    as strategies will now be sending orders.
  /*
  std::cout << "[Main] Sending manual test orders..." << std::endl;

  // Send a Market Order for AAPL (Good)
  hpts::oms::Order good_order; // Keep one for basic OM test if needed
  good_order.client_order_id = "Manual_Good_001";
  good_order.instrument_id = "SPY"; // Changed to SPY to not interfere with AAPL strategy too much
  good_order.side = hpts::oms::OrderSide::BUY;
  good_order.type = hpts::oms::OrderType::MARKET;
  good_order.quantity = 50; // Within max_order_size, max_open_contracts
  order_manager.send_order(good_order);
  std::this_thread::sleep_for(std::chrono::milliseconds(200)); // allow processing

  // Order too large
  hpts::oms::Order large_order;
  large_order.client_order_id = "RiskTest_Large_002";
  large_order.instrument_id = "AAPL";
  large_order.side = hpts::oms::OrderSide::BUY;
  large_order.type = hpts::oms::OrderType::MARKET;
  large_order.quantity = 1500; // Exceeds max_order_size = 1000
  order_manager.send_order(large_order);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Order for disallowed instrument
  hpts::oms::Order disallowed_instr_order;
  disallowed_instr_order.client_order_id = "RiskTest_Disallowed_003";
  disallowed_instr_order.instrument_id = "GOOG"; // Not in allowed_instruments
  disallowed_instr_order.side = hpts::oms::OrderSide::BUY;
  disallowed_instr_order.type = hpts::oms::OrderType::MARKET;
  disallowed_instr_order.quantity = 10;
  order_manager.send_order(disallowed_instr_order);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Orders that breach position limit for MSFT (max_open_contracts_per_instrument = 500)
  hpts::oms::Order pos_limit_order1;
  pos_limit_order1.client_order_id = "RiskTest_PosLimit_004a";
  pos_limit_order1.instrument_id = "MSFT"; // Allowed
  pos_limit_order1.side = hpts::oms::OrderSide::BUY;
  pos_limit_order1.type = hpts::oms::OrderType::MARKET;
  pos_limit_order1.quantity = 300;
  order_manager.send_order(pos_limit_order1);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  hpts::oms::Order pos_limit_order2;
  pos_limit_order2.client_order_id = "RiskTest_PosLimit_004b";
  pos_limit_order2.instrument_id = "MSFT";
  pos_limit_order2.side = hpts::oms::OrderSide::BUY;
  pos_limit_order2.type = hpts::oms::OrderType::MARKET;
  pos_limit_order2.quantity = 300; // This should push 300 (existing) + 300 = 600 over the 500 limit
  order_manager.send_order(pos_limit_order2);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Order that should be fine after some volume already traded for MSFT
  hpts::oms::Order msft_order_after_some_vol;
  msft_order_after_some_vol.client_order_id = "RiskTest_MSFT_Vol_005";
  msft_order_after_some_vol.instrument_id = "MSFT";
  msft_order_after_some_vol.side = hpts::oms::OrderSide::SELL; // Sell some back
  msft_order_after_some_vol.type = hpts::oms::OrderType::MARKET;
  msft_order_after_some_vol.quantity = 100;
  order_manager.send_order(msft_order_after_some_vol);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  */

  // 4. Let strategies run
  std::cout << "[Main] Strategies running for 50 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(50)); // Let it run for a while

  // 5. Stop strategies and services
  std::cout << "[Main] Stopping strategies..." << std::endl;
  for (auto& strategy : strategies) {
      strategy->stop();
  }
  std::cout << "[Main] Stopping Market Data Source..." << std::endl;
  mock_mds.stop();

  std::cout << "[Main] Program finished." << std::endl;
  return 0;
}
