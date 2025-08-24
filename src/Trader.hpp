#pragma once
#include "utility.hpp"
#include "Api.hpp"
#include <nlohmann/json.hpp>

class Trader{
public:
	// Constructor
	Trader(); 
	// Procedures
	void Run();
	// Return [status, resp] of the transaction
	std::pair<int, std::string> place_order(const std::string&, double, int);
	std::pair<int, std::string> cancel_order(const std::string&);
	std::pair<int, std::string> modify_order(const std::string&, double, int);
	std::pair<int, std::string> get_orderbook(const std::string&, int);
	std::pair<int, std::string> view_position(const std::string&);
	std::pair<int, std::string> get_openorders(const std::string&);
	std::pair<int, std::string> get_marketdata(const std::string&, int);

	// Returns status code for the transaction
	int show_resp(const std::string&);
	int handleCancelOrder(const std::function<std::pair<int, std::string>(std::string)>& action); 
	int handlePlaceOrder(const std::function<std::pair<int, std::string>(std::string, double, int)>& action);
	int handleModifyOrder(const std::function<std::pair<int, std::string> (std::string, double, int)>& action);
	int handleGetOrderBook(const std::function<std::pair<int, std::string> (std::string, int)>& action);
	int handleViewPosition(const std::function<std::pair<int, std::string> (std::string)>& action);
	int handleOpenOrders(const std::function<std::pair<int, std::string> (std::string)>& action);
	int handleMarketData(const std::function<std::pair<int, std::string>(std::string, int)>& action); 
private:
	Api *m_api;
};
