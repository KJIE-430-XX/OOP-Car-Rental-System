#include "OverdueFee.h"

#include <algorithm>

namespace {
double feeAtRate(int overdueDays, double dailyRate) {
    return static_cast<double>(std::max(0, overdueDays)) * dailyRate;
}
}

double StandardCarOverdueFee::feeFor(int overdueDays) const {
    return feeAtRate(overdueDays, 50.0); // 50 for daily penalty (2 days = 2 x 50 = 100)
}

double LuxuryCarOverdueFee::feeFor(int overdueDays) const {
    return feeAtRate(overdueDays, 100.0); // 100 for daily penalty (2 days = 2 x 100 = 200)
}

double SUVOverdueFee::feeFor(int overdueDays) const {
    return feeAtRate(overdueDays, 75.0); // 75 for daily penalty (2 days = 2 x 75 = 150)
}

std::unique_ptr<OverdueFeeCalculator> makeOverdueFeeCalculator(
    const std::string& category
) {
    if (category == "StandardCar") return std::make_unique<StandardCarOverdueFee>();
    if (category == "LuxuryCar") return std::make_unique<LuxuryCarOverdueFee>();
    if (category == "SUV") return std::make_unique<SUVOverdueFee>();
    return nullptr;
}
