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

# SQLite database (C++ wrapper)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_INTERNAL_SQLITE ON CACHE BOOL "" FORCE)  # bundles sqlite3 itself, no system install needed

FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1
)
FetchContent_MakeAvailable(SQLiteCpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SQLiteCpp PUBLIC -include cstdint)
endif()

add_executable(CarRentalSystem main.cpp Database.cpp)

# Link to the Crow Library
target_link_libraries(CarRentalSystem PRIVATE Crow::Crow SQLiteCpp)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

