#include "Vehicle.h"

#include <utility>

using namespace std;

// All under class Vehicle (Vehicle::)
// The constructor - std::move allows us to move the parameter into the Vehicle object efficiently, so the data remains in the object after the constructor finishes.
Vehicle::Vehicle(
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
) : id_(std::move(id)),
    plate_(std::move(plate)),
brand_(std::move(brand)),
model_(std::move(model)),
    description_(std::move(description)),
    status_(std::move(status)),
    // int or double - direct copying is fine because they are small primitive values
    mileage_(mileage),
    dailyRate_(dailyRate),
    securityDeposit_(securityDeposit),
    insuranceRate_(insuranceRate) {}


// Getters -- Use this function to return private data, so we can use Vehicle.id to access them
const string& Vehicle::id() const { return id_; }
const string& Vehicle::plate() const { return plate_; }
const string& Vehicle::brand() const { return brand_; }
const string& Vehicle::model() const { return model_; }
const string& Vehicle::description() const { return description_; }
const string& Vehicle::status() const { return status_; }
double Vehicle::mileage() const { return mileage_; }
double Vehicle::dailyRate() const { return dailyRate_; }
double Vehicle::securityDeposit() const { return securityDeposit_; }
double Vehicle::insuranceRate() const { return insuranceRate_; }

// The derived-class overrides
string StandardCar::category() const { return "StandardCar"; }

string LuxuryCar::category() const { return "LuxuryCar"; }

string SUV::category() const { return "SUV"; }
