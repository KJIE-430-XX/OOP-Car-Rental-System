#include "Vehicle.h"

#include "Reservation.h"
#include "OverdueFee.h"
#include <SQLiteCpp/SQLiteCpp.h>
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

    // A confirmed reservation may leave the vehicle Available until checkout.
    if (status_ == "Under_Maintenance" || returnedMileage < mileage_) {
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

// Overloading markUnderMaintenance function with four parameters
bool Vehicle::markUnderMaintenance(
    SQLite::Database& database,
    int administratorId,
    const string& administratorUsername,
    const string& notes
) {
    // Return false if the status is "Available"
    if (status_ != "Available") {
        return false;
    }

    // Query to select previous maintenance record
    // Use the previous record for forming a complete maintenance history
    SQLite::Statement previous(
        database,
        "SELECT maintenance_id FROM Maintenance_records "
        "WHERE vehicle_id = ? ORDER BY maintenance_id DESC LIMIT 1"
    );
    previous.bind(1, id_);
    const bool hasPrevious = previous.executeStep();

    // Query insert new maintenance record
    SQLite::Statement record(
        database,
        "INSERT INTO Maintenance_records "
        "(vehicle_id, started_by_admin_id, started_by_username, notes, previous_maintenance_id) "
        "VALUES (?, ?, ?, ?, ?)"
    );
    record.bind(1, id_);
    record.bind(2, administratorId);
    record.bind(3, administratorUsername);
    record.bind(4, notes);
    if (hasPrevious) {
        record.bind(5, previous.getColumn(0).getInt()); // Insert the previous id into the new record
    } else {
        record.bind(5);
    }
    record.exec();

    // Query to update the vehicle status to Under_Maintenance
    SQLite::Statement updateVehicle(
        database,
        "UPDATE Vehicles SET status = 'Under_Maintenance' WHERE vehicle_id = ?"
    );
    updateVehicle.bind(1, id_);
    updateVehicle.exec();
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

// Overloading markAvailable function with four parameters
bool Vehicle::markAvailable(
    SQLite::Database& database,
    int administratorId,
    const string& administratorUsername,
    const string& completionNotes
) {
    if (status_ != "Under_Maintenance") {
        return false;
    }

    // Update maintenance record's end date, who edit it and maintenance notes 
    SQLite::Statement record(
        database,
        "UPDATE Maintenance_records "
        "SET ended_at = datetime('now'), ended_by_admin_id = ?, ended_by_username = ?, "
        "completion_notes = ? "
        "WHERE vehicle_id = ? AND ended_at IS NULL"
    );
    record.bind(1, administratorId);
    record.bind(2, administratorUsername);
    record.bind(3, completionNotes);
    record.bind(4, id_);
    record.exec();
    if (database.getTotalChanges() != 1) {
        return false;
    }

    // Update vehicle status back to Available.
    SQLite::Statement updateVehicle(
        database,
        "UPDATE Vehicles SET status = 'Available' WHERE vehicle_id = ?"
    );
    updateVehicle.bind(1, id_);
    updateVehicle.exec();
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

// Perform the create reservation function
RentalOperationResult RentalSystem::createReservation(
    SQLite::Database& database,
    const string& vehicleId,
    int customerId,
    const string& startDate,
    const string& endDate,
    Reservation& reservation
) {
    // Validate and return specific result such as InvalidDates
    if (!Reservation::validDateRange(startDate, endDate)) {
        return RentalOperationResult::InvalidDates;
    }

    Vehicle* vehicle = findVehicle(vehicleId);
    if (!vehicle) {
        return RentalOperationResult::NotFound;
    }
    if (vehicle->status() == "Under_Maintenance") {
        return RentalOperationResult::UnderMaintenance;
    }

    // Check whether current user already book the vehicle A already
    SQLite::Statement activeReservation(
        database,
        "SELECT 1 FROM Reservations "
        "WHERE vehicle_id = ? AND customer_id = ? "
        "AND status IN ('Confirmed', 'Active') LIMIT 1"
    );
    activeReservation.bind(1, vehicleId);
    activeReservation.bind(2, customerId);
    if (activeReservation.executeStep()) {
        return RentalOperationResult::DuplicateReservation;
    }

    // If found got overlapping reseravation or not
    SQLite::Statement overlappingReservation(
        database,
        "SELECT 1 FROM Reservations "
        "WHERE vehicle_id = ? AND status != 'Completed' "
        "AND start_date <= ? AND end_date >= ? LIMIT 1"
    );
    overlappingReservation.bind(1, vehicleId);
    overlappingReservation.bind(2, endDate);
    overlappingReservation.bind(3, startDate);
    if (overlappingReservation.executeStep()) {
        return RentalOperationResult::DateUnavailable;
    }

    // Create reservation if above validations all passed.
    // Call constructor and related member-function to calculate the total fee
    reservation = Reservation(
        "", vehicleId, customerId, startDate, endDate, *vehicle);
    SQLite::Transaction transaction(database);
    SQLite::Statement insert(
        database,
        "INSERT INTO Reservations "
        "(customer_id, vehicle_id, start_date, end_date, total_fee, status) "
        "VALUES (?, ?, ?, ?, ?, 'Confirmed')"
    );
    insert.bind(1, customerId);
    insert.bind(2, vehicleId);
    insert.bind(3, reservation.startDate());
    insert.bind(4, reservation.endDate());
    insert.bind(5, reservation.totalFee());
    insert.exec();
    transaction.commit();

    // Return success message
    return RentalOperationResult::Success;
}

// Perform the checkout vehicle function
RentalOperationResult RentalSystem::checkoutVehicle(
    SQLite::Database& database,
    int reservationId,
    double returnedMileage,
    int customerId,
    bool administrator
) {
    // Check the reservation has exist or not before checkout
    SQLite::Statement reservationQuery(
        database,
        "SELECT vehicle_id, customer_id, status, end_date FROM Reservations "
        "WHERE reservation_id = ?"
    );
    reservationQuery.bind(1, reservationId);
    if (!reservationQuery.executeStep()) {
        return RentalOperationResult::NotFound;
    }
    // Only admin or owner can checkout the vehicle
    if (!administrator && reservationQuery.getColumn(1).getInt() != customerId) {
        return RentalOperationResult::Unauthorized;
    }
    const string reservationStatus = reservationQuery.getColumn(2).getString();
    if (reservationStatus != "Confirmed"
        && reservationStatus != "Active"
        && reservationStatus != "Overdue") {
        return RentalOperationResult::AlreadyCompleted;
    }


    // Check the vehicle has exist or not from in-memory hash map
    const string vehicleId = reservationQuery.getColumn(0).getString();
    Vehicle* vehicle = findVehicle(vehicleId);
    if (!vehicle) {
        return RentalOperationResult::NotFound;
    }
    // Ensure the mileage is not less than original mileage
    if (returnedMileage < vehicle->mileage()
        || vehicle->status() == "Under_Maintenance") {
        return RentalOperationResult::InvalidMileage;
    }
    // If status = overdue, then call makeOverdueFeeCalculator based on the category to calculate the overdue fee
    double overdueFee = 0.0;
    if (reservationStatus == "Overdue") {
        auto calculator = makeOverdueFeeCalculator(vehicle->category());
        if (!calculator) {
            return RentalOperationResult::DatabaseError;
        }
        overdueFee = calculator->feeFor(
            Reservation::overdueDays(
                reservationQuery.getColumn(3).getString(),
                Reservation::todayDate()
            )
        );
    }

    // Update the vehicle status to 'Pending Inspection'
    SQLite::Transaction transaction(database);
    SQLite::Statement updateVehicle(
        database,
        "UPDATE Vehicles SET status = 'Pending Inspection', mileage = ? WHERE vehicle_id = ?"
    );
    updateVehicle.bind(1, returnedMileage);
    updateVehicle.bind(2, vehicleId);
    updateVehicle.exec();

    // Update the reservation status to 'Pending Inspection' and checkout date is now
    SQLite::Statement updateReservation(
        database,
        "UPDATE Reservations SET status = 'Pending Inspection', checkout_date = date('now'), "
        "return_mileage = ?, overdue_fee = ? "
        "WHERE reservation_id = ? AND status IN ('Confirmed', 'Active', 'Overdue')"
    );
    updateReservation.bind(1, returnedMileage);
    updateReservation.bind(2, overdueFee);
    updateReservation.bind(3, reservationId);
    updateReservation.exec();

    // Retrieve the updated row, if not return, then system return AlreadyCompleted
    SQLite::Statement changedRows(database, "SELECT changes()");
    if (!changedRows.executeStep() || changedRows.getColumn(0).getInt() != 1) {
        return RentalOperationResult::AlreadyCompleted;
    }

    // Confirms the transaction and commits its changes.
    transaction.commit();
    // Return Success Message
    return RentalOperationResult::Success;
}

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

// Perform rent a vehicle
bool RentalSystem::rentVehicle(
    SQLite::Database& database,
    const string& vehicleId,
    const string& customerId
) {
    // Find the vehicle from in-memory hash map
    Vehicle* vehicle = findVehicle(vehicleId);
    if (!vehicle || !vehicle->isAvailable() || customerId.empty()) {
        return false;
    }

    SQLite::Transaction transaction(database);
    // Update the vehicle into 'Rented'
    SQLite::Statement update(
        database,
        "UPDATE Vehicles SET status = ? WHERE vehicle_id = ?"
    );
    update.bind(1, "Rented");
    update.bind(2, vehicleId);
    update.exec();
    if (!vehicle->rentVehicle(customerId)) {
        return false;
    }
    transaction.commit();
    return true;
}

// Perform return vehicle
bool RentalSystem::returnVehicle(
    SQLite::Database& database,
    const string& vehicleId,
    double returnedMileage
) {
    // Find the vehicle from in-memory hash map
    Vehicle* vehicle = findVehicle(vehicleId);
    if (!vehicle || vehicle->status() == "Under_Maintenance"
        || returnedMileage < vehicle->mileage()) {
        return false;
    }

    SQLite::Transaction transaction(database);
    // Update vehicle into 'Available'
    SQLite::Statement update(
        database,
        "UPDATE Vehicles SET status = ?, mileage = ? WHERE vehicle_id = ?"
    );
    update.bind(1, "Available");
    update.bind(2, returnedMileage);
    update.bind(3, vehicleId);
    update.exec();

    // Return false if the milegae is invalid
    if (!vehicle->returnVehicle(returnedMileage)) {
        return false;
    }
    transaction.commit();
    return true;
}

// The derived-class overrides
string StandardCar::category() const { return "StandardCar"; }

// Override for calculateRentalFee (Standard Car)
double StandardCar::calculateRentalFee(int rentalDays) const {
    if (rentalDays <= 0) {
        return 0.0;
    }

    const double baseFee = dailyRate() * rentalDays;
    return rentalDays >= 7 ? baseFee * 0.90 : baseFee;
}

string LuxuryCar::category() const { return "LuxuryCar"; }

// Override for calculateRentalFee (Luxury Car)
double LuxuryCar::calculateRentalFee(int rentalDays) const {
    if (rentalDays <= 0) {
        return 0.0;
    }

    return dailyRate() * rentalDays
        + securityDeposit()
        + insuranceRate() * rentalDays;
}

string SUV::category() const { return "SUV"; }

// Override for calculateRentalFee (SUV)
double SUV::calculateRentalFee(int rentalDays) const {
    if (rentalDays <= 0) {
        return 0.0;
    }

    const double baseFee = dailyRate() * rentalDays;

    // If rental day larger than 7 days, then discount 15%
    return rentalDays >= 7 ? baseFee * 0.85 : baseFee;
}
