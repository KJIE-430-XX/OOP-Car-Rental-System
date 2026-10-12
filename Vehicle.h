#pragma once // Prevent redefinition of class Vehicle

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// Base class: Vehicle
class Vehicle {
public:
    // Constructor: Vehicle data
    Vehicle(
        string id,
        string plate,
        string brand,
        string model,
        string description,
        string status,
        double mileage,
        double dailyRate,
        double securityDeposit,
        double insuranceRate
    );
    virtual ~Vehicle() = default; // Ensures safe cleanup for all derived classes; virtual is necessary

    // Return reference instead of copying it, const to prevent caller modify the value
    const string& id() const; 
    const string& plate() const;
    const string& brand() const;
    const string& model() const;
    const string& description() const;
    const string& status() const;
    const string& assignedCustomer() const;
    bool isAvailable() const;
    double mileage() const;
    double dailyRate() const;
    double securityDeposit() const;
    double insuranceRate() const;

    // virtual: the call is resolved at runtime based on the actual object type.
    // = 0: makes it pure virtual, meaning the base class provides no implementation and every concrete derived class must override it.
    // Vehicle v; is invalid
    // Vehicle* v = new SUV(); is valid
    virtual string category() const = 0;

    bool rentVehicle(const string& customerId);
    bool returnVehicle(double returnedMileage);
    bool markUnderMaintenance();
    bool markAvailable();

private:
    // Encapsulation: Only member function can access them
    string id_;
    string plate_;
    string brand_;
    string model_;
    string description_;
    string status_;
    string assignedCustomer_;
    double mileage_;
    double dailyRate_;
    double securityDeposit_;
    double insuranceRate_;
};

// Declaring the RentalSystem class
class RentalSystem {
public:
    Vehicle* findVehicle(const string& vehicleId);
    vector<Vehicle*> findVehiclesByCategory(const string& category) const;

    bool rentVehicle(const string& vehicleId, const string& customerId);
    bool returnVehicle(const string& vehicleId, double returnedMileage);

private:
    unordered_map<string, unique_ptr<Vehicle>> vehicleMap_;
};

// Derived class for Vehicle
// final: nothing can inherit from them
class StandardCar final : public Vehicle {
public:
    // This is an inheriting constructor
    // It pulls all of Vehicle's constructors into StandardCar
    using Vehicle::Vehicle;
    string category() const override;
};

class LuxuryCar final : public Vehicle {
public:
    using Vehicle::Vehicle;
    string category() const override;
};

class SUV final : public Vehicle {
public:
    using Vehicle::Vehicle;
    string category() const override;
};
