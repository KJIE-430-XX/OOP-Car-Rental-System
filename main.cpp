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
#include "Reservation.h"
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
struct Session {
    bool administrator;
    int customerId;
};
unordered_map<string, Session> sessions;
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
    const auto found = sessions.find(session);
    return found != sessions.end() && found->second.administrator;
}

// Check authorization
bool isAuthenticated(const crow::request& request) {
    const string session = sessionFromRequest(request);
    lock_guard<mutex> lock(sessionsMutex);
    return sessions.find(session) != sessions.end();
}

int customerIdFromRequest(const crow::request& request) {
    const string session = sessionFromRequest(request);
    lock_guard<mutex> lock(sessionsMutex);
    const auto found = sessions.find(session);
    return found == sessions.end() ? 0 : found->second.customerId;
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
    result["available"] = vehicle.isAvailable();
    return result;
}

crow::json::wvalue reservationJson(SQLite::Statement& query) {
    crow::json::wvalue result;
    result["id"] = query.getColumn(0).getInt();
    result["vehicle_id"] = query.getColumn(1).getInt();
    result["start_date"] = query.getColumn(2).getString();
    result["end_date"] = query.getColumn(3).getString();
    result["total_fee"] = query.getColumn(4).getDouble();
    result["status"] = query.getColumn(5).getString();
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

    CROW_ROUTE(app, "/reservation.html")([](const crow::request& request) {
        return protectedPage(request, "frontend/reservation.html");
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
                sessions[session] = {result.administrator, result.customerId};
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
            sessions.erase(session);
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
        responseBody["customer_id"] = customerIdFromRequest(request);
        return crow::response(200, responseBody);
    });

    CROW_ROUTE(app, "/api/reservations").methods(crow::HTTPMethod::GET)
    ([](const crow::request& request) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            const bool administrator = isAdministrator(request);
            SQLite::Statement query(
                database,
                administrator
                    ? "SELECT reservation_id, vehicle_id, start_date, end_date, total_fee, status "
                      "FROM Reservations ORDER BY reservation_id DESC"
                    : "SELECT reservation_id, vehicle_id, start_date, end_date, total_fee, status "
                      "FROM Reservations WHERE customer_id = ? ORDER BY reservation_id DESC"
            );
            if (!administrator) {
                query.bind(1, customerIdFromRequest(request));
            }

            crow::json::wvalue::list reservations;
            while (query.executeStep()) {
                reservations.push_back(reservationJson(query));
            }
            crow::json::wvalue responseBody;
            responseBody["reservations"] = std::move(reservations);
            return crow::response(200, responseBody);
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to load reservations.");
        }
    });

    CROW_ROUTE(app, "/api/reservations/<int>").methods(crow::HTTPMethod::GET)
    ([](const crow::request& request, int reservationId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement query(
                database,
                "SELECT r.reservation_id, r.customer_id, r.vehicle_id, "
                "r.start_date, r.end_date, r.total_fee, r.status, "
                "r.checkout_date, v.license_plate, v.brand, v.model, "
                "v.description, v.vehicle_type, v.daily_rate "
                "FROM Reservations r "
                "JOIN Vehicles v ON v.vehicle_id = r.vehicle_id "
                "WHERE r.reservation_id = ?"
            );
            query.bind(1, reservationId);
            if (!query.executeStep()) {
                return jsonMessage(404, "Booking not found.");
            }

            const int ownerId = query.getColumn(1).getInt();
            if (!isAdministrator(request) && ownerId != customerIdFromRequest(request)) {
                return jsonMessage(403, "You can only view your own booking.");
            }

            crow::json::wvalue result;
            result["id"] = query.getColumn(0).getInt();
            result["customer_id"] = ownerId;
            result["vehicle_id"] = query.getColumn(2).getInt();
            result["start_date"] = query.getColumn(3).getString();
            result["end_date"] = query.getColumn(4).getString();
            result["total_fee"] = query.getColumn(5).getDouble();
            result["status"] = query.getColumn(6).getString();
            result["checkout_date"] = query.getColumn(7).isNull()
                ? "" : query.getColumn(7).getString();
            result["plate"] = query.getColumn(8).getString();
            result["brand"] = query.getColumn(9).getString();
            result["model"] = query.getColumn(10).getString();
            result["description"] = query.getColumn(11).getString();
            result["category"] = query.getColumn(12).getString();
            result["daily_rate"] = query.getColumn(13).getDouble();
            return crow::response(200, result);
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to load booking details.");
        }
    });

    CROW_ROUTE(app, "/api/reservations").methods(crow::HTTPMethod::POST)
    ([](const crow::request& request) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
        if (isAdministrator(request)) {
            return jsonMessage(403, "Customers must create bookings.");
        }

        auto body = crow::json::load(request.body);
        if (!body || !hasStringFields(body, {"vehicle_id", "start_date", "end_date"})) {
            return jsonMessage(400, "Vehicle and rental dates are required.");
        }
        const string vehicleId = body["vehicle_id"].s();
        const string startDate = body["start_date"].s();
        const string endDate = body["end_date"].s();
        if (!Reservation::validDateRange(startDate, endDate)) {
            return jsonMessage(400, "Use valid dates in YYYY-MM-DD format.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement vehicleQuery(
                database,
                "SELECT vehicle_id, license_plate, brand, model, description, vehicle_type, "
                "status, mileage, daily_rate, security_deposit, insurance_rate "
                "FROM Vehicles WHERE vehicle_id = ?"
            );
            vehicleQuery.bind(1, vehicleId);
            if (!vehicleQuery.executeStep()) {
                return jsonMessage(404, "Vehicle not found.");
            }

            auto vehicle = makeVehicle(
                vehicleQuery.getColumn(5).getString(),
                vehicleQuery.getColumn(0).getString(),
                vehicleQuery.getColumn(1).getString(),
                vehicleQuery.getColumn(2).getString(),
                vehicleQuery.getColumn(3).getString(),
                vehicleQuery.getColumn(4).getString(),
                vehicleQuery.getColumn(6).getString(),
                vehicleQuery.getColumn(7).getDouble(),
                vehicleQuery.getColumn(8).getDouble(),
                vehicleQuery.getColumn(9).getDouble(),
                vehicleQuery.getColumn(10).getDouble()
            );
            const int customerId = customerIdFromRequest(request);
            if (!vehicle || !vehicle->rentVehicle(to_string(customerId))) {
                return jsonMessage(409, "This vehicle is not available.");
            }

            Reservation reservation(
                "", vehicleId, customerId, startDate, endDate, vehicle->dailyRate());
            SQLite::Statement insert(
                database,
                "INSERT INTO Reservations "
                "(customer_id, vehicle_id, start_date, end_date, total_fee, status) "
                "VALUES (?, ?, ?, ?, ?, 'Confirmed')"
            );
            insert.bind(1, customerId);
            insert.bind(2, vehicleId);
            insert.bind(3, reservation.startDate());
            insert.bind(4, reservation.endDate());
            insert.bind(5, reservation.totalFee());
            insert.exec();

            SQLite::Statement updateVehicle(
                database,
                "UPDATE Vehicles SET status = ? WHERE vehicle_id = ?"
            );
            updateVehicle.bind(1, vehicle->status());
            updateVehicle.bind(2, vehicleId);
            updateVehicle.exec();
            return jsonMessage(201, "Booking confirmed.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to create the booking.");
        }
    });

    CROW_ROUTE(app, "/api/reservations/<int>/checkout")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& request, int reservationId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        auto body = crow::json::load(request.body);
        if (!body || !body["returned_mileage"]
            || body["returned_mileage"].t() != crow::json::type::Number) {
            return jsonMessage(400, "Returned mileage is required.");
        }
        const double returnedMileage = body["returned_mileage"].d();

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement reservationQuery(
                database,
                "SELECT vehicle_id, customer_id, status FROM Reservations "
                "WHERE reservation_id = ?"
            );
            reservationQuery.bind(1, reservationId);
            if (!reservationQuery.executeStep()) {
                return jsonMessage(404, "Booking not found.");
            }
            const int ownerId = reservationQuery.getColumn(1).getInt();
            if (!isAdministrator(request) && ownerId != customerIdFromRequest(request)) {
                return jsonMessage(403, "You can only check out your own booking.");
            }
            if (reservationQuery.getColumn(2).getString() != "Confirmed") {
                return jsonMessage(409, "This booking has already been checked out.");
            }

            const string vehicleId = reservationQuery.getColumn(0).getString();
            SQLite::Statement vehicleQuery(
                database,
                "SELECT vehicle_id, license_plate, brand, model, description, vehicle_type, "
                "status, mileage, daily_rate, security_deposit, insurance_rate "
                "FROM Vehicles WHERE vehicle_id = ?"
            );
            vehicleQuery.bind(1, vehicleId);
            if (!vehicleQuery.executeStep()) {
                return jsonMessage(404, "Vehicle not found.");
            }

            auto vehicle = makeVehicle(
                vehicleQuery.getColumn(5).getString(),
                vehicleQuery.getColumn(0).getString(),
                vehicleQuery.getColumn(1).getString(),
                vehicleQuery.getColumn(2).getString(),
                vehicleQuery.getColumn(3).getString(),
                vehicleQuery.getColumn(4).getString(),
                vehicleQuery.getColumn(6).getString(),
                vehicleQuery.getColumn(7).getDouble(),
                vehicleQuery.getColumn(8).getDouble(),
                vehicleQuery.getColumn(9).getDouble(),
                vehicleQuery.getColumn(10).getDouble()
            );
            if (!vehicle || !vehicle->returnVehicle(returnedMileage)) {
                return jsonMessage(409, "Returned mileage must not be lower than the current mileage.");
            }

            SQLite::Statement updateReservation(
                database,
                "UPDATE Reservations SET status = 'Completed', checkout_date = date('now') "
                "WHERE reservation_id = ?"
            );
            updateReservation.bind(1, reservationId);
            updateReservation.exec();

            SQLite::Statement updateVehicle(
                database,
                "UPDATE Vehicles SET status = ?, mileage = ? WHERE vehicle_id = ?"
            );
            updateVehicle.bind(1, vehicle->status());
            updateVehicle.bind(2, vehicle->mileage());
            updateVehicle.bind(3, vehicleId);
            updateVehicle.exec();
            return jsonMessage(200, "Vehicle checked out successfully.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to complete checkout.");
        }
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

    CROW_ROUTE(app, "/api/vehicles/<string>").methods(crow::HTTPMethod::PUT)
    ([](const crow::request& request, const string& vehicleId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
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
        const string status = body["status"].s();
        if (!makeVehicle(category, "", "", "", "", "", "", 0, 0, 0, 0)) {
            return jsonMessage(400, "Unknown vehicle category.");
        }
        if (status != "Available" && status != "Rented" && status != "Under_Maintenance") {
            return jsonMessage(400, "Invalid vehicle status.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement statement(
                database,
                "UPDATE Vehicles SET license_plate = ?, brand = ?, model = ?, description = ?, "
                "vehicle_type = ?, status = ?, mileage = ?, daily_rate = ?, security_deposit = ?, "
                "insurance_rate = ? WHERE vehicle_id = ?"
            );
            statement.bind(1, body["plate"].s());
            statement.bind(2, body["brand"].s());
            statement.bind(3, body["model"].s());
            statement.bind(4, body["description"].s());
            statement.bind(5, category);
            statement.bind(6, status);
            statement.bind(7, stod(body["mileage"].s()));
            statement.bind(8, stod(body["daily_rate"].s()));
            statement.bind(9, stod(body["security_deposit"].s()));
            statement.bind(10, stod(body["insurance_rate"].s()));
            statement.bind(11, vehicleId);
            statement.exec();

            if (database.getTotalChanges() == 0) {
                return jsonMessage(404, "Vehicle not found.");
            }
            return jsonMessage(200, "Vehicle updated successfully.");
        } catch (const invalid_argument&) {
            return jsonMessage(400, "Mileage and rates must be valid numbers.");
        } catch (const out_of_range&) {
            return jsonMessage(400, "Mileage and rates are out of range.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(409, "That license plate already exists.");
        }
    });

    CROW_ROUTE(app, "/api/vehicles/<string>").methods(crow::HTTPMethod::DELETE)
    ([](const crow::request& request, const string& vehicleId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
        if (!isAdministrator(request)) {
            return jsonMessage(403, "Administrator access is required.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement statement(database, "DELETE FROM Vehicles WHERE vehicle_id = ?");
            statement.bind(1, vehicleId);
            statement.exec();

            if (database.getTotalChanges() == 0) {
                return jsonMessage(404, "Vehicle not found.");
            }
            return jsonMessage(200, "Vehicle deleted successfully.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(409, "This vehicle cannot be deleted because it is used by a rental record.");
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