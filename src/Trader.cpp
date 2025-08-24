#include "Trader.hpp"

int Trader::show_resp(const std::string& resp){
	nlohmann::json obj = nlohmann::json::parse(resp);

	if(obj.contains("error")){
		std::cout << "Error encounterd\n";
		std::cout << obj["error"].dump(4) << '\n';
		return 1;
	}

	std::cout << obj.dump(4) << '\n';
	return 0;
}

// Place Order Menu
int Trader::handlePlaceOrder(const std::function<std::pair<int, std::string>(std::string, double, int)>& action) {
	try {
		std::string ins; double price; int quantity;
		std::cout << "Enter Instrument name:(eg BTC-PERPETUAL, ETH-PERPETUAL) ";
		std::cin >> ins;
		std::cout << "Enter Price: ";
		std::cin >> price;
        	std::cout << "Enter Quantity: ";
		std::cin >> quantity;
		std::pair<int, std::string> result = action(ins, price, quantity);
		int status = result.first;
		std::string resp = result.second;
		int valid = show_resp(resp);
		return status | valid;
	}
	catch (const std::exception& e) {
                std::cout << "Error\n" << e.what() << '\n';
		return 1;
        }
}

int Trader::handleCancelOrder(const std::function<std::pair<int, std::string>(std::string)>& action) {
	try {
		std::string order_id;
		std::cout << "Enter OrderId: ";
		std::cin >> order_id;
		std::pair<int, std::string> result = action(order_id);
		int status = result.first;
		std::string resp = result.second;
		int valid = show_resp(resp);
		return status | valid;
	} 
	catch (const std::exception& e) {
		std::cout << "Error\n" << e.what() << '\n';
		return 1;
	}
}

int Trader::handleModifyOrder(const std::function<std::pair<int, std::string> (std::string, double, int)>& action){
	try {
		std::string order_id; int quantity; double price;
		std::cout << "Enter OrderId: ";
		std::cin >> order_id;
		std::cout << "Enter New Price: ";
		std::cin >> price;
		std::cout << "Enter New quantity: ";
		std::cin >> quantity;
		std::pair<int, std::string> result = action(order_id, price, quantity);
		int status = result.first;
		std::string resp = result.second;
		int valid = show_resp(resp);
		return status | valid;
	} 
	catch (const std::exception& e) {
		std::cout << "Error\n" << e.what() << '\n';
		return 1;
	}
}

int Trader::handleViewPosition(const std::function<std::pair<int, std::string> (std::string)>& action){
	try{
		std::string inst;
		std::cout << "Enter Instrument name:(eg ETH-PERPETUAL) ";
		std::cin >> inst;
		std::pair<int, std::string> result = action(inst);
		int status = result.first;
		std::string resp = result.second;
		int valid = show_resp(resp);
		return status | valid;
	}
	catch (const std::exception& e){
		std::cout << "Error\n" << e.what() << '\n';
		return 1;
	}
}

int Trader::handleGetOrderBook(const std::function<std::pair<int, std::string> (std::string, int)>& action){
	try{
		std::string inst; int depth;
		std::cout << "Enter Instrument name:(eg BTC-PERPETUAL) ";
		std::cin >> inst;
		std::cout << "Depth: ";
		std::cin >> depth;
		std::pair<int, std::string> result = action(inst, depth);
		int status = result.first;
		std::string resp = result.second;
		int valid = show_resp(resp);
		return status | valid;
	}
	catch (const std::exception& e){
		std::cout << "Error\n" << e.what() << '\n';
		return 1;
	}
}

int Trader::handleOpenOrders(const std::function<std::pair<int, std::string> (std::string)>& action){
	try{
		std::string kind;
		std::cout << "Kind:(eg. null, future, option) ";
		std::cin >> kind;
		std::pair<int, std::string> result = action(kind);
		int status = result.first;
		std::string resp = result.second;

		int valid = show_resp(resp);
		return status | valid;
	}
	catch (const std::exception& e){
		std::cout << "Error\n" << e.what() << '\n';
		return 1;
	}
}

