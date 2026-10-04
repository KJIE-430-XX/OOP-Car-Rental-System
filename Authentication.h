#pragma once

#include <memory>
#include <string>
#include <vector>

using namespace std;

struct CustomerRegistrationResult {
    bool success;
    int customerId;
    string message;
};

struct LoginResult {
    bool success;
    bool administrator;
    int customerId;
    string name;
    string message;
};

class AuthenticationProvider {
public:
    virtual ~AuthenticationProvider() = default;
    virtual LoginResult login(
        const string& username,
        const string& password
    ) const = 0;
};

// Class for Login
// Handles login for customer accounts
class CustomerAuthenticationProvider final : public AuthenticationProvider {
public:

    // Constructor (same name with class)
    // 'explicit' prevents accidental implicit conversion from string.
    explicit CustomerAuthenticationProvider(const string& databasePath);

    // login function - Checks username/password against the database. (public)
    LoginResult login(
        const string& username,
        const string& password
    ) const override;

private:

    //Private the database path
    string databasePath;
};

// Handles login for admin accounts
class AdministratorAuthenticationProvider final : public AuthenticationProvider {
public:
    explicit AdministratorAuthenticationProvider(const string& databasePath);

    LoginResult login(
        const string& username,
        const string& password
    ) const override;

private:
    string databasePath;
};

// Single entry point connect with main.cpp and contact with Authentication.cpp
class AuthenticationService {
public:

    // 'explicit' prevents accidental implicit conversion from string.
    // Avoid: AuthenticationService authenticationService = databasePath;
    // Only Allow: AuthenticationService authenticationService(databasePath);
    explicit AuthenticationService(const string& databasePath);

    // Inserts a new customer into the Customers table.
    CustomerRegistrationResult registerAccount(
        const string& username,
        const string& password,
        const string& name,
        const string& phoneNumber
    ) const;

    LoginResult login(
        const string& username,
        const string& password
    ) const;

private:
    string databasePath;
    vector<unique_ptr<AuthenticationProvider>> providers;
};