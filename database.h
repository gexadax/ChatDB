#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <string>

class DatabaseManager {
private:
    SQLRETURN ret;
    SQLHANDLE henv;
    SQLHANDLE hdbc;
    SQLHANDLE hstmt;

public:
    DatabaseManager();
    ~DatabaseManager();
    bool connectToDatabase();
    void disconnectFromDatabase();
    bool createTables();
    bool insertDataIntoTable();
    bool checkAndCreateDatabase();
    SQLHANDLE getHDBC() const {
        return hdbc;
    }
};
