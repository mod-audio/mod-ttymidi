cmake_minimum_required(VERSION 3.7.2)
project( ttymidi )

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_BUILD_TYPE debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wshadow -Werror -fvisibility=hidden")

# cmake --help-module-list
# else add modules to this path:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# PThread dependency
find_package(Threads REQUIRED)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

# Jack dependency
find_package(Jack REQUIRED)
include_directories(${JACK_INCLUDE_DIRS})
set(LIBS ${LIBS} ${JACK_LIBRARIES})

set(SOURCES src/ttymidi.c src/mod-semaphore.h)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${LIBS})



##set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -lpthread")

add_library(${PROJECT_NAME}-jack-internal SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME}-jack-internal PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME}-jack-internal PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME}-jack-internal PROPERTY POSITION_INDEPENDENT_CODE ON)


#enable_testing()
# add_test(NAME SimpleExitSuccess COMMAND ${CMAKE_SOURCE_DIR}/test/success.sh)
# add_test(NAME SimpleExitFailure COMMAND ${CMAKE_SOURCE_DIR}/test/fail.sh)
# set_tests_properties(SimpleExitFailure PROPERTIES WILL_FAIL true)
