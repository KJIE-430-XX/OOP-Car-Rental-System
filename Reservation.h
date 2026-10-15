#pragma once

#include <string>

using namespace std;

// Declaring the reservation class
class Reservation {
public:

    // Declaring the constructor
    Reservation(
        // Parameters
        string reservationId,
        string vehicleId,
        int customerId,
        string startDate,
        string endDate,
        double dailyRate
    );

    // Public member function declarations (Read-only references)
    const string& id() const;
    const string& vehicleId() const;
    int customerId() const;
    const string& startDate() const;
    const string& endDate() const;
    double totalFee() const;

    static bool validDateRange(
        const string& startDate,
        const string& endDate
    );
    static string statusForDate(
        const string& startDate,
        const string& endDate,
        const string& today
    );
    static string todayDate();

private:

    // Private helper function declarations
    static int dateValue(const string& date);
    static int daysBetween(
        const string& startDate,
        const string& endDate
    );
    static bool validDate(const string& date);

    // Private member variables
    string reservationId_;
    string vehicleId_;
    int customerId_;
    string startDate_;
    string endDate_;
    double totalFee_;
};