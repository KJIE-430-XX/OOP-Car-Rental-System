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
    // Check whether the vehicle is available and the customer ID is not empty
    if (!isAvailable() || customerId.empty()) {
        return false;
    }

    // Assign the vehicle to the customer
    assignedCustomer_ = customerId;
    // Update the vehicle status to Rented
    status_ = "Rented";
    return true;
}

bool Vehicle::returnVehicle(double returnedMileage) {

    // Check whether the vehicle is rented and the returned mileage is valid
    if (status_ != "Rented" || returnedMileage < mileage_) {
        return false;
    }

    // Update the vehicle's mileage to the returned mileage
    mileage_ = returnedMileage;
    // Clear the assigned customer's ID because the rental has ended
    assignedCustomer_.clear();
    // Update the vehicle status back to Available
    status_ = "Available";
    return true;
}

bool Vehicle::markUnderMaintenance() {
    // Only an available vehicle can be marked for maintenance
    if (status_ != "Available") {
        return false;
    }

    // Update the vehicle status to Under Maintenance
    status_ = "Under_Maintenance";
    return true;
}

bool Vehicle::markAvailable() {
    // Only a vehicle under maintenance can be marked as available
    if (status_ != "Under_Maintenance") {
        return false;
    }

    // Update the vehicle status to Available
    status_ = "Available";
    return true;
}

void RentalSystem::addVehicle(unique_ptr<Vehicle> vehicle) {
    // Check that the unique_ptr owns a valid Vehicle object
    if (vehicle) {
        // Store the vehicle using its ID as the key
        // std::move transfers ownership of the unique_ptr into the map
        vehicleMap_[vehicle->id()] = std::move(vehicle);
    }
}

void RentalSystem::removeVehicle(const string& vehicleId) {
    // Remove the vehicle entry with the specified ID from the map
    vehicleMap_.erase(vehicleId);
}

Vehicle* RentalSystem::findVehicle(const string& vehicleId) {
    // Search the map for the specified vehicle ID
    const auto found = vehicleMap_.find(vehicleId);

    // Return nullptr if the vehicle is not found
    // Otherwise, return a raw pointer to the Vehicle object
    return found == vehicleMap_.end() ? nullptr : found->second.get();
}

// vector<Vehicle*> RentalSystem::findVehiclesByCategory(const string& category) const {
//     vector<Vehicle*> result;
//     for (const auto& entry : vehicleMap_) {
//         if (category.empty() || entry.second->category() == category) {
//             result.push_back(entry.second.get());
//         }
//     }
//     return result;
// }

bool RentalSystem::rentVehicle(const string& vehicleId, const string& customerId) {

    // Find the vehicle using its ID
    Vehicle* vehicle = findVehicle(vehicleId);

    // Check that the vehicle exists before calling its rentVehicle() function
    // Return false if the vehicle is not found
    // Otherwise, return the result of the vehicle class's rental operation
    return vehicle != nullptr && vehicle->rentVehicle(customerId);
}

bool RentalSystem::returnVehicle(const string& vehicleId, double returnedMileage) {

    // Find the vehicle using its ID
    Vehicle* vehicle = findVehicle(vehicleId);

    // Check that the vehicle exists before calling its returnVehicle() function
    // Return false if the vehicle is not found
    // Otherwise, return the result of the vehicle class's return operation
    return vehicle != nullptr && vehicle->returnVehicle(returnedMileage);
}

// The derived-class overrides
string StandardCar::category() const { return "StandardCar"; }

string LuxuryCar::category() const { return "LuxuryCar"; }

string SUV::category() const { return "SUV"; }
