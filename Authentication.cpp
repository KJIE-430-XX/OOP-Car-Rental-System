#include "Authentication.h"

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

CustomerAuthenticationProvider::CustomerAuthenticationProvider(
    const string& databasePath
) : databasePath(databasePath) {}

LoginResult CustomerAuthenticationProvider::login(
    const string& username,
    const string& password
) const {
    try {
        SQLite::Database database = openDatabase(databasePath);
        SQLite::Statement query(
            database,
            "SELECT customer_id, name FROM Customers "
            "WHERE username = ? AND password = ?"
        );
        query.bind(1, username);
        query.bind(2, password);

        if (query.executeStep()) {
            return {
                true,
                false,
                query.getColumn(0).getInt(),
                query.getColumn(1).getString(),
                "Login successful."
            };
        }
    } catch (const SQLite::Exception&) {
        return {false, false, 0, "", "Unable to access the database."};
    }

    return {false, false, 0, "", "Invalid username or password."};
}

AdministratorAuthenticationProvider::AdministratorAuthenticationProvider(
    const string& databasePath
) : databasePath(databasePath) {}

LoginResult AdministratorAuthenticationProvider::login(
    const string& username,
    const string& password
) const {
    try {
        SQLite::Database database = openDatabase(databasePath);
        SQLite::Statement query(
            database,
            "SELECT admin_id FROM Administrators "
            "WHERE username = ? AND password = ?"
        );
        query.bind(1, username);
        query.bind(2, password);

        if (query.executeStep()) {
            return {
                true,
                true,
                query.getColumn(0).getInt(),
                "Administrator",
                "Administrator login successful."
            };
        }
    } catch (const SQLite::Exception&) {
        return {false, false, 0, "", "Unable to access the database."};
    }

    return {false, false, 0, "", "Invalid username or password."};
}

// Create authentication providers for customer and administrator
AuthenticationService::AuthenticationService(const string& databasePath)
    : databasePath(databasePath) {
    providers.push_back(
        make_unique<AdministratorAuthenticationProvider>(databasePath)
    );
    providers.push_back(
        make_unique<CustomerAuthenticationProvider>(databasePath)
    );
}

CustomerRegistrationResult AuthenticationService::registerAccount(
    const string& username,
    const string& password,
    const string& name,
    const string& phoneNumber
) const {
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
        if (string(exception.what()).find("UNIQUE constraint failed") != string::npos) {
            return {false, 0, "That username is already registered."};
        }
        return {false, 0, "Unable to register the account."};
    }
}

LoginResult AuthenticationService::login(
    const string& username,
    const string& password
) const {

    // Run two provider, AdministratorAuthenticationProvider first, then CustomerAuthenticationProvider
    for (const auto& provider : providers) {
        LoginResult result = provider->login(username, password);
        if (result.success || result.message == "Unable to access the database.") {
            return result;
        }
    }

    return {false, false, 0, "", "Invalid username or password."};
}