// Market Stream Menu
int Trader::handleMarketData(const std::function<std::pair<int, std::string>(std::string, int)>& action) {
	try {
		std::cout << "[1] Subscribe to symbol\n";
		std::cout << "[2] Unsubscribe to symbol\n";
		std::cout << "[3] Orderbook of symbols\n";
		std::cout << "[4] Back\n";
		int opt; std::cin >> opt;
		if(opt > 4){
			throw "Invalid Input\n";
		}
		
		if(opt == 1){
			std::string channel;
			std::cout << "Enter channel to subscribe: (eg. announcements, platform_state, book.ETH-PERPETUAL.100ms) ";
			std::cin >> channel;
			std::pair<int, std::string> result = action(channel, 1);
			int status = result.first;
			std::string resp = result.second;
			int valid = show_resp(resp);
			return status | valid;
		} else if(opt == 2){
			std::string channel;
			std::cout << "Enter channel to unsubscribe: (eg. announcements, platform_state, user.lock) ";
			std::cin >> channel;
			std::pair<int, std::string> result = action(channel, 2);
			int status = result.first;
			std::string resp = result.second;
			int valid = show_resp(resp);
			return status | valid;
		} else if(opt == 3){
			int count;
			std::cout << "Enter number of announcements [1, 50]: (eg. 24) ";
			std::cin >> count;
			std::pair<int, std::string> result = action(std::to_string(count), 3);
			int status = result.first;
			std::string resp = result.second;
			int valid = show_resp(resp);
			return status | valid;
		} else{
			return 1;
		}		
	}
	catch (const std::exception& e) {
                std::cout << "Error\n" << e.what() << '\n';
		return 1;
        }
}

// Constructor
Trader::Trader(){
	std::cout << "New Api Instance Created\n";
	m_api = new Api();
}

// Function responsible for placing order
std::pair<int, std::string> Trader::place_order(const std::string &inst, double price, int quant){
	std::ostringstream payload;
    	payload << R"({
        	"jsonrpc": "2.0",
        	"method": "private/buy",
        	"params": {
            		"instrument_name": ")" << inst << R"(",
            		"type": "limit",
            		"price": )" << price << R"(,
            		"amount": )" << quant << R"(
        	},
        	"id": "1"
    	})";

    	std::string payload_str = payload.str();
	return m_api -> api_private(payload_str);
}

// Function responsible for canclling order
std::pair<int, std::string> Trader::cancel_order(const std::string &order_id){
	std::ostringstream payload;
    	payload << R"({
        	"jsonrpc": "2.0",
        	"method": "private/cancel",
        	"params": {
            		"order_id": ")" << order_id << R"("
        	},
        	"id": "2"
    	})";

    	std::string payload_str = payload.str();
	return m_api -> api_private(payload_str);
}

// Function responsible for modifing order
std::pair<int, std::string> Trader::modify_order(const std::string &order_id, double price, int quant){
	std::ostringstream payload;
    	payload << R"({
        	"jsonrpc": "2.0",
        	"method": "private/edit",
        	"params": {
            		"order_id": ")" << order_id << R"(",
            		"price": )" << price << R"(,
            		"amount": )" << quant << R"(
        	},
        	"id": "3"
    	})";

    	std::string payload_str = payload.str();
	return m_api -> api_private(payload_str);
}

// Function responsible for fetching order book
std::pair<int, std::string> Trader::get_orderbook(const std::string &inst, int depth){
	std::ostringstream payload;
    	payload << R"({
        	"jsonrpc": "2.0",
        	"method": "public/get_order_book",
        	"params": {
            		"instrument_name": ")" << inst << R"(",
			"depth": )" << depth << R"(
        	},
        	"id": "4"
    	})";

    	std::string payload_str = payload.str();
	return m_api -> api_public(payload_str);
}

// Function to View our current position
std::pair<int, std::string> Trader::view_position(const std::string &inst){
	std::ostringstream payload;
    	payload << R"({
        	"jsonrpc": "2.0",
        	"method": "private/get_position",
        	"params": {
            		"instrument_name": ")" << inst << R"("
        	},
        	"id": "5"
    	})";

    	std::string payload_str = payload.str();
	return m_api -> api_private(payload_str);
}

