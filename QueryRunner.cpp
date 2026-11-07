#include <SQLiteCpp/SQLiteCpp.h>

#include <iostream>
#include <string>

namespace {

void printUsage(const char* executable) {
    std::cerr << "Usage: " << executable
              << " [database-path] \"SQL query\"\n"
              << "Example: " << executable
              << " car_rental.db \"SELECT * FROM Vehicles;\"\n";
}

std::string joinArguments(int argc, char* argv[], int firstArgument) {
    std::string query;
    for (int index = firstArgument; index < argc; ++index) {
        if (!query.empty()) {
            query += ' ';
        }
        query += argv[index];
    }
    return query;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string databasePath = argv[1];
    const std::string sql = joinArguments(argc, argv, 2);

    try {
        SQLite::Database database(databasePath, SQLite::OPEN_READWRITE);
        SQLite::Statement query(database, sql);

        const int columnCount = query.getColumnCount();
        if (columnCount == 0) {
            query.exec();
            std::cerr << database.getChanges() << " row(s) affected\n";
            return 0;
        }

        for (int column = 0; column < columnCount; ++column) {
            if (column > 0) {
                std::cout << '\t';
            }
            std::cout << query.getColumnName(column);
        }
        std::cout << '\n';

        int rowCount = 0;
        while (query.executeStep()) {
            for (int column = 0; column < columnCount; ++column) {
                if (column > 0) {
                    std::cout << '\t';
                }

                const SQLite::Column& value = query.getColumn(column);
                if (!value.isNull()) {
                    std::cout << value.getText();
                }
            }
            std::cout << '\n';
            ++rowCount;
        }

        std::cerr << rowCount << " row(s)\n";
    } catch (const SQLite::Exception& error) {
        std::cerr << "Query failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
