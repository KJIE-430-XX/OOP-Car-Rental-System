#pragma once

#include <string>

void initializeDatabase(const std::string& databasePath);

struct CustomerRegistrationResult {
	bool success;
	int customerId;
	std::string message;
};

struct LoginResult {
	bool success;
	bool administrator;
	int customerId;
	std::string name;
	std::string message;
};

CustomerRegistrationResult registerCustomer(
	const std::string& databasePath,
	const std::string& username,
	const std::string& password,
	const std::string& name,
	const std::string& phoneNumber
);

LoginResult authenticateUser(
	const std::string& databasePath,
	const std::string& username,
	const std::string& password
);