#include "database.h"

DatabaseManager::DatabaseManager() : henv(nullptr), hdbc(nullptr), hstmt(nullptr), ret(SQL_SUCCESS) {}

DatabaseManager::~DatabaseManager() {
    disconnectFromDatabase();
}

bool DatabaseManager::connectToDatabase() {
    hstmt = NULL;
    ret = SQL_SUCCESS;
    //std::cout << "Connecting to the database..." << std::endl;

    // Выделяем ресурсы для окружения ODBC
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        //std::cerr << "Failed to allocate environment handle." << std::endl;
        return false;
    }

    // Устанавливаем версию ODBC
    ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        //std::cerr << "Failed to set ODBC version." << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // Выделяем ресурсы для подключения к базе данных
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        //std::cerr << "Failed to allocate database connection handle." << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // Строка подключения к базе данных
    std::wstring connectionString = L"DSN=chatdb;UID=root;PWD=root";
    // Подключаемся к базе данных
    ret = SQLDriverConnect(hdbc, NULL, (SQLWCHAR*)connectionString.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        //std::cerr << "Failed to connect to the database." << std::endl;
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);

        // Если подключение не удалось, выполняем checkAndCreateDatabase()
        if (!checkAndCreateDatabase()) {
            //std::cerr << "Failed to check and create database." << std::endl;
            return false;
        }
    }

    //std::cout << "Connected to the database." << std::endl;

    // Возвращаем результат успешности подключения
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

void DatabaseManager::disconnectFromDatabase() {
    if (hstmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = NULL;
    }
    //std::cout << "Disconnecting from the database..." << std::endl;

    // Освобождаем обработчик выражения, если он был выделен
    if (hstmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    // Отключаемся от базы данных и освобождаем обработчик соединения
    if (hdbc) {
        SQLDisconnect(hdbc);
        //std::cout << "Disconnected from the database." << std::endl;
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }

    // Освобождаем обработчик окружения
    if (henv) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
}

bool DatabaseManager::createTables() {
    ret = SQL_SUCCESS;
    hstmt = NULL;

    if (connectToDatabase()) {
        // Создание таблиц
        std::string queryCreateUsers = "CREATE TABLE users ("
            "user_id INTEGER PRIMARY KEY AUTO_INCREMENT,"
            "first_name VARCHAR(50) NOT NULL,"
            "last_name VARCHAR(50) NOT NULL,"
            "email VARCHAR(100) UNIQUE NOT NULL"
            ");";

        std::string queryCreatePasswords = "CREATE TABLE passwords ("
            "user_id INTEGER PRIMARY KEY,"
            "password_hash VARCHAR(32) NOT NULL,"
            "FOREIGN KEY (user_id) REFERENCES users(user_id)"
            ");";

        std::string queryCreateMessages = "CREATE TABLE messages ("
            "message_id INTEGER PRIMARY KEY AUTO_INCREMENT,"
            "sender_id INTEGER NOT NULL,"
            "receiver_id INTEGER NOT NULL,"
            "message_text TEXT NOT NULL,"
            "send_date TIMESTAMP NOT NULL,"
            "delivery_status INTEGER NOT NULL DEFAULT 0,"
            "FOREIGN KEY (sender_id) REFERENCES users(user_id),"
            "FOREIGN KEY (receiver_id) REFERENCES users(user_id)"
            ");";

        std::string queryCreateUserTrigger = "CREATE TRIGGER register_user_trigger\n"
            "AFTER INSERT ON users\n"
            "FOR EACH ROW\n"
            "BEGIN\n"
            "    INSERT INTO passwords (user_id, password_hash) VALUES (NEW.user_id, 'pass');\n"
            "END;";

        std::string queryCreateDeleteUserTrigger = "CREATE TRIGGER delete_user_trigger\n"
            "BEFORE DELETE ON users\n"
            "FOR EACH ROW\n"
            "BEGIN\n"
            "    DELETE FROM messages WHERE sender_id = OLD.user_id OR receiver_id = OLD.user_id;\n"
            "    DELETE FROM passwords WHERE user_id = OLD.user_id;\n"
            "END;";

        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryCreateUsers.c_str(), SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to create 'users' table." << std::endl;
            disconnectFromDatabase();
            return false;
        }
        std::cout << "Table 'users' created." << std::endl;

        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryCreatePasswords.c_str(), SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to create 'passwords' table." << std::endl;
            disconnectFromDatabase();
            return false;
        }
        std::cout << "Table 'passwords' created." << std::endl;

        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryCreateMessages.c_str(), SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to create 'messages' table." << std::endl;
            disconnectFromDatabase();
            return false;
        }
        std::cout << "Table 'messages' created." << std::endl;

        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryCreateUserTrigger.c_str(), SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to create 'register_user_trigger' trigger." << std::endl;
            disconnectFromDatabase();
            return false;
        }
        std::cout << "Trigger 'register_user_trigger' created." << std::endl;

        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryCreateDeleteUserTrigger.c_str(), SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to create 'delete_user_trigger' trigger." << std::endl;
            disconnectFromDatabase();
            return false;
        }
        std::cout << "Trigger 'delete_user_trigger' created." << std::endl;

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        disconnectFromDatabase();
        return true;
    }
    else {
        std::cerr << "Failed to connect to the database." << std::endl;
        return false;
    }
    system("cls");

}

