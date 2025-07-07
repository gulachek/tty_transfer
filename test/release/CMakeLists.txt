cmake_minimum_required(VERSION 3.10)
project(TtyTransferTest)

find_package(tty_transfer REQUIRED)

add_executable(test
	test.c
)

target_link_libraries(test PRIVATE tty_transfer)
