#include <iostream>
#include <memory>
#include "Trader.hpp"

int main(){
	Trader trader = Trader();
	trader.Run();
        std::cout << "Exiting...\n";
} 