bool DatabaseManager::insertDataIntoTable() {
    if (connectToDatabase()) {
        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        // Добавление данных в таблицу users
        std::string queryUsers = "INSERT INTO users(first_name, last_name, email) VALUES "
            "('User1', 'User1 Last', 'user1@example.com'),"
            "('User2', 'User2 Last', 'user2@example.com'),"
            "('User3', 'User3 Last', 'user3@example.com')";

        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryUsers.c_str(), SQL_NTS);

        SQLLEN rowCount;
        SQLRowCount(hstmt, &rowCount);

        if (ret == SQL_SUCCESS && rowCount > 0) {
            std::cout << "Data inserted into 'users' table." << std::endl;
        }
        else {
            std::cerr << "Failed to insert data into 'users' table." << std::endl;
            disconnectFromDatabase();
            return false;
        }

        // Добавление данных в таблицу messages
        std::string queryMessages = "INSERT INTO messages(sender_id, receiver_id, message_text, send_date, delivery_status) VALUES "
            "(1, 2, 'Hello all', CURRENT_TIMESTAMP, 1),"
            "(2, 1, 'Hi', CURRENT_TIMESTAMP, 1),"
            "(3, 1, 'How are you?', CURRENT_TIMESTAMP, 0)";

        ret = SQLExecDirectA(hstmt, (SQLCHAR*)queryMessages.c_str(), SQL_NTS);

        SQLRowCount(hstmt, &rowCount);

        if (ret == SQL_SUCCESS && rowCount > 0) {
            std::cout << "Data inserted into 'messages' table." << std::endl;
        }
        else {
            std::cerr << "Failed to insert data into 'messages' table." << std::endl;
            disconnectFromDatabase();
            return false;
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        disconnectFromDatabase();
        return true;
    }
    else {
        std::cerr << "Failed to connect to the database." << std::endl;
        return false;
    }
}

bool DatabaseManager::checkAndCreateDatabase() {
    std::wcout << L"Initializing ODBC environment..." << std::endl;
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

    // Connect to the MySQL server
    std::wcout << L"Connecting to MySQL server..." << std::endl;
    SQLWCHAR connStr[] = L"DRIVER={MySQL ODBC 8.0 ANSI Driver};"
        L"SERVER=localhost;"
        L"USER=root;"
        L"PASSWORD=root;"
        L"OPTION=3;";

    ret = SQLDriverConnectW(hdbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::wcerr << L"Failed to connect to the MySQL server." << std::endl;
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // Check if the database 'chatdb' exists
    std::wstring checkDbQuery = L"SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'chatdb'";
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    ret = SQLExecDirectW(hstmt, (SQLWCHAR*)checkDbQuery.c_str(), SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::wcerr << L"Failed to execute the database existence check query." << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    SQLLEN rowCount;
    SQLRowCount(hstmt, &rowCount);

    if (rowCount == 0) {
        // Create the database
        std::wstring createDbQuery = L"CREATE DATABASE chatdb";
        std::wcout << L"Creating 'chatdb' database..." << std::endl;
        ret = SQLExecDirectW(hstmt, (SQLWCHAR*)createDbQuery.c_str(), SQL_NTS);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::wcerr << L"Failed to create 'chatdb' database." << std::endl;
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            SQLDisconnect(hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
            return false;
        }

        std::wcout << L"Database 'chatdb' created." << std::endl;
        // Create tables and insert data
        createTables();
        insertDataIntoTable();
    }
    else {
        std::wcout << L"Connection to database 'chatdb' established." << std::endl;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);

    return true;
}