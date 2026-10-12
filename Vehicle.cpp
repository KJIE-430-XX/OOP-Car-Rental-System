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
    assignedCustomer_(),
    // int or double - direct copying is fine because they are small primitive values
    mileage_(mileage),
    dailyRate_(dailyRate),
    securityDeposit_(securityDeposit),
    insuranceRate_(insuranceRate) {}


// Getters -- Use this member function to return private data, so we can use Vehicle.id (function call) to access them
const string& Vehicle::id() const { return id_; }
const string& Vehicle::plate() const { return plate_; }
const string& Vehicle::brand() const { return brand_; }
const string& Vehicle::model() const { return model_; }
const string& Vehicle::description() const { return description_; }
const string& Vehicle::status() const { return status_; }
const string& Vehicle::assignedCustomer() const { return assignedCustomer_; }
bool Vehicle::isAvailable() const { return status_ == "Available"; }
double Vehicle::mileage() const { return mileage_; }
double Vehicle::dailyRate() const { return dailyRate_; }
double Vehicle::securityDeposit() const { return securityDeposit_; }
double Vehicle::insuranceRate() const { return insuranceRate_; }

bool Vehicle::rentVehicle(const string& customerId) {
    if (!isAvailable() || customerId.empty()) {
        return false;
    }

    assignedCustomer_ = customerId;
    status_ = "Rented";
    return true;
}

bool Vehicle::returnVehicle(double returnedMileage) {
    if (status_ != "Rented" || returnedMileage < mileage_) {
        return false;
    }

    mileage_ = returnedMileage;
    assignedCustomer_.clear();
    status_ = "Available";
    return true;
}

bool Vehicle::markUnderMaintenance() {
    if (status_ != "Available") {
        return false;
    }

    status_ = "Under_Maintenance";
    return true;
}

bool Vehicle::markAvailable() {
    if (status_ != "Under_Maintenance") {
        return false;
    }

    status_ = "Available";
    return true;
}

Vehicle* RentalSystem::findVehicle(const string& vehicleId) {
    const auto found = vehicleMap_.find(vehicleId);
    return found == vehicleMap_.end() ? nullptr : found->second.get();
}

vector<Vehicle*> RentalSystem::findVehiclesByCategory(const string& category) const {
    vector<Vehicle*> result;
    for (const auto& entry : vehicleMap_) {
        if (category.empty() || entry.second->category() == category) {
            result.push_back(entry.second.get());
        }
    }
    return result;
}

bool RentalSystem::rentVehicle(const string& vehicleId, const string& customerId) {
    Vehicle* vehicle = findVehicle(vehicleId);
    return vehicle != nullptr && vehicle->rentVehicle(customerId);
}

bool RentalSystem::returnVehicle(const string& vehicleId, double returnedMileage) {
    Vehicle* vehicle = findVehicle(vehicleId);
    return vehicle != nullptr && vehicle->returnVehicle(returnedMileage);
}

// The derived-class overrides
string StandardCar::category() const { return "StandardCar"; }

string LuxuryCar::category() const { return "LuxuryCar"; }

string SUV::category() const { return "SUV"; }
