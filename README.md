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

## Running a database query for testing

`QueryRunner` is a separate executable for testing SQL queries against the
SQLite database. It supports both read and write queries:

```text
cmake --build build --target QueryRunner --config Debug
build\Debug\QueryRunner.exe car_rental.db "SELECT * FROM Vehicles;"
build\Debug\QueryRunner.exe car_rental.db "UPDATE Reservations SET start_date = '2026-11-04', end_date = '2026-11-06' WHERE reservation_id = 16;"
```

The database path is the first argument and the SQL query is the remaining
argument text. Query results are printed as tab-separated values. For write
queries, the runner reports the number of affected rows.

## 🐳 Run Using Docker

You can run this system on your local machine using Docker without manually installing the project's development dependencies.

### Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) (Windows/Mac)
- [Docker Engine](https://docs.docker.com/engine/install/) (Linux)

### 1. Download the Docker Image

Open your terminal and execute:

```bash
docker pull dkhappy/car-rental:1.0
```

### 2. Run the Application

Execute the following command:

```bash
docker run -d \
  --name car-rental-system \
  -p 18080:18080 \
  dkhappy/car-rental:1.0
```

### 3. Access the System

Once the container is running, open your web browser and visit:

**http://localhost:18080**

You can now access the Car Rental Management System.

### 4. Stop the Application

To stop the running container:

```bash
docker stop car-rental-system
```

To start it again:

```bash
docker start car-rental-system
```

To remove the container:

```bash
docker rm -f car-rental-system
```

## 🛠️ Docker Information

| Item | Description |
|---|---|
| Docker Hub Username | dkhappy |
| Image Name | car-rental |
| Image Version | 1.0 |
| Container Port | 18080 |
| Access URL | http://localhost:18080 |

## 📝 Notes

- Make sure Docker Desktop or Docker Engine is running before executing the commands.
- The Docker image contains the application and its required packaged dependencies.
- No manual installation of the C++ compiler or application libraries is required.
- The application must be configured to listen on port 18080.
- Database persistence should be configured to prevent data loss when containers are removed.

## 📦 Docker Hub

Docker Image: https://hub.docker.com/r/dkhappy/car-rental