#pragma once

#include <memory>
#include <string>

// Calculates the fee for the number of days a vehicle was returned late.
class OverdueFeeCalculator {
public:
    virtual ~OverdueFeeCalculator() = default;
    virtual double feeFor(int overdueDays) const = 0; // Virtual function for overriding
};

// Derived class from OverdueFeeCalculator for each vehicle category
class StandardCarOverdueFee final : public OverdueFeeCalculator {
public:
    double feeFor(int overdueDays) const override;
};

class LuxuryCarOverdueFee final : public OverdueFeeCalculator {
public:
    double feeFor(int overdueDays) const override;
};

class SUVOverdueFee final : public OverdueFeeCalculator {
public:
    double feeFor(int overdueDays) const override;
};

std::unique_ptr<OverdueFeeCalculator> makeOverdueFeeCalculator(
    const std::string& category
);
