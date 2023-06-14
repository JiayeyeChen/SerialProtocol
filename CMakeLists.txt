cmake_minimum_required(VERSION 3.0.2)
project(serial-manager)

find_package(Boost REQUIRED COMPONENTS system thread regex)

include_directories(
    Inc
    ${Boost_INCLUDE_DIR}
)

set(SRC
    ${CMAKE_CURRENT_LIST_DIR}/Src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Src/crc16_modbus.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Src/serial_protocol_linux.cpp
)

set(LIBS
    ${Boost_LIBRARIES}
)

add_executable(serial-manager ${SRC})
target_link_libraries(serial-manager ${LIBS})
