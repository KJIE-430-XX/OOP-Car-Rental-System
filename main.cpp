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
#include "DamageFee.h"
#include "Reservation.h"
#include "Vehicle.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <atomic>
#include <cstdint>
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
    string username;
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

// factory function: Create the correct damage fee calculator based on the vehicle category
unique_ptr<DamageFeeCalculator> makeDamageFeeCalculator(const string& category) {

    // Call the correct derived class for the category
    if (category == "StandardCar") return make_unique<StandardCarDamageFee>();
    if (category == "LuxuryCar") return make_unique<LuxuryCarDamageFee>();
    if (category == "SUV") return make_unique<SUVDamageFee>();
    return nullptr;
}

// Extract the session ID from the Cookie header in the HTTP request.
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

// Check whether the user is an admin or not
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

// Get the customer ID from the user's session in the request.
int customerIdFromRequest(const crow::request& request) {
    const string session = sessionFromRequest(request);
    lock_guard<mutex> lock(sessionsMutex);
    const auto found = sessions.find(session);
    return found == sessions.end() ? 0 : found->second.customerId;
}

// Get the username from the user's session in the request.
string usernameFromRequest(const crow::request& request) {
    const string session = sessionFromRequest(request);
    lock_guard<mutex> lock(sessionsMutex);
    const auto found = sessions.find(session);
    return found == sessions.end() ? "" : found->second.username;
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

// factory function: converts a category string into a concrete C++ object since the category is derived class
// creates the correct Vehicle derived class from a category string
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

// Helper function: loads existing vehicle records from the SQLite database into the RentalSystem object when the application starts
void loadVehiclesFromDatabase(RentalSystem& rentalSystem) {
    SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
    SQLite::Statement query(
        database,
        "SELECT vehicle_id, license_plate, brand, model, description, vehicle_type, "
        "status, mileage, daily_rate, security_deposit, insurance_rate "
        "FROM Vehicles ORDER BY vehicle_id"
    );

    while (query.executeStep()) {
        auto vehicle = makeVehicle( // creates the correct Vehicle derived class from a category string
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
        if (!vehicle) {
            throw runtime_error("Unknown vehicle category in database.");
        }
        rentalSystem.addVehicle(std::move(vehicle));
    }
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

void updateAutomaticReservationStatuses() {
    SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
    SQLite::Transaction transaction(database);
    SQLite::Statement query(
        database,
        "SELECT reservation_id, vehicle_id, start_date, end_date "
        "FROM Reservations WHERE status IN ('Confirmed', 'Active', 'Overdue')"
    );

    // Get current date as a string in YYYY-MM-DD format.
    const string today = Reservation::todayDate();

    while (query.executeStep()) {
        const int reservationId = query.getColumn(0).getInt();
        const string vehicleId = query.getColumn(1).getString();

        // Automatically check in at the start date and mark late returns overdue.
        const string newStatus = Reservation::statusForDate(
            query.getColumn(2).getString(),
            query.getColumn(3).getString(),
            today
        );

        SQLite::Statement updateReservation(
            database,
            "UPDATE Reservations SET status = ? "
            "WHERE reservation_id = ? AND status != ?"
        );
        updateReservation.bind(1, newStatus);
        updateReservation.bind(2, reservationId);
        updateReservation.bind(3, newStatus);
        updateReservation.exec();

        if (newStatus == "Active" || newStatus == "Overdue") {
            SQLite::Statement rentVehicle(
                database,
                "UPDATE Vehicles SET status = 'Rented' "
                "WHERE vehicle_id = ? AND status = 'Available'"
            );
            rentVehicle.bind(1, vehicleId);
            rentVehicle.exec();
        }
    }
    transaction.commit();
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

    try {
        // Perform Automation Check-in or Overdue detection before API
        updateAutomaticReservationStatuses();
    } catch (const exception& e) {
        cerr << "Automatic reservation status update failed: "
             << e.what() << endl;
        return 1;
    }

    crow::SimpleApp app;

    //Call constructor in Authentication.cpp via Authentication.h
    AuthenticationService authenticationService(databasePath);

    // Create a RentalSystem object
    RentalSystem rentalSystem;
    try {
        // load the vehicles - run the helper function
        loadVehiclesFromDatabase(rentalSystem);
    } catch (const exception& e) {
        // Catch the error if occur
        cerr << "Vehicle loading failed: " << e.what() << endl;
        return 1;
    }

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
                sessions[session] = {result.administrator, result.customerId, username};
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

    // Retrieve reservation record from database
    CROW_ROUTE(app, "/api/reservations").methods(crow::HTTPMethod::GET)
    ([](const crow::request& request) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        try {
            // Perform Automation Check-in or Overdue detection before API
            updateAutomaticReservationStatuses();
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Transaction transaction(database);
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

    // Retrieve reservation from database based on id
    CROW_ROUTE(app, "/api/reservations/<int>").methods(crow::HTTPMethod::GET)
    ([](const crow::request& request, int reservationId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }

        try {
            // Perform Automation Check-in or Overdue detection before API
            updateAutomaticReservationStatuses();
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

            // Retrieve the Vehicle Damage Report for specific reservation
            SQLite::Statement report(
                database,
                "SELECT report_id, damage_fee, paid FROM Damage_reports "
                "WHERE reservation_id = ?"
            );
            report.bind(1, reservationId);
            if (report.executeStep()) {
                result["damage_fee"] = report.getColumn(1).getDouble();
                result["damage_paid"] = report.getColumn(2).getInt() != 0;
                crow::json::wvalue::list items;
                SQLite::Statement itemQuery(
                    database,
                    "SELECT component, fee FROM Damage_items WHERE report_id = ? "
                    "ORDER BY item_id"
                );
                itemQuery.bind(1, report.getColumn(0).getInt());
                while (itemQuery.executeStep()) {
                    crow::json::wvalue item;
                    item["component"] = itemQuery.getColumn(0).getString();
                    item["fee"] = itemQuery.getColumn(1).getDouble();
                    items.push_back(std::move(item));
                }
                result["damage_items"] = std::move(items);
            } else {
                result["damage_fee"] = 0.0;
                result["damage_paid"] = false;
            }
            return crow::response(200, result);
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to load booking details.");
        }
    });

    // Reserve vehicle (rent vehicle)
    CROW_ROUTE(app, "/api/reservations").methods(crow::HTTPMethod::POST)
    ([&rentalSystem](const crow::request& request) {
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

        try {
            // Perform Automation Check-in or Overdue detection before API
            updateAutomaticReservationStatuses();
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            const int customerId = customerIdFromRequest(request);
            Reservation reservation;
            const RentalOperationResult result = rentalSystem.createReservation(
                database, vehicleId, customerId, startDate, endDate, reservation);
            switch (result) {

            // Display different message based on return type
            case RentalOperationResult::Success:
                return jsonMessage(201, "Booking confirmed.");
            case RentalOperationResult::InvalidDates:
                return jsonMessage(400, "Use valid dates in YYYY-MM-DD format.");
            case RentalOperationResult::NotFound:
                return jsonMessage(404, "Vehicle not found.");
            case RentalOperationResult::UnderMaintenance:
                return jsonMessage(409, "This vehicle is under maintenance.");
            case RentalOperationResult::DuplicateReservation:
                return jsonMessage(
                    409, "You already have an active rental for this vehicle.");
            case RentalOperationResult::DateUnavailable:
                return jsonMessage(
                    409, "This vehicle is already reserved during the requested dates.");
            default:
                return jsonMessage(500, "Unable to create the booking.");
            }
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to create the booking.");
        }
    });

    // Checkout vehicle (return vehicle)
    CROW_ROUTE(app, "/api/reservations/<int>/checkout")
        .methods(crow::HTTPMethod::POST)
    ([&rentalSystem](const crow::request& request, int reservationId) {
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
            // Perform Automation Check-in or Overdue detection before API
            updateAutomaticReservationStatuses();
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            const RentalOperationResult result = rentalSystem.checkoutVehicle(
                database,
                reservationId,
                returnedMileage,
                customerIdFromRequest(request),
                isAdministrator(request));
            switch (result) {
            case RentalOperationResult::Success:
                return jsonMessage(200, "Return submitted. The vehicle is pending inspection.");
            case RentalOperationResult::NotFound:
                return jsonMessage(404, "Booking or vehicle not found.");
            case RentalOperationResult::Unauthorized:
                return jsonMessage(403, "You can only check out your own booking.");
            case RentalOperationResult::AlreadyCompleted:
                return jsonMessage(409, "This booking has already been checked out.");
            case RentalOperationResult::PendingInspection:
                return jsonMessage(409, "This booking is already pending inspection.");
            case RentalOperationResult::InvalidMileage:
                return jsonMessage(
                    409, "Returned mileage must not be lower than the current mileage.");
            default:
                return jsonMessage(500, "Unable to complete checkout.");
            }
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to complete checkout.");
        }
    });

    // Allow administrator to inspect a returned vehicle
    CROW_ROUTE(app, "/api/reservations/<int>/inspection")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& request, int reservationId) {
        if (!isAuthenticated(request) || !isAdministrator(request)) {
            return jsonMessage(403, "Administrator access is required.");
        }
        auto body = crow::json::load(request.body);
        if (!body || !body["damages"]
            || body["damages"].t() != crow::json::type::List) {
            return jsonMessage(400, "A damage checklist is required.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);

            // Retrieve Reservation and vehicle information
            SQLite::Statement reservation(
                database,
                "SELECT r.vehicle_id, r.status, v.vehicle_type "
                "FROM Reservations r JOIN Vehicles v ON v.vehicle_id = r.vehicle_id "
                "WHERE r.reservation_id = ?"
            );

            // Return error message if no reservation found
            reservation.bind(1, reservationId);
            if (!reservation.executeStep()) {
                return jsonMessage(404, "Booking not found.");
            }

            // Return error message if status is not "Pending Inspection"
            if (reservation.getColumn(1).getString() != "Pending Inspection") {
                return jsonMessage(409, "This booking is not pending inspection.");
            }

            // Use the category to call the correct function
            auto calculator = makeDamageFeeCalculator(reservation.getColumn(2).getString());
            if (!calculator) {
                return jsonMessage(500, "Unable to calculate damage fees.");
            }

            // Insert the damage record into the Damage Report table (without damage_fee)
            SQLite::Transaction transaction(database);
            SQLite::Statement report(
                database,
                "INSERT INTO Damage_reports "
                "(reservation_id, vehicle_id, returned_mileage, damage_fee, inspection_date) "
                "SELECT ?, vehicle_id, return_mileage, 0, date('now') "
                "FROM Reservations WHERE reservation_id = ?"
            );
            report.bind(1, reservationId);
            report.bind(2, reservationId);
            report.exec();
            const int64_t reportId = database.getLastInsertRowid();
            double total = 0.0;

            // Retrive the damages checklist from the form
            for (const auto& damage : body["damages"]) {

                // Validate the damage
                if (damage.t() != crow::json::type::String) {
                    return jsonMessage(400, "Damage checklist entries must be text.");
                }
                const string component = damage.s();

                // Call factory function to calculate the damage fee based on the category
                const double fee = calculator->feeFor(component);
                if (fee <= 0.0) {
                    return jsonMessage(400, "The damage checklist contains an invalid component.");
                }

                // Insert the damage into the damage item table
                SQLite::Statement item(
                    database,
                    "INSERT INTO Damage_items (report_id, component, fee) VALUES (?, ?, ?)"
                );
                item.bind(1, reportId);
                item.bind(2, component);
                item.bind(3, fee);
                item.exec();

                // Accumulate the damage total fee
                total += fee;
            }

            // Update the total damage fee into the Damage Report table
            SQLite::Statement updateReport(
                database, "UPDATE Damage_reports SET damage_fee = ? WHERE report_id = ?");
            updateReport.bind(1, total);
            updateReport.bind(2, reportId);
            updateReport.exec();

            // Apdate the reservation from 'Pending Inspection' to 'Awaiting Payment'
            SQLite::Statement updateReservation(
                database,
                "UPDATE Reservations SET status = 'Awaiting Payment' "
                "WHERE reservation_id = ?"
            );
            updateReservation.bind(1, reservationId);
            updateReservation.exec();
            transaction.commit();
            return jsonMessage(200, "Damage report submitted.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to submit the damage report.");
        }
    });

    // Processes the damage fee payment and completes the vehicle return
    CROW_ROUTE(app, "/api/reservations/<int>/damage-payment")
        .methods(crow::HTTPMethod::POST)
    ([&rentalSystem](const crow::request& request, int reservationId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);

            // Retrieve the reservation
            SQLite::Statement reservation(
                database,
                "SELECT customer_id, vehicle_id, status, return_mileage FROM Reservations "
                "WHERE reservation_id = ?"
            );
            reservation.bind(1, reservationId);
            if (!reservation.executeStep()) {
                return jsonMessage(404, "Booking not found.");
            }
            if (reservation.getColumn(0).getInt() != customerIdFromRequest(request)) {
                return jsonMessage(403, "You can only pay your own damage fee.");
            }
            if (reservation.getColumn(2).getString() != "Awaiting Payment") {
                return jsonMessage(409, "This booking has no outstanding damage fee.");
            }

            // Update the damage report table, set the paid = 1, payment_date = today
            SQLite::Transaction transaction(database);
            SQLite::Statement report(
                database,
                "UPDATE Damage_reports SET paid = 1, payment_date = date('now') "
                "WHERE reservation_id = ? AND paid = 0"
            );
            report.bind(1, reservationId);
            report.exec();
            if (database.getTotalChanges() != 1) {
                return jsonMessage(409, "The damage fee has already been paid.");
            }

            // Update the reservation table, change the status from 'Awaiting Payment' to 'Completed'
            SQLite::Statement complete(
                database,
                "UPDATE Reservations SET status = 'Completed' WHERE reservation_id = ?"
            );
            complete.bind(1, reservationId);
            complete.exec();

            // Update the vehicle back to available after user completely pay the damage fee and return the vehicle.
            SQLite::Statement available(
                database,
                "UPDATE Vehicles SET status = 'Available' WHERE vehicle_id = ?"
            );
            available.bind(1, reservation.getColumn(1).getString());
            available.exec();
            transaction.commit();
            Vehicle* vehicle = rentalSystem.findVehicle(reservation.getColumn(1).getString());
            if (vehicle && !vehicle->returnVehicle(reservation.getColumn(3).getDouble())) {
                return jsonMessage(500, "Payment completed but vehicle state could not be synchronized.");
            }
            return jsonMessage(200, "Damage fee paid. Vehicle return completed.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to complete the damage payment.");
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
            // Perform Automation Check-in or Overdue detection before API
            updateAutomaticReservationStatuses();
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
                auto vehicle = makeVehicle( // // creates the correct Vehicle derived class from a category string
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
    ([&rentalSystem](const crow::request& request) {

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

            // Create a C++ vehicle object of the correct category and add it to the RentalSystem's in-memory collection
            rentalSystem.addVehicle(makeVehicle( // // creates the correct Vehicle derived class from a category string
                category,
                to_string(database.getLastInsertRowid()), // Get the latest id
                body["plate"].s(),
                body["brand"].s(),
                body["model"].s(),
                body["description"].s(),
                body["status"].s(),
                stod(body["mileage"].s()),
                stod(body["daily_rate"].s()),
                stod(body["security_deposit"].s()),
                stod(body["insurance_rate"].s())
            ));
            return jsonMessage(201, "Vehicle added successfully.");
        } catch (const invalid_argument&) {
            return jsonMessage(400, "Mileage and rates must be valid numbers.");
        } catch (const out_of_range&) {
            return jsonMessage(400, "Mileage and rates are out of range.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(409, "That license plate already exists.");
        }
    });

    // Maintenance - Admin use to mark the vehicle under maintenance
    CROW_ROUTE(app, "/api/vehicles/<string>/maintenance").methods(crow::HTTPMethod::POST)
    ([&rentalSystem](const crow::request& request, const string& vehicleId) {

        // Ensure the user is admin before allow them to access
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
        if (!isAdministrator(request)) {
            return jsonMessage(403, "Administrator access is required.");
        }

        auto body = crow::json::load(request.body);
        string notes;
        if (body && body["notes"] && body["notes"].t() != crow::json::type::String) {
            return jsonMessage(400, "Maintenance notes must be text.");
        }
        if (body && body["notes"]) {
            notes = body["notes"].s();
        }

        try {
            // Perform Automation Check-in or Overdue detection before API
            updateAutomaticReservationStatuses();
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Transaction transaction(database);
            // Select the vehicle from in-memory vehicleMap_ where id = vehicleId
            Vehicle* vehicle = rentalSystem.findVehicle(vehicleId);
            if (!vehicle) {
                return jsonMessage(404, "Vehicle not found.");
            }

            // Change the status between 'Available' and 'Under_Maintenance'
            // It will also update the vehicle->status()
            const bool changed = vehicle->status() == "Available"
                ? vehicle->markUnderMaintenance(
                    database,
                    customerIdFromRequest(request),
                    usernameFromRequest(request),
                    notes)
                : vehicle->markAvailable(
                    database,
                    customerIdFromRequest(request),
                    usernameFromRequest(request),
                    notes);
            if (!changed) {
                return jsonMessage(
                    409,
                    "Only available vehicles can be placed under maintenance, "
                    "and only maintenance vehicles can be made available."
                );
            }

            transaction.commit();
            return jsonMessage(200, "Vehicle status updated successfully.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to update vehicle status.");
        }
    });

    // Retrieve maintenance record for the vehicle
    CROW_ROUTE(app, "/api/vehicles/<string>/maintenance-records")
        .methods(crow::HTTPMethod::GET)
    ([](const crow::request& request, const string& vehicleId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
        if (!isAdministrator(request)) {
            return jsonMessage(403, "Administrator access is required.");
        }

        // Query to select the maintenance records
        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement query(
                database,
                "SELECT maintenance_id, started_at, ended_at, "
                "started_by_admin_id, started_by_username, ended_by_admin_id, "
                "ended_by_username, notes, completion_notes, previous_maintenance_id "
                "FROM Maintenance_records WHERE vehicle_id = ? "
                "ORDER BY maintenance_id DESC"
            );
            query.bind(1, vehicleId);

            crow::json::wvalue::list records;

            // Convert the query result into json format.
            while (query.executeStep()) {
                crow::json::wvalue record;
                record["id"] = query.getColumn(0).getInt();
                record["started_at"] = query.getColumn(1).getString();
                record["ended_at"] = query.getColumn(2).isNull()
                    ? "" : query.getColumn(2).getString();
                record["started_by_admin_id"] = query.getColumn(3).getInt();
                record["started_by_username"] = query.getColumn(4).getString();
                record["ended_by_admin_id"] = query.getColumn(5).isNull()
                    ? 0 : query.getColumn(5).getInt();
                record["ended_by_username"] = query.getColumn(6).isNull()
                    ? "" : query.getColumn(6).getString();
                record["notes"] = query.getColumn(7).getString();
                record["completion_notes"] = query.getColumn(8).getString();
                record["previous_maintenance_id"] = query.getColumn(9).isNull()
                    ? 0 : query.getColumn(9).getInt();
                records.push_back(std::move(record));
            }

            crow::json::wvalue responseBody;
            responseBody["records"] = std::move(records);
            return crow::response(200, responseBody);
        } catch (const SQLite::Exception&) {
            return jsonMessage(500, "Unable to load maintenance records.");
        }
    });

    // Update vehicle details (Edit)
    CROW_ROUTE(app, "/api/vehicles/<string>").methods(crow::HTTPMethod::PUT)
    ([&rentalSystem](const crow::request& request, const string& vehicleId) {
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
        if (body["maintenance_notes"]
            && body["maintenance_notes"].t() != crow::json::type::String) {
            return jsonMessage(400, "Maintenance notes must be text.");
        }
        string maintenanceNotes;
        if (body["maintenance_notes"]) {
            maintenanceNotes = body["maintenance_notes"].s();
        }
        // Some validation check
        if (!makeVehicle(category, "", "", "", "", "", "", 0, 0, 0, 0)) {
            return jsonMessage(400, "Unknown vehicle category.");
        }
        if (status != "Available" && status != "Rented" && status != "Under_Maintenance") {
            return jsonMessage(400, "Invalid vehicle status.");
        }

        try {
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Transaction transaction(database);

            // Select the edited vehicle first
            SQLite::Statement currentVehicleQuery(
                database,
                "SELECT vehicle_id, license_plate, brand, model, description, vehicle_type, "
                "status, mileage, daily_rate, security_deposit, insurance_rate "
                "FROM Vehicles WHERE vehicle_id = ?"
            );
            currentVehicleQuery.bind(1, vehicleId);

            // Error message if not found
            if (!currentVehicleQuery.executeStep()) {
                return jsonMessage(404, "Vehicle not found.");
            }

            // Validate the category with derived class
            auto currentVehicle = makeVehicle( // creates the correct Vehicle derived class from a category string
                currentVehicleQuery.getColumn(5).getString(),
                currentVehicleQuery.getColumn(0).getString(),
                currentVehicleQuery.getColumn(1).getString(),
                currentVehicleQuery.getColumn(2).getString(),
                currentVehicleQuery.getColumn(3).getString(),
                currentVehicleQuery.getColumn(4).getString(),
                currentVehicleQuery.getColumn(6).getString(),
                currentVehicleQuery.getColumn(7).getDouble(),
                currentVehicleQuery.getColumn(8).getDouble(),
                currentVehicleQuery.getColumn(9).getDouble(),
                currentVehicleQuery.getColumn(10).getDouble()
            );
            if (!currentVehicle) {
                return jsonMessage(500, "Unable to load vehicle.");
            }
            if (currentVehicle->status() == "Pending Inspection") {
                return jsonMessage(
                    409, "This vehicle cannot be edited until its return inspection is paid."
                );
            }

            // Define the statusChangeValid and reuse the function to check
            // Validate whether a vehicle's status can be changed to the status requested
            bool statusChangeValid = true;
            if (status != currentVehicle->status()) {
                if (status == "Under_Maintenance") {
                    statusChangeValid = currentVehicle->markUnderMaintenance(
                        database,
                        // Identify which administrator performed the vehicle status change
                        customerIdFromRequest(request),
                        usernameFromRequest(request),
                        maintenanceNotes);
                } else if (status == "Available"
                    && currentVehicle->status() == "Under_Maintenance") {
                    statusChangeValid = currentVehicle->markAvailable(
                        database,
                        // Identify which administrator performed the vehicle status change
                        customerIdFromRequest(request),
                        usernameFromRequest(request),
                        maintenanceNotes);
                }
            }

            // If before value is 'Rented', then return error
            if (!statusChangeValid) {
                return jsonMessage(
                    409,
                    "A vehicle can only be marked under maintenance when available, "
                    "and only maintenance vehicles can be marked available."
                );
            }

            // Sqlite update the vehicle details
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

            // If zero records been updated, then return vehicle not found
            if (database.getTotalChanges() == 0) {
                return jsonMessage(404, "Vehicle not found.");
            }

            // Add the update vehicle details into the in-memory vehicleMap_
            // It will replace rather than add new if the vehicleId is same
            rentalSystem.addVehicle(makeVehicle(
                category,
                vehicleId,
                body["plate"].s(),
                body["brand"].s(),
                body["model"].s(),
                body["description"].s(),
                status,
                stod(body["mileage"].s()),
                stod(body["daily_rate"].s()),
                stod(body["security_deposit"].s()),
                stod(body["insurance_rate"].s())
            ));
            transaction.commit();
            return jsonMessage(200, "Vehicle updated successfully.");
        } catch (const invalid_argument&) {
            return jsonMessage(400, "Mileage and rates must be valid numbers.");
        } catch (const out_of_range&) {
            return jsonMessage(400, "Mileage and rates are out of range.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(409, "That license plate already exists.");
        }
    });

    // Delete vehicle
    CROW_ROUTE(app, "/api/vehicles/<string>").methods(crow::HTTPMethod::DELETE)
    ([&rentalSystem](const crow::request& request, const string& vehicleId) {
        if (!isAuthenticated(request)) {
            return jsonMessage(401, "Authentication is required.");
        }
        if (!isAdministrator(request)) {
            return jsonMessage(403, "Administrator access is required.");
        }

        try {

            // Delete the vehicle from sqlite database where id = vehicleId
            SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
            SQLite::Statement statement(database, "DELETE FROM Vehicles WHERE vehicle_id = ?");
            statement.bind(1, vehicleId);
            statement.exec();

            // If delete failed, then vehicle not found
            if (database.getTotalChanges() == 0) {
                return jsonMessage(404, "Vehicle not found.");
            }

            // Erase the vehicle details from in-memory vehicleMap_ also
            rentalSystem.removeVehicle(vehicleId);
            return jsonMessage(200, "Vehicle deleted successfully.");
        } catch (const SQLite::Exception&) {
            return jsonMessage(409, "This vehicle cannot be deleted because it is used by a rental record.");
        }
    });

    // Register account
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