#include "Database.h"

#include <SQLiteCpp/SQLiteCpp.h>

namespace {
SQLite::Database openDatabase(const std::string& databasePath) {
    return SQLite::Database(
        databasePath,
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
}
}

void initializeDatabase(const std::string& databasePath) {
    SQLite::Database database = openDatabase(databasePath);

    database.exec("PRAGMA foreign_keys = ON;");

    database.exec(R"sql(
        CREATE TABLE IF NOT EXISTS Customers (
            customer_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username VARCHAR(30) NOT NULL UNIQUE,
            password VARCHAR(30) NOT NULL,
            name VARCHAR(50) NOT NULL,
            phone_number VARCHAR(20),
            active_transactions VARCHAR(50)
        );

        CREATE TABLE IF NOT EXISTS Vehicles (
            vehicle_id VARCHAR(20) PRIMARY KEY,
            license_plate VARCHAR(20) NOT NULL UNIQUE,
            vehicle_type VARCHAR(20) NOT NULL,
            status VARCHAR(20) NOT NULL,
            mileage FLOAT NOT NULL,
            daily_rate DOUBLE NOT NULL,
            security_deposit DOUBLE NOT NULL,
            insurance_rate FLOAT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS Transactions (
            transaction_id VARCHAR(20) PRIMARY KEY,
            customer_id INTEGER NOT NULL,
            vehicle_id VARCHAR(20) NOT NULL,
            rental_days INTEGER NOT NULL,
            total_rental_fee DOUBLE NOT NULL,
            damage_fee DOUBLE NOT NULL,
            final_total DOUBLE NOT NULL,
            status VARCHAR(20) NOT NULL,
            FOREIGN KEY (customer_id) REFERENCES Customers(customer_id),
            FOREIGN KEY (vehicle_id) REFERENCES Vehicles(vehicle_id)
        );

        CREATE TABLE IF NOT EXISTS Damage_logs (
            log_id INTEGER PRIMARY KEY AUTOINCREMENT,
            vehicle_id VARCHAR(20) NOT NULL,
            severity_score DOUBLE NOT NULL,
            description VARCHAR(255),
            customer_signoff BOOLEAN NOT NULL DEFAULT 0,
            FOREIGN KEY (vehicle_id) REFERENCES Vehicles(vehicle_id)
        );

        CREATE TABLE IF NOT EXISTS Administrators (
            admin_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username VARCHAR(30) NOT NULL,
            password VARCHAR(30) NOT NULL,
            role VARCHAR(20) NOT NULL
        );

        INSERT OR IGNORE INTO Administrators (admin_id, username, password, role)
        VALUES (1, 'admin', 'carrentadmin430', 'super');

        UPDATE Administrators
        SET password = 'carrentadmin430', role = 'super'
        WHERE username = 'admin';
    )sql");
}

CustomerRegistrationResult registerCustomer(
    const std::string& databasePath,
    const std::string& username,
    const std::string& password,
    const std::string& name,
    const std::string& phoneNumber
) {
    if (username.empty() || password.empty() || name.empty() || phoneNumber.empty()) {
        return {false, 0, "All fields are required."};
    }

    try {
        SQLite::Database database = openDatabase(databasePath);
        SQLite::Statement statement(
            database,
            "INSERT INTO Customers "
            "(username, password, name, phone_number, active_transactions) "
            "VALUES (?, ?, ?, ?, ?)"
        );
        statement.bind(1, username);
        statement.bind(2, password);
        statement.bind(3, name);
        statement.bind(4, phoneNumber);
        statement.bind(5, "");
        statement.exec();

        return {
            true,
            static_cast<int>(database.getLastInsertRowid()),
            "Registration successful. You can now log in."
        };
    } catch (const SQLite::Exception& exception) {
        if (std::string(exception.what()).find("UNIQUE constraint failed") != std::string::npos) {
            return {false, 0, "That username is already registered."};
        }
        return {false, 0, "Unable to register the account."};
    }
}

LoginResult authenticateUser(
    const std::string& databasePath,
    const std::string& username,
    const std::string& password
) {
    try {
        SQLite::Database database = openDatabase(databasePath);

        SQLite::Statement administratorQuery(
            database,
            "SELECT admin_id, role FROM Administrators "
            "WHERE username = ? AND password = ?"
        );
        administratorQuery.bind(1, username);
        administratorQuery.bind(2, password);
        if (administratorQuery.executeStep()) {
            return {
                true,
                true,
                administratorQuery.getColumn(0).getInt(),
                "Administrator",
                "Administrator login successful."
            };
        }

        SQLite::Statement customerQuery(
            database,
            "SELECT customer_id, name FROM Customers "
            "WHERE username = ? AND password = ?"
        );
        customerQuery.bind(1, username);
        customerQuery.bind(2, password);
        if (customerQuery.executeStep()) {
            return {
                true,
                false,
                customerQuery.getColumn(0).getInt(),
                customerQuery.getColumn(1).getString(),
                "Login successful."
            };
        }
    } catch (const SQLite::Exception&) {
        return {false, false, 0, "", "Unable to access the database."};
    }

    return {false, false, 0, "", "Invalid username or password."};
}