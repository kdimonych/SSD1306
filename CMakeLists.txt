cmake_minimum_required(VERSION 3.13)

project(external-devices.ssd1306 C CXX)

set(HEADER_LIST
    ExternalHardware/ssd1306/SSD1306_HAL.hpp
    ExternalHardware/ssd1306/SSD1306.hpp)

set(SOURCE_LIST
    ExternalHardware/ssd1306/SSD1306_HAL.cpp)

# Add library cpp files
add_library(external-devices.ssd1306 ${HEADER_LIST} ${SOURCE_LIST} ) 

target_sources(external-devices.ssd1306 
    INTERFACE ${HEADER_LIST}
    PUBLIC ${SOURCE_LIST}
)

target_link_libraries(external-devices.ssd1306 abstract-platform.common abstract-platform.i2c abstract-platform.output.display)

# Add include directory
target_include_directories(external-devices.ssd1306 PUBLIC ${CMAKE_CURRENT_LIST_DIR})