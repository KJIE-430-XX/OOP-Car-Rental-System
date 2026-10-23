# BAI22123 Object Oriented Programming
# Car-Rental-System

This project aims to develop a functional Car Rental Management System using C++ and OOP principles, with HTML, CSS, and JavaScript providing a GUI for end users, while applying appropriate data structures and algorithms to manage vehicles, customers, bookings, and rental fees.

## Developer Information

- CHOW KAI JIE BAI_A2009F-2509001

## Technologies Used

The project is built using the following technologies:

- **HTML** - Structure and markup
- **CSS** - Styling and layout
- **JavaScript** - Client-side interactivity
- **C++** - Server-side logic and backend

## Polymorphic rental fees

Rental prices are calculated through `Vehicle::calculateRentalFee(int rentalDays)`,
so the reservation flow does not need to branch on the vehicle category:

- **StandardCar**: daily rate multiplied by the rental duration, with a 10%
  discount for rentals of 7 days or more.
- **LuxuryCar**: daily rate plus the configured insurance rate for each rental
  day, with the configured security deposit added once.
- **SUV**: daily rate multiplied by the rental duration, with a 15% extended
  rental discount for rentals of 7 days or more.

The security deposit and insurance rate are stored per vehicle and are therefore
configurable when an administrator creates or edits a vehicle.

Reservation creation and checkout are coordinated by `RentalSystem`, which
owns the SQLite queries and transactions. `Vehicle` remains responsible only
for validating and changing the state of one vehicle, while `Reservation`
handles date validation and polymorphic fee calculation. Checkout updates the
vehicle and reservation records in one database transaction.

Reservation statuses are synchronized before reservation and vehicle API
operations. A confirmed reservation becomes `Active` when its start date
arrives, and the vehicle becomes `Rented`. After the end date, the reservation
becomes `Overdue` but remains open so the customer can provide the actual
returned mileage. Manual checkout then records the return, marks the
reservation `Completed`, and makes the vehicle available.

### Repository

- **GitHub**: [https://github.com/KJIE-430-XX/OOP-Car-Rental-System.git]
# Return inspection and damage fees

Returning a vehicle records the latest mileage and changes the reservation to
`Pending Inspection`. An administrator then opens the booking, selects damaged
components from the checklist, and submits the report. Damage fees are
calculated polymorphically by vehicle category:

- Standard cars use the base repair estimates.
- SUVs use a 1.15 multiplier for larger components and parts.
- Luxury cars use a 1.50 multiplier for premium parts and labour.

The customer can review each damaged component and its fee while the booking
is `Awaiting Payment`. The dummy payment action marks the damage report paid,
sets the reservation to `Completed`, and makes the vehicle available again.