#include "utility.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

class CParser{
public:
	// String to Frames
	std::vector<std::vector<uint8_t>> stof(const std::string& str); 
	// Frames to String
	std::string ftos(std::vector<std::vector<uint8_t>>& frames);
};
