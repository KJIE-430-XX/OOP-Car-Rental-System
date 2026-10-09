#include "Database.h"

#include <SQLiteCpp/SQLiteCpp.h>

using namespace std;

namespace {
SQLite::Database openDatabase(const string& databasePath) {
    return SQLite::Database(
        databasePath,
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
}
}

void initializeDatabase(const string& databasePath) {
    SQLite::Database database = openDatabase(databasePath);

    database.exec("PRAGMA busy_timeout = 5000;");
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
            vehicle_id INTEGER PRIMARY KEY AUTOINCREMENT,
            license_plate VARCHAR(20) NOT NULL UNIQUE,
            brand VARCHAR(50) NOT NULL,
            model VARCHAR(50) NOT NULL,
            description VARCHAR(255) NOT NULL,
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

    try {
        database.exec("ALTER TABLE Vehicles ADD COLUMN brand VARCHAR(50) NOT NULL DEFAULT ''");
    } catch (const SQLite::Exception&) {
    }
    try {
        database.exec("ALTER TABLE Vehicles ADD COLUMN model VARCHAR(50) NOT NULL DEFAULT ''");
    } catch (const SQLite::Exception&) {
    }
    try {
        database.exec("ALTER TABLE Vehicles ADD COLUMN description VARCHAR(255) NOT NULL DEFAULT ''");
    } catch (const SQLite::Exception&) {
    }

    bool integerVehicleId = false;
    {
        SQLite::Statement vehicleInfo(database, "PRAGMA table_info(Vehicles)");
        while (vehicleInfo.executeStep()) {
            if (vehicleInfo.getColumn(1).getString() == "vehicle_id") {
                integerVehicleId = vehicleInfo.getColumn(2).getString() == "INTEGER";
                break;
            }
        }
    }

    if (!integerVehicleId) {
        database.exec(R"sql(
            PRAGMA foreign_keys = OFF;
            BEGIN TRANSACTION;
            CREATE TABLE Vehicles_new (
                vehicle_id INTEGER PRIMARY KEY AUTOINCREMENT,
                license_plate VARCHAR(20) NOT NULL UNIQUE,
                brand VARCHAR(50) NOT NULL,
                model VARCHAR(50) NOT NULL,
                description VARCHAR(255) NOT NULL,
                vehicle_type VARCHAR(20) NOT NULL,
                status VARCHAR(20) NOT NULL,
                mileage FLOAT NOT NULL,
                daily_rate DOUBLE NOT NULL,
                security_deposit DOUBLE NOT NULL,
                insurance_rate FLOAT NOT NULL
            );
            INSERT INTO Vehicles_new
                (license_plate, brand, model, description, vehicle_type, status, mileage,
                 daily_rate, security_deposit, insurance_rate)
            SELECT license_plate, brand, model, description, vehicle_type, status, mileage,
                   daily_rate, security_deposit, insurance_rate
            FROM Vehicles
            ORDER BY rowid;
            CREATE TEMPORARY TABLE VehicleIdMap AS
                SELECT old.vehicle_id AS old_id, new.vehicle_id AS new_id
                FROM Vehicles old
                JOIN Vehicles_new new ON old.license_plate = new.license_plate;
            UPDATE Transactions
            SET vehicle_id = (
                SELECT new_id FROM VehicleIdMap
                WHERE old_id = Transactions.vehicle_id
            )
            WHERE vehicle_id IN (SELECT old_id FROM VehicleIdMap);
            UPDATE Damage_logs
            SET vehicle_id = (
                SELECT new_id FROM VehicleIdMap
                WHERE old_id = Damage_logs.vehicle_id
            )
            WHERE vehicle_id IN (SELECT old_id FROM VehicleIdMap);
            DROP TABLE Vehicles;
            ALTER TABLE Vehicles_new RENAME TO Vehicles;
            DROP TABLE VehicleIdMap;
            COMMIT;
            PRAGMA foreign_keys = ON;
        )sql");
    }
}
