cmake_minimum_required(VERSION 3.10.0)
project(CarRentalSystem VERSION 0.1.0 LANGUAGES C CXX)

# Crow API
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Crow's Findasio.cmake looks for this exact cache variable.
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "" FORCE)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(Crow)

add_executable(CarRentalSystem main.cpp)

# Link to the Crow Library
target_link_libraries(CarRentalSystem PRIVATE Crow::Crow)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
