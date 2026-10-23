#include "DamageFee.h"

#include <unordered_map>

// Defines a constant lookup table : stores the estimated damage fee for each vehicle component
// It allow compiler found the component via "componentFees.find(Front bumper)"
namespace {
const std::unordered_map<std::string, double> componentFees = {
    {"Front bumper", 450.0}, {"Front grille", 300.0}, {"Front bonnet / hood", 700.0},
    {"Left headlight", 550.0}, {"Right headlight", 550.0}, {"Front windshield", 650.0},
    {"Front number plate", 120.0}, {"Front-left door", 900.0}, {"Front-right door", 900.0},
    {"Rear-left door", 900.0}, {"Rear-right door", 900.0}, {"Left side mirror", 280.0},
    {"Right side mirror", 280.0}, {"Left side panel / fender", 600.0},
    {"Right side panel / fender", 600.0}, {"Side skirts", 400.0}, {"Rear bumper", 450.0},
    {"Left taillight", 400.0}, {"Right taillight", 400.0}, {"Boot / trunk / tailgate", 850.0},
    {"Rear windshield", 650.0}, {"Rear number plate", 120.0},
    {"Front-left tyre and rim", 500.0}, {"Front-right tyre and rim", 500.0},
    {"Rear-left tyre and rim", 500.0}, {"Rear-right tyre and rim", 500.0},
    {"Spare tyre (if provided)", 350.0}, {"Dashboard", 750.0}, {"Steering wheel", 450.0},
    {"Front seats", 650.0}, {"Rear seats", 650.0}, {"Seat belts", 250.0},
    {"Centre console", 500.0}, {"Interior door panels", 350.0},
    {"Floor mats and carpet", 300.0}, {"Roof lining", 500.0}, {"Boot interior", 350.0},
    {"Engine warning lights", 600.0}, {"Engine oil / fluid leakage", 850.0},
    {"Visible engine compartment damage", 1200.0}, {"Air conditioning", 900.0},
    {"Windscreen wipers", 180.0}, {"Door handles and locks", 300.0},
    {"Windows and window mechanisms", 450.0}, {"Side indicators and other lights", 250.0}
};
}

// Define the baseDamageFee function
double baseDamageFee(const std::string& component) {
    const auto found = componentFees.find(component); // Find the vehicle component
    return found == componentFees.end() ? 0.0 : found->second; // If not found, then it is 0.0, else the value of the component
}

// Define three derived classes' member function to call baseDamageFee for calculating the damage fee
double StandardCarDamageFee::feeFor(const std::string& component) const {
    return baseDamageFee(component); // Ori damage fee for Standard Car
}

double LuxuryCarDamageFee::feeFor(const std::string& component) const {
    return baseDamageFee(component) * 1.5; // Damage fee x 1.5 for Luxury Car
}

double SUVDamageFee::feeFor(const std::string& component) const {
    return baseDamageFee(component) * 1.15; // Damage fee x 1.15 for SUV
}
