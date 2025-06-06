#pragma once

#include <string>
#include <vector>
#include <set>
#include <cstdint> // For long long

namespace hpts {
namespace risk {

struct RiskConfig {
    long long max_order_size = 1000000; // Default large limit
    long long max_open_contracts_per_instrument = 5000; // Max net open position for any single instrument
    long long max_total_contracts_across_all_instruments = 20000; // Max net open position summed across all instruments
    long long max_daily_volume_per_instrument = 100000; // Max total traded volume (buy + sell) for an instrument

    std::set<std::string> allowed_instruments; // If empty, all instruments are allowed. Otherwise, a whitelist.

    // Default constructor is fine for now, members are initialized.
    // Could add a constructor to load from a file or specific settings.
};

// Internal state tracked by RiskManager per instrument
struct InstrumentRiskState {
    long long net_position = 0;
    long long daily_traded_volume = 0;
};

} // namespace risk
} // namespace hpts
