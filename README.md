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
- **Crow** - Lightweight C++ web framework for handling HTTP requests, routing and REST APIs
- **SQLite** - Lightweight, file-based relational database used to store and manage application data

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

### 1. Clone the Repository

Open your terminal and execute:

```bash
git clone https://github.com/KJIE-430-XX/OOP-Car-Rental-System.git
```

Navigate to the project directory:

```bash
cd YOUR_PROJECT_DIRECTORY
```

### 2. Build and Run the Application

Execute the following command:

```bash
docker compose up --build -d
```

Docker Compose will automatically:
- Build the application using the Dockerfile.
- Create the Docker image.
- Create the application container.
- Map port 18080 to your local machine.
- Create a persistent volume for SQLite database storage.
- Start the application in the background.

### 3. Access the System

Once the container is running, open your web browser and visit:

**http://localhost:18080**

You can now access the Car Rental Management System.

### 4. Stop the Application

To stop the application and remove its container, execute:

```bash
docker compose down
```

To start the application again:

```bash
docker compose up -d
```

To rebuild the application after making source code changes:

```bash
docker compose up --build -d
```

### 5. Database Persistence

The application uses a Docker named volume called `car-rental-data` to store SQLite database files.

Database records will remain available even after stopping or removing the application container.

## 🛠️ Docker Information

| Item | Description |
|---|---|
| Docker Hub Username | dkhappy |
| Image Name | car-rental-system |
| Image Version | latest |
| Container Port | 18080 |
| Volume Name | car-rental-data |
| Access URL | http://localhost:18080 |

## 📝 Notes

- Make sure Docker Desktop or Docker Engine is running before executing the commands.
- Docker Compose builds the application using the provided Dockerfile.
- The Docker image contains the application and its required packaged dependencies.
- No manual installation of the C++ compiler or application libraries is required.
- The application must be configured to listen on port 18080.
- The Docker volume preserves SQLite database records independently of the container lifecycle.
- The first build may take longer because Docker needs to download the required base images and dependencies.

## 📦 Docker Hub

Docker Hub: https://hub.docker.com/r/dkhappy/car-rental