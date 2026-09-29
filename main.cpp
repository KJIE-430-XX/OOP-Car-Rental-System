/*
Command for build and test project locally

cmake --build build
.\build\Debug\CarRentalSystem.exe

link: http://localhost:18080/
*/

#include "crow.h"
#include <fstream>
#include <sstream>

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
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
        if (!body) return crow::response(400, R"({"message":"Invalid JSON"})");

        std::string username = body["username"].s();
        std::string password = body["password"].s();

        if (username == "admin" && password == "1234")
            return crow::response(200, R"({"message":"Login successful!"})");
        else
            return crow::response(401, R"({"message":"Invalid credentials"})");
    });

    app.port(18080).run();
    return 0;
}