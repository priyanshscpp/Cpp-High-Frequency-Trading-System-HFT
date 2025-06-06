cmake_minimum_required(VERSION 3.15)
project(HighPerfTradingSystem VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Compiler Flags ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
    # Add sanitizer options
    option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
    option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
    if(ENABLE_ASAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    endif()
    if(ENABLE_UBSAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    # MSVC specific flags, e.g. /MP for parallel builds
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# --- Include Directories ---
include_directories(src) # Add src as a top-level include directory
include_directories(src/interfaces)
include_directories(src/market_data)
include_directories(src/oms)
include_directories(src/risk_management)
include_directories(src/strategies)      # Added

# --- Subdirectories ---
# Placeholder for adding src subdirectory if we create a CMakeLists.txt there
# add_subdirectory(src) # We will manage targets directly for now

# --- Source Files ---
set(MAIN_APP_SOURCES
    src/main.cpp
    src/Api.cpp
    src/Socket.cpp
    src/Trader.cpp
    src/Boost_WebSocket/BSocket.cpp
    src/Custom_WebSocket/CParser.cpp
    src/Custom_WebSocket/CSocket.cpp
    src/WebSocketpp/Socketpp.cpp
    src/market_data/MockMarketDataSource.cpp
    src/oms/OrderManager.cpp
    src/risk_management/RiskManager.cpp
    src/strategies/MeanReversionStrategy.cpp
    src/strategies/MomentumStrategy.cpp # Added new source
)

add_executable(hpts_main ${MAIN_APP_SOURCES})

# --- Dependencies ---
# Boost (Asio, Beast for WebSocket)
# On Linux: sudo apt-get install libboost-all-dev
# On macOS: brew install boost
# On Windows: vcpkg install boost-asio boost-beast
find_package(Boost REQUIRED COMPONENTS system thread) # Add other components as needed
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(hpts_main PRIVATE Boost::system Boost::thread)
    # For Boost.Asio and Beast, they are often header-only or linked via other Boost libs
    # For now, we assume headers are found via Boost_INCLUDE_DIRS.
    # If specific linking is needed for Asio/Beast, it would be added here.
else()
    message(FATAL_ERROR "Boost not found. Please install Boost.")
endif()

        # nlohmann_json
        find_package(nlohmann_json 3.2 REQUIRED) # Check for version 3.2 or higher
        if(nlohmann_json_FOUND)
            target_link_libraries(hpts_main PRIVATE nlohmann_json::nlohmann_json)
            # For header-only, include_directories might also be needed if not handled by target_link_libraries
            # include_directories(${nlohmann_json_INCLUDE_DIRS}) # Often not needed for modern CMake targets
            message(STATUS "nlohmann_json found and linked.")
        else()
            message(FATAL_ERROR "nlohmann_json not found. Please install it.")
        endif()

        # WebSocket++
        find_package(websocketpp REQUIRED)
        if(websocketpp_FOUND)
            # websocketpp is often header-only but its CMake config might provide an INTERFACE library
            # target_link_libraries(hpts_main PRIVATE websocketpp::websocketpp)
            # For header-only, ensuring includes are available is key.
            # If it provides a target, linking is good for propagating usage requirements.
            # If not, include_directories(${WEBSOCKETPP_INCLUDE_DIR}) might be needed if not already covered.
            # The package usually sets up its include directory correctly for targets.
            # Let's assume it provides an INTERFACE target or sets include path.
            # If it's header-only and doesn't have a proper CMake target,
            # just ensuring its headers are in an include path (e.g. /usr/include) is enough.
            # The find_package should make its include directories available.
            target_link_libraries(hpts_main PRIVATE websocketpp::websocketpp)
            message(STATUS "WebSocket++ found and linked.")
        else()
            message(FATAL_ERROR "WebSocket++ not found. Please install libwebsocketpp-dev.")
        endif()

        # OpenSSL
        find_package(OpenSSL REQUIRED)
        if(OpenSSL_FOUND)
            target_link_libraries(hpts_main PRIVATE OpenSSL::SSL OpenSSL::Crypto)
            message(STATUS "OpenSSL found and linked.")
        else()
            message(FATAL_ERROR "OpenSSL not found. Please install libssl-dev.")
        endif()

# --- Testing (Google Test) ---
option(BUILD_TESTING "Build tests" ON) # Default to ON
if(BUILD_TESTING)
    enable_testing()
    # On Linux: sudo apt-get install libgtest-dev googletest
    #           cd /usr/src/googletest; sudo cmake .; sudo make; sudo cp lib/*.a /usr/lib/
    # On macOS: brew install googletest
    # On Windows: vcpkg install gtest
    find_package(GTest REQUIRED)
    if(GTest_FOUND)
        include_directories(${GTEST_INCLUDE_DIRS})

        # Example of adding a test executable
        # file(GLOB TEST_SOURCES "test/*.cpp" "test/test_latency/*.cpp") # Adjust glob as needed
        # add_executable(hpts_tests ${TEST_SOURCES})
        # target_link_libraries(hpts_tests PRIVATE GTest::GTest GTest::Main hpts_main) # Link tests against main lib/exe if needed
        # add_test(NAME UnitTests COMMAND hpts_tests)
        message(STATUS "Google Test found. Test configuration will be added in a later step.")
    else()
        message(WARNING "Google Test not found. Tests will not be built.")
    endif()
endif()

# --- Installation ---
# Basic installation for the executable
install(TARGETS hpts_main DESTINATION bin)

# --- Generate compile_commands.json for tooling ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "CMake configuration completed. Project: ${PROJECT_NAME}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}") # Informative, though not explicitly set here
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Boost found: ${Boost_FOUND} at ${Boost_INCLUDE_DIRS}")

# Print final CXX flags for verification
# get_target_property(TARGET_CXX_FLAGS hpts_main COMPILE_OPTIONS)
# message(STATUS "hpts_main CXX flags: ${TARGET_CXX_FLAGS}")
# The above doesn't work as expected, use CMAKE_CXX_FLAGS for global flags
message(STATUS "Global CXX flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Global Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")
