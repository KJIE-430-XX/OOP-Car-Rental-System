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
#include "Authentication.h"
#include "Database.h"
#include "Vehicle.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>

using namespace std;

string readFile(const string& path);

namespace {
const string databasePath = "car_rental.db";
mutex sessionsMutex;
unordered_map<string, bool> administratorSessions;
atomic<unsigned long> nextSessionId{1};

bool hasStringFields(
    const crow::json::rvalue& body,
    const initializer_list<const char*>& fields
) {
    for (const char* field : fields) {
        if (!body[field] || body[field].t() != crow::json::type::String) {
            return false;
        }

    }
    return true;
}

crow::response jsonMessage(int statusCode, const string& message) {
    crow::json::wvalue body;
    body["message"] = message;
    return crow::response(statusCode, body);
}

string sessionFromRequest(const crow::request& request) {
    const string cookieHeader = request.get_header_value("Cookie");
    const string prefix = "session=";
    const size_t start = cookieHeader.find(prefix);
    if (start == string::npos) {
        return "";
    }
    const size_t valueStart = start + prefix.size();
    const size_t valueEnd = cookieHeader.find(';', valueStart);
    return cookieHeader.substr(valueStart, valueEnd == string::npos
        ? string::npos : valueEnd - valueStart);
}

bool isAdministrator(const crow::request& request) {
    const string session = sessionFromRequest(request);
    lock_guard<mutex> lock(sessionsMutex);
    const auto found = administratorSessions.find(session);
    return found != administratorSessions.end() && found->second;
}

// Check authorization
bool isAuthenticated(const crow::request& request) {
    const string session = sessionFromRequest(request);
    lock_guard<mutex> lock(sessionsMutex);
    return administratorSessions.find(session) != administratorSessions.end();
}

// Helper function: redirect to login page when user access is unauthorized
crow::response redirectToLogin() {
    crow::response response(302);
    response.set_header("Location", "/login.html");
    return response;
}

// Function: Call isAuthenticated to check access and returns redirectToLogin() if false
crow::response protectedPage(
    const crow::request& request,
    const string& path
) {
    if (!isAuthenticated(request)) {
        return redirectToLogin();
    }

    crow::response response(readFile(path));
    response.set_header("Content-Type", "text/html");
    return response;
}

// converts a category string into a concrete C++ object since the category is derived class
std::unique_ptr<Vehicle> makeVehicle(
    const string& category,
    const string& id,
    const string& plate,
    const string& brand,
    const string& model,
    const string& description,
    const string& status,
    double mileage,
    double dailyRate,
    double securityDeposit,
    double insuranceRate
) {
    if (category == "StandardCar") {
        return make_unique<StandardCar>(
            id, plate, brand, model, description, status, mileage, dailyRate, securityDeposit, insuranceRate);
    }
    if (category == "LuxuryCar") {
        return make_unique<LuxuryCar>(
            id, plate, brand, model, description, status, mileage, dailyRate, securityDeposit, insuranceRate);
    }
    if (category == "SUV") {
        return make_unique<SUV>(
            id, plate, brand, model, description, status, mileage, dailyRate, securityDeposit, insuranceRate);
    }
    return nullptr;
}

crow::json::wvalue vehicleJson(const Vehicle& vehicle) {
    crow::json::wvalue result;
    result["id"] = vehicle.id();
    result["plate"] = vehicle.plate();
    result["brand"] = vehicle.brand();
    result["model"] = vehicle.model();
    result["status"] = vehicle.status();
    result["mileage"] = vehicle.mileage();
    result["daily_rate"] = vehicle.dailyRate();
    result["security_deposit"] = vehicle.securityDeposit();
    result["insurance_rate"] = vehicle.insuranceRate();
    result["category"] = vehicle.category();
    result["description"] = vehicle.description();
    return result;
}
}