// Function to View open orders
std::pair<int, std::string> Trader::get_openorders(const std::string &kind){
	std::ostringstream payload;
	if(kind == "null") {
		payload << R"({
        		"jsonrpc": "2.0",
        		"method": "private/get_open_orders",
        		"params": {
        		},
        		"id": "6"
    		})";
	} else{
    		payload << R"({
        		"jsonrpc": "2.0",
        		"method": "private/get_open_orders",
        		"params": {
            			"kind": ")" << kind << R"(",
				"type": "all"
        		},
        		"id": "6"
    		})";
	}

    	std::string payload_str = payload.str();
	return m_api -> api_private(payload_str);
}

// Function to View Market data
std::pair<int, std::string> Trader::get_marketdata(const std::string &channel, int type){
	std::ostringstream payload;
	if(type == 1){
    		payload << R"({
        		"jsonrpc": "2.0",
        		"method": "private/subscribe",
        		"params": {
            			"channels": [")" << channel << R"("]
        		},
        		"id": "7"
    		})";
	} else if(type == 2) {
		payload << R"({
        		"jsonrpc": "2.0",
        		"method": "private/unsubscribe",
        		"params": {
            			"channels": [")" << channel << R"("]
        		},
        		"id": "8"
    		})";
	} else{
		payload << R"({
        		"jsonrpc": "2.0",
        		"method": "public/get_announcements",
        		"params": {
            			"count": )" << std::stoi(channel) << R"(
        		},
        		"id": "9"
    		})";
	}
    	std::string payload_str = payload.str();
	return m_api -> api_private(payload_str);
}

// The holy run loop!
void Trader::Run() {
	std::cout << "Trader Running ...\n";
	bool run = true;
    	while (run) {
        	std::cout << "Select an Option:\n";
        	std::cout << "[1] Place Order\n";
        	std::cout << "[2] Cancel Order\n";
        	std::cout << "[3] Modify Order\n";
        	std::cout << "[4] Get OrderBook\n";
        	std::cout << "[5] View Current Positions\n";
        	std::cout << "[6] Market Streaming\n";
		std::cout << "[7] Open Orders\n";
		std::cout << "[8] Exit\n";
        	int inp; std::cin >> inp;

        	switch (inp) {
            		case 1: { // Place order
                		int status = handlePlaceOrder([this](std::string inst, double price, int quant) { 
						return place_order(inst, price, quant); });
                		if(status){
					std::cout << "Place order failed\n";
				} else{
					std::cout << "Order placed successfully\n";
				}
				break;
			}
            		case 2: { // Cancel order
                		int status = handleCancelOrder([this](std::string order_id) { 
					return cancel_order(order_id); });
				if(status){
					std::cout << "Cancel order failed\n";
				} else{
					std::cout << "Order cancelled successfully\n";
				}
                		break;
			}
            		case 3: { // Modify order
                		int status = handleModifyOrder([this](std::string order_id, double price, int quant) { 
					return modify_order(order_id, price, quant); });
				if(status){
					std::cout << "Modify Order Failed\n";
				} else{
					std::cout << "Order modified\n";
				}
                		break;
			}
            		case 4: { // Fetch order book logic here
                		int status = handleGetOrderBook([this](std::string inst, int depth) { 
					return get_orderbook(inst, depth); });
                		if(status){
					std::cout << "Couldn't fetch order book\n";
				} else{
					std::cout << "Fetching complete\n";
				}
				break;
			}
            		case 5: { // Fetch positions logic here
				int status = handleViewPosition([this](std::string inst) { 
					return view_position(inst); });
                		if(status){
					std::cout << "Couldn't view position\n";
				} else{
					std::cout << "Successfully fetched positon\n";
				}
				break;
			}
            		case 6: { // Start WebSocket market data stream here
                		int status = handleMarketData([this](std::string kind, int type) { 
					return get_marketdata(kind, type); });
                		if(status){
					std::cout << "Couldn't view market data\n";
				} else{
					std::cout << "Successfully fetched market data\n";
				}
				break;
			}
			case 7: { // Get Open Orders
				int status = handleOpenOrders([this](std::string kind) { 
					return get_openorders(kind); });
                		if(status){
					std::cout << "Couldn't view open orders\n";
				} else{
					std::cout << "Successfully fetched open orders\n";
				}
				break;
			}
            		case 8: { // Exit
                		run = false;
				break;
			}
            		default: {
                		std::cout << "Invalid Input [Input should be between 1 and 8]\n";
        		}
    		}
	}
}
