#include "users.h"
#include "database.h" 

UserManager::UserManager() : hdbc(nullptr), hstmt(nullptr), ret(SQL_SUCCESS) {
    DatabaseManager dbManager;
    if (dbManager.connectToDatabase()) {
        hdbc = dbManager.getHDBC();
        ret = SQL_SUCCESS;
        hstmt = NULL;
        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    }
}


UserManager::~UserManager() {
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
}

bool UserManager::registerUser(const std::string& first_name, const std::string& last_name, const std::string& email, const std::string& password) {
    DatabaseManager dbManager;
    if (dbManager.connectToDatabase()) {
        std::string queryInsertUser = "INSERT INTO users (first_name, last_name, email) VALUES (?, ?, ?)";

        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        // Insert user data
        ret = SQLPrepareA(hstmt, (SQLCHAR*)queryInsertUser.c_str(), SQL_NTS);
        ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLCHAR*)first_name.c_str(), 0, NULL);
        ret = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLCHAR*)last_name.c_str(), 0, NULL);
        ret = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 100, 0, (SQLCHAR*)email.c_str(), 0, NULL);
        ret = SQLExecute(hstmt);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "Failed to register user." << std::endl;
            return false;
        }

        std::cout << "User registered successfully." << std::endl;

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        dbManager.disconnectFromDatabase();
        return true;
    }
    else {
        std::cerr << "Failed to connect to the database." << std::endl;
        return false;
    }
}

bool UserManager::deleteUserAndMessages(const std::string& first_name) {
    DatabaseManager dbManager;
    if (dbManager.connectToDatabase()) {
        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        // Остальной код без изменений

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        dbManager.disconnectFromDatabase();
        return true;
    }
    else {
        std::cerr << "Failed to connect to the database." << std::endl;
        return false;
    }
}

bool UserManager::loginPass(const std::string& first_name, const std::string& password_hash) {
    DatabaseManager dbManager;

    if (dbManager.connectToDatabase()) {
        SQLHANDLE hstmt;
        SQLHANDLE hdbc = dbManager.getHDBC();

        std::string queryLogin = "SELECT u.user_id FROM users u "
            "INNER JOIN passwords p ON u.user_id = p.user_id "
            "WHERE u.first_name = ? AND p.password_hash = ?";

        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        ret = SQLPrepareA(hstmt, (SQLCHAR*)queryLogin.c_str(), SQL_NTS);
        ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLCHAR*)first_name.c_str(), 0, NULL);
        ret = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 32, 0, (SQLCHAR*)password_hash.c_str(), 0, NULL);

        ret = SQLExecute(hstmt);

        SQLINTEGER user_id;
        ret = SQLBindCol(hstmt, 1, SQL_C_SLONG, &user_id, sizeof(user_id), NULL);

        ret = SQLFetch(hstmt);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            std::cout << "Retrieved user_id: " << user_id << std::endl;
            if (user_id > 0) {
                std::cout << "Login successful. User ID: " << user_id << std::endl;
                SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                dbManager.disconnectFromDatabase();
                return true;
            }
        }
        else {
            std::cerr << "Login failed." << std::endl;
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            dbManager.disconnectFromDatabase();
            return false;
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        dbManager.disconnectFromDatabase();
    }
    else {
        std::cerr << "Failed to connect to the database." << std::endl;
        return false;
    }

    return false;
}


