cmake_minimum_required(VERSION 3.13)

project(external-devices.ssd1306)

set(HEADER_LIST
    SSD1306.h )

set(SOURCE_LIST
    SSD1306.cpp)

# Add library cpp files
add_library(external-devices.ssd1306 ${HEADER_LIST} ${SOURCE_LIST} ) 

# Enable or disable exceptions based on the platform or configuration
option(USE_EXCEPTIONS "Enable exceptions" ON)

if(USE_EXCEPTIONS)
    target_compile_definitions(external-devices.ssd1306 PUBLIC __EXCEPTIONS)
else()
    target_compile_definitions(external-devices.ssd1306 PUBLIC -fno-exceptions)
endif()

target_sources(external-devices.ssd1306 
    INTERFACE ${HEADER_LIST}
    PUBLIC ${SOURCE_LIST}
)

target_link_libraries(external-devices.ssd1306 abstract-platform.i2c)

# Add include directory
target_include_directories(external-devices.ssd1306 PUBLIC ${CMAKE_CURRENT_LIST_DIR})