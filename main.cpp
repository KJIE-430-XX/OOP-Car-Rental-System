/*
Command for build and test project locally

cmake --build build
.\build\Debug\CarRentalSystem.exe

cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Debug
cmake --build build-linux -j"$(nproc)"
./build-linux/CarRentalSystem

link: http://localhost:18080/
*/

#include "crow.h"
#include "Database.h"
#include <fstream>
#include <sstream>

namespace {
const std::string databasePath = "car_rental.db";

bool hasStringFields(
    const crow::json::rvalue& body,
    const std::initializer_list<const char*>& fields
) {
    for (const char* field : fields) {
        if (!body[field] || body[field].t() != crow::json::type::String) {
            return false;
        }
    }
    return true;
}
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        initializeDatabase(databasePath);
    }
    catch (const std::exception& e) {
        std::cerr << "Database init failed: " << e.what() << std::endl;
        return 1;
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
        crow::response res(readFile("frontend/login.html"));
        res.set_header("Content-Type", "text/html");
        return res;
    });

    CROW_ROUTE(app, "/login.html")([]() {
        crow::response res(readFile("frontend/login.html"));
        res.set_header("Content-Type", "text/html");
        return res;
    });

    CROW_ROUTE(app, "/register.html")([]() {
        crow::response res(readFile("frontend/register.html"));
        res.set_header("Content-Type", "text/html");
        return res;
    });

    CROW_ROUTE(app, "/home.html")([]() {
        crow::response res(readFile("frontend/index.html"));
        res.set_header("Content-Type", "text/html");
        return res;
    });

    CROW_ROUTE(app, "/style.css")([]() {
        crow::response res(readFile("frontend/style.css"));
        res.set_header("Content-Type", "text/css");
        return res;
    });

    CROW_ROUTE(app, "/script.js")([]() {
        crow::response res(readFile("frontend/script.js"));
        res.set_header("Content-Type", "application/javascript");
        return res;
    });

    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasStringFields(body, {"username", "password"})) {
            return crow::response(400, R"({"message":"Username and password are required."})");
        }

        std::string username = body["username"].s();
        std::string password = body["password"].s();
        LoginResult result = authenticateUser(databasePath, username, password);

        crow::json::wvalue responseBody;
        responseBody["message"] = result.message;
        responseBody["administrator"] = result.administrator;
        responseBody["customer_id"] = result.customerId;
        responseBody["name"] = result.name;
        return crow::response(result.success ? 200 : 401, responseBody);
    });

    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasStringFields(body, {"username", "password", "name", "phone_number"})) {
            return crow::response(400, R"({"message":"All fields are required."})");
        }

        CustomerRegistrationResult result = registerCustomer(
            databasePath,
            body["username"].s(),
            body["password"].s(),
            body["name"].s(),
            body["phone_number"].s()
        );
        crow::json::wvalue responseBody;
        responseBody["message"] = result.message;
        responseBody["customer_id"] = result.customerId;
        return crow::response(result.success ? 201 : 409, responseBody);
    });

    app.port(18080).run();
    return 0;
}