#pragma once // Prevent redefinition of class Vehicle

#include <string>

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
    double mileage() const;
    double dailyRate() const;
    double securityDeposit() const;
    double insuranceRate() const;

    // virtual: the call is resolved at runtime based on the actual object type.
    // = 0: makes it pure virtual, meaning the base class provides no implementation and every concrete derived class must override it.
    // Vehicle v; is invalid
    // Vehicle* v = new SUV(); is valid
    virtual string category() const = 0;

private:
    // Encapsulation: Only member function can access them
    string id_;
    string plate_;
    string brand_;
    string model_;
    string description_;
    string status_;
    double mileage_;
    double dailyRate_;
    double securityDeposit_;
    double insuranceRate_;
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
