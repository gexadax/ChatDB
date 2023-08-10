#pragma once

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include <vector>

class UserManager {
private:
    SQLRETURN ret;
    SQLHANDLE hstmt;
    SQLHDBC hdbc;
    struct User {
        std::string first_name;
        std::string last_name;
        std::string email;
        std::string password_hash;
    };

    std::vector<User> users;
public:
    UserManager();
    ~UserManager();

    bool registerUser(const std::string& first_name, const std::string& last_name, const std::string& email, const std::string& password);
    bool deleteUserAndMessages(const std::string& first_name);
    bool loginPass(const std::string& first_name, const std::string& password_hash);
};


