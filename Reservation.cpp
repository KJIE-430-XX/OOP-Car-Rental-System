#include "Reservation.h"

#include <ctime>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

using namespace std;

Reservation::Reservation()
    : customerId_(0), totalFee_(0.0) {}

// The constructor - std::move allows us to move the parameter into the Reservation object efficiently, so the data remains in the object after the constructor finishes.
Reservation::Reservation(
    string reservationId,
    string vehicleId,
    int customerId,
    string startDate,
    string endDate,
    const Vehicle& vehicle
) : reservationId_(std::move(reservationId)),
    vehicleId_(std::move(vehicleId)),
    customerId_(customerId),
    startDate_(std::move(startDate)),
    endDate_(std::move(endDate)),
    totalFee_(vehicle.calculateRentalFee(daysBetween(startDate_, endDate_))) {}

// Getter - Use this member function to return private data, so we can use Reservation.id (function call) to access them
const string& Reservation::id() const { return reservationId_; }
const string& Reservation::vehicleId() const { return vehicleId_; }
int Reservation::customerId() const { return customerId_; }
const string& Reservation::startDate() const { return startDate_; }
const string& Reservation::endDate() const { return endDate_; }
double Reservation::totalFee() const { return totalFee_; }

// Helper function - bool return True / False - receives a string date
bool Reservation::validDate(const string& date) {

    // Check the date format: YYYY-MM-DD, for example 2026-10-12.
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }

    // Loop: check the character exist one by one, return false if found
    // Fail e.g. 2026-1A-12 || 202A-10-12
    for (size_t index = 0; index < date.size(); ++index) {
        if (index != 4 && index != 7 &&
            (date[index] < '0' || date[index] > '9')) {
            // If not meet the criteria, return false
            return false;
        }
    }

    // Extract the month and date
    // substr(start, length)
    const int month = stoi(date.substr(5, 2));
    const int day = stoi(date.substr(8, 2));

    // Return true only if meet all requirements
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

// Helper function: Convert the date into a number
// 2026-10-12 --> 2026×10000 + 10×100 + 12 = 20261012
// Make comparison easier later
int Reservation::dateValue(const string& date) {
    return stoi(date.substr(0, 4)) * 10000
        + stoi(date.substr(5, 2)) * 100
        + stoi(date.substr(8, 2));
}

// Helper function: Calculate the number of days
int Reservation::daysBetween(
    const string& startDate,
    const string& endDate
) {
    // Building the start date
    tm start = {}; // tm is the C struct from <ctime> that holds a broken-down calendar time
    start.tm_year = stoi(startDate.substr(0, 4)) - 1900; // If not subtract 1900, it = 3926 (2026+1900)
    start.tm_mon = stoi(startDate.substr(5, 2)) - 1; // Get the month, January = 0, so 1 - 1 = 0
    start.tm_mday = stoi(startDate.substr(8, 2)); // Get the days
    start.tm_hour = 12;

    // Building the end date
    tm end = {};
    end.tm_year = stoi(endDate.substr(0, 4)) - 1900;
    end.tm_mon = stoi(endDate.substr(5, 2)) - 1;
    end.tm_mday = stoi(endDate.substr(8, 2));
    end.tm_hour = 12;

    // Converting to timestamps
    // mktime converts a tm into a time_t
    const time_t startTime = mktime(&start);
    const time_t endTime = mktime(&end);
    // / (60 * 60 * 24) → divides by 86,400 to get days
    // Seconds --> Days
    return static_cast<int>(difftime(endTime, startTime) / (60 * 60 * 24)) + 1; 
    // +1 is because including the first day
    // e.g. Start 2026-10-12, end 2026-10-15 :: three days different but duration is 4 (3+1)
}

// Member function: checks whether the start date and end date are valid and the end date is not earlier than the start date.
bool Reservation::validDateRange(
    const string& startDate,
    const string& endDate
) {
    return validDate(startDate) && validDate(endDate)
        && dateValue(endDate) >= dateValue(startDate);
}

// Member function: Determine a reservation's status based on the current date and get today's date automatically from the computer.
string Reservation::statusForDate(
    const string& startDate,
    const string& endDate,
    const string& today
) {
    if (dateValue(today) >= dateValue(startDate)) {
        if (dateValue(today) > dateValue(endDate)) {
            return "Overdue";
        }
        return "Active";
    }
    return "Confirmed";
}

// Member Function: Automatically gets the current local date from the computer and returns it as a string in YYYY-MM-DD format.
string Reservation::todayDate() {
    const time_t now = time(nullptr);
    tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    ostringstream date;
    date << put_time(&localTime, "%Y-%m-%d");
    return date.str();
}

// Calculate the overdue days - end date between checkout date.
int Reservation::overdueDays(
    const string& endDate,
    const string& checkoutDate
) {
    return max(0, daysBetween(endDate, checkoutDate) - 1);
}
