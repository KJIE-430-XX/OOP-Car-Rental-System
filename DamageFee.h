#pragma once

#include <string>

// Declare Damage Fee Calculator Class
class DamageFeeCalculator {
public:
    // Virtual destructor ensures that the object is destroyed correctly
    // even when its exact derived type is accessed through the base class.
    virtual ~DamageFeeCalculator() = default;
    // Virtual function for overriding
    virtual double feeFor(const std::string& component) const = 0;
};

// Standard Car Devired Class
class StandardCarDamageFee final : public DamageFeeCalculator {
public:
    double feeFor(const std::string& component) const override;
};

// Luxury Car Devired Class
class LuxuryCarDamageFee final : public DamageFeeCalculator {
public:
    double feeFor(const std::string& component) const override;
};

// SUV Devired Class
class SUVDamageFee final : public DamageFeeCalculator {
public:
    double feeFor(const std::string& component) const override;
};


// Declare baseDamageFee function
double baseDamageFee(const std::string& component);