string readFile(const string& path) {
    ifstream file(path);
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        initializeDatabase(databasePath);
    }
    catch (const exception& e) {
        cerr << "Database init failed: " << e.what() << endl;
        return 1;
    }

    crow::SimpleApp app;

    //Call constructor in Authentication.cpp via Authentication.h
    AuthenticationService authenticationService(databasePath);

    // Define Crow route for the website's root URL
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

    CROW_ROUTE(app, "/home.html")([](const crow::request& request) {
        return protectedPage(request, "frontend/index.html");
    });

    CROW_ROUTE(app, "/add_vehicle.html")([](const crow::request& request) {
        return protectedPage(request, "frontend/add_vehicle.html");
    });

    CROW_ROUTE(app, "/standardcar.html")([](const crow::request& request) {
        return protectedPage(request, "frontend/standardcar.html");
    });

    CROW_ROUTE(app, "/luxurycar.html")([](const crow::request& request) {
        return protectedPage(request, "frontend/luxurycar.html");
    });

    CROW_ROUTE(app, "/suv.html")([](const crow::request& request) {
        return protectedPage(request, "frontend/suv.html");
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
    ([&authenticationService](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasStringFields(body, {"username", "password"})) {
            return crow::response(400, R"({"message":"Username and password are required."})");
        }

        string username = body["username"].s();
        string password = body["password"].s();

        //Call the Authentication.cpp 
        // Encapsulation: login logic is contained inside AuthenticationService  (Authentication.cpp).
        // Abstraction: main.cpp only needs to call login()
        LoginResult result = authenticationService.login(username, password);

        crow::json::wvalue responseBody;
        responseBody["message"] = result.message;
        responseBody["administrator"] = result.administrator;
        responseBody["customer_id"] = result.customerId;
        responseBody["name"] = result.name;
        crow::response response(result.success ? 200 : 401, responseBody);
        if (result.success) {
            const string session = "session-" + to_string(nextSessionId++);
            {
                lock_guard<mutex> lock(sessionsMutex);
                administratorSessions[session] = result.administrator;
            }
            response.set_header("Set-Cookie", "session=" + session + "; Path=/; HttpOnly");
        }
        return response;
    });

    CROW_ROUTE(app, "/api/logout").methods(crow::HTTPMethod::POST)
    ([](const crow::request& request) {
        const string session = sessionFromRequest(request);
        if (!session.empty()) {
            lock_guard<mutex> lock(sessionsMutex);
            administratorSessions.erase(session);
        }

        crow::response response(200, R"({"message":"Logged out successfully."})");
        response.set_header("Set-Cookie", "session=; Path=/; Max-Age=0; HttpOnly");
        return response;
    });

    CROW_ROUTE(app, "/api/session").methods(crow::HTTPMethod::GET)
    ([](const crow::request& request) {
        crow::json::wvalue responseBody;
        responseBody["authenticated"] = isAuthenticated(request);
        responseBody["administrator"] = isAdministrator(request);
        return crow::response(200, responseBody);
    });

    //Get the list vehicles (optionally filtered by ?category=)
    CROW_ROUTE(app, "/api/vehicles").methods(crow::HTTPMethod::GET)
    ([](const crow::request& request) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        const string category = request.url_params.get("category")
            ? request.url_params.get("category") : "";
        if (!makeVehicle(category, "", "", "", "", "", "", 0, 0, 0, 0) && !category.empty()) {
            return crow::response(400, R"({"message":"Unknown vehicle category."})");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement query(
                database,
                category.empty()
                    ? "SELECT vehicle_id, license_plate, brand, model, description, vehicle_type, status, mileage, daily_rate, security_deposit, insurance_rate FROM Vehicles ORDER BY vehicle_id"
                    : "SELECT vehicle_id, license_plate, brand, model, description, vehicle_type, status, mileage, daily_rate, security_deposit, insurance_rate FROM Vehicles WHERE vehicle_type = ? ORDER BY vehicle_id"
            );
            if (!category.empty()) {
                query.bind(1, category);
            }

            crow::json::wvalue::list vehicles;

            // After get the query results, passes them into makeVehicle(...) to create a concrete polymorphic object
            // Always ensure the category is exist first (StandardCar, LuxuryCar, SUV)
            while (query.executeStep()) {
                auto vehicle = makeVehicle(
                    query.getColumn(5).getString(),
                    query.getColumn(0).getString(),
                    query.getColumn(1).getString(),
                    query.getColumn(2).getString(),
                    query.getColumn(3).getString(),
                    query.getColumn(4).getString(),
                    query.getColumn(6).getString(),
                    query.getColumn(7).getDouble(),
                    query.getColumn(8).getDouble(),
                    query.getColumn(9).getDouble(),
                    query.getColumn(10).getDouble()
                );
                if (vehicle) {
                    // Call the getters, and convert them into json
                    vehicles.push_back(vehicleJson(*vehicle));
                }
            }
            crow::json::wvalue responseBody;
            responseBody["vehicles"] = std::move(vehicles);
            return crow::response(200, responseBody);
        } catch (const SQLite::Exception&) {
            return crow::response(500, R"({"message":"Unable to load vehicles."})");
        }
    });

    // Add a vehicle (admin only) -- Post method to pass the data to endpoint
    CROW_ROUTE(app, "/api/vehicles").methods(crow::HTTPMethod::POST)
    ([](const crow::request& request) {

        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        // Return error message if user not an admin
        if (!isAdministrator(request)) {
            return jsonMessage(403, "Administrator access is required.");
        }

        auto body = crow::json::load(request.body);
        if (!body || !hasStringFields(body, {
            "plate", "brand", "model", "description", "category", "status", "mileage",
            "daily_rate", "security_deposit", "insurance_rate"
        })) {
            return jsonMessage(400, "All vehicle fields are required.");
        }
        const string category = body["category"].s();
        if (body["status"].s() != "Available" &&
            body["status"].s() != "Rented" &&
            body["status"].s() != "Under_Maintenance") {
            return jsonMessage(400, "Invalid vehicle status.");
        }

        // Return error message if category not exist
        if (!makeVehicle(category, "", "", "", "", "", "", 0, 0, 0, 0)) {
            return jsonMessage(400, "Unknown vehicle category.");
        }

        // Start to try catch for bind (insert) the vehicle
        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement statement(
                database,
                "INSERT INTO Vehicles "
                "(license_plate, brand, model, description, vehicle_type, status, mileage, daily_rate, security_deposit, insurance_rate) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
            );
            statement.bind(1, body["plate"].s());
            statement.bind(2, body["brand"].s());
            statement.bind(3, body["model"].s());
            statement.bind(4, body["description"].s());
            statement.bind(5, category);
            statement.bind(6, body["status"].s());
            statement.bind(7, stod(body["mileage"].s()));
            statement.bind(8, stod(body["daily_rate"].s()));
            statement.bind(9, stod(body["security_deposit"].s()));
            statement.bind(10, stod(body["insurance_rate"].s()));
            statement.exec();
            return jsonMessage(201, "Vehicle added successfully.");
        } catch (const invalid_argument&) {
            return jsonMessage(400, "Mileage and rates must be valid numbers.");
        } catch (const out_of_range&) {
            return jsonMessage(400, "Mileage and rates are out of range.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(409, "That license plate already exists.");
        }
    });

    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([&authenticationService](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasStringFields(body, {"username", "password", "name", "phone_number"})) {
            return crow::response(400, R"({"message":"All fields are required."})");
        }

        // Encapsulation: login logic is contained inside AuthenticationService  (Authentication.cpp).
        // Abstraction: main.cpp only needs to call login()
        CustomerRegistrationResult result = authenticationService.registerAccount(
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