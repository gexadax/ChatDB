#include "chat.h"
#include "database.h"
#include <iostream>
#include "users.h"

SQLRETURN ret;
SQLHANDLE henv;
SQLHANDLE hdbc;
SQLHANDLE hstmt;

UserManager userManager;

void ChatManager::displayUserChat(const std::string& username) {
    DatabaseManager dbManager;
    if (dbManager.connectToDatabase()) {
        SQLRETURN ret;
        SQLHANDLE hstmt;
        SQLHANDLE hdbc = dbManager.getHDBC();
            std::string queryGetChat = "SELECT u.first_name, m.message_text, m.send_date "
                "FROM messages m "
                "INNER JOIN users u ON m.sender_id = u.user_id "
                "WHERE u.first_name = ? "
                "ORDER BY m.send_date";

            ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
            ret = SQLPrepareA(hstmt, (SQLCHAR*)queryGetChat.c_str(), SQL_NTS);
            ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLCHAR*)username.c_str(), 0, NULL);

            ret = SQLExecute(hstmt);

            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                std::cout << "Chat history for user '" << username << "':" << std::endl;

                SQLCHAR senderName[50], message[1000], timestamp[50];
                SQLLEN senderNameLen, messageLen, timestampLen;

                while (SQLFetch(hstmt) == SQL_SUCCESS) {
                    SQLGetData(hstmt, 1, SQL_C_CHAR, senderName, sizeof(senderName), &senderNameLen);
                    SQLGetData(hstmt, 2, SQL_C_CHAR, message, sizeof(message), &messageLen);
                    SQLGetData(hstmt, 3, SQL_C_CHAR, timestamp, sizeof(timestamp), &timestampLen);

                    std::cout << timestamp << " " << senderName << ": " << message << std::endl;
                }
            }
            else {
                std::cerr << "Failed to retrieve chat history." << std::endl;
                dbManager.disconnectFromDatabase();
                return;
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            dbManager.disconnectFromDatabase();
        }
        else {
            std::cerr << "Failed to connect to the database." << std::endl;
        }

    }

bool ChatManager::sendMessage(const std::string& senderFirstName, const std::string& receiverFirstName, const std::string& messageText) {
        DatabaseManager dbManager;
        if (dbManager.connectToDatabase()) {
            // Получение идентификатора отправителя
            std::string queryGetSenderID = "SELECT user_id FROM users WHERE first_name = ?";

            ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
            ret = SQLPrepareA(hstmt, (SQLCHAR*)queryGetSenderID.c_str(), SQL_NTS);
            ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLCHAR*)senderFirstName.c_str(), 0, NULL);

            ret = SQLExecute(hstmt);

            SQLLEN senderID;
            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                SQLGetData(hstmt, 1, SQL_C_LONG, &senderID, sizeof(senderID), NULL);
            }
            else {
                std::cerr << "Failed to retrieve sender ID." << std::endl;
                dbManager.disconnectFromDatabase();
                return false;
            }

            // Получение идентификатора получателя
            std::string queryGetReceiverID = "SELECT user_id FROM users WHERE first_name = ?";

            ret = SQLPrepareA(hstmt, (SQLCHAR*)queryGetReceiverID.c_str(), SQL_NTS);
            ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLCHAR*)receiverFirstName.c_str(), 0, NULL);

            ret = SQLExecute(hstmt);

            SQLLEN receiverID;
            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                SQLGetData(hstmt, 1, SQL_C_LONG, &receiverID, sizeof(receiverID), NULL);
            }
            else {
                std::cerr << "Failed to retrieve receiver ID." << std::endl;
                dbManager.disconnectFromDatabase();
                return false;
            }

            // Вставка сообщения в таблицу messages
            std::string queryInsertMessage = "INSERT INTO messages(sender_id, receiver_id, message_text, send_date) VALUES (?, ?, ?, CURRENT_TIMESTAMP)";

            ret = SQLPrepareA(hstmt, (SQLCHAR*)queryInsertMessage.c_str(), SQL_NTS);
            ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &senderID, 0, NULL);
            ret = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &receiverID, 0, NULL);
            ret = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 1000, 0, (SQLCHAR*)messageText.c_str(), 0, NULL);

            ret = SQLExecute(hstmt);

            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                std::cout << "Message sent." << std::endl;
            }
            else {
                std::cerr << "Failed to send message." << std::endl;
                dbManager.disconnectFromDatabase();
                return false;
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            dbManager.disconnectFromDatabase();
            return true;
        }
        else {
            std::cerr << "Failed to connect to the database." << std::endl;
            return false;
        }
    }

void userMenu(const std::string& username) {
    ChatManager chatManager; // Создаем экземпляр класса ChatManager

    int userChoice;

    do {
        std::cout << "(User menu) 1. Send message 2. Read message 3. Delete user 4. Exit" << std::endl;
        std::cin >> userChoice;

        switch (userChoice) {
        case 1: {
            // Опция "Send message"
            std::string receiverFirstName, messageText;
            std::cout << "Enter the recipient's first name: ";
            std::cin >> receiverFirstName;
            std::cout << "Enter the message: ";
            std::cin.ignore(); // Чтобы пропустить символ новой строки после предыдущего ввода
            std::getline(std::cin, messageText);

            if (chatManager.sendMessage(username, receiverFirstName, messageText)) {
                std::cout << "Message sent." << std::endl;
            }
            else {
                std::cout << "Failed to send message." << std::endl;
            }
            break;
        }
        case 2: {
            // Опция "Read message"
            chatManager.displayUserChat(username);
            break;
        }
        case 3: {
            // Опция "Delete user"
            std::string first_name;
            std::cout << "Enter the first name of the user to delete: ";
            std::cin >> first_name;

            if (userManager.deleteUserAndMessages(first_name)) {
                std::cout << "User and related messages deleted successfully." << std::endl;
            }
            else {
                std::cout << "Failed to delete user and related messages." << std::endl;
            }
            break;
        }

        case 4: {
            // Опция "Exit"
            std::cout << "Logging out." << std::endl;
            return; // Выход из пользовательского меню
        }
        default: {
            std::cout << "Invalid choice. Please select a valid option." << std::endl;
            break;
        }
        }
    } while (userChoice != 4);
}

void chatRoom(const std::string& first_name) {
    int choice;

    do {
        std::cout << "Chat Room Options:" << std::endl;
        std::cout << "1. Send Message" << std::endl;
        std::cout << "2. Read Messages" << std::endl;
        std::cout << "3. Exit Chat Room" << std::endl;

        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
        case 1: {
            std::string receiverFirstName, messageText;
            std::cout << "Enter the recipient's first name: ";
            std::cin >> receiverFirstName;
            std::cout << "Enter the message: ";
            std::cin.ignore();
            std::getline(std::cin, messageText);

            ChatManager chatManager;
            if (chatManager.sendMessage(first_name, receiverFirstName, messageText)) {
                std::cout << "Message sent." << std::endl;
            }
            else {
                std::cout << "Failed to send message." << std::endl;
            }
            break;
        }
        case 2: {
            ChatManager chatManager;
            chatManager.displayUserChat(first_name);
            break;
        }
        case 3: {
            std::cout << "Exiting Chat Room." << std::endl;
            return;
        }
        default: {
            std::cout << "Invalid choice. Please select a valid option." << std::endl;
            break;
        }
        }
    } while (choice != 3);
}


//void chatMenu() {
//    UserManager userManager;
//
//    int choice;
//
//    do {
//        std::cout << "(Chat menu) 1. Register 2. Login 3. Exit" << std::endl;
//        std::cin >> choice;
//
//        switch (choice) {
//        case 1: {
//            // Опция "Register"
//            std::string firstName, lastName, email, password;
//            std::cout << "Enter your first name: ";
//            std::cin >> firstName;
//            std::cout << "Enter your last name: ";
//            std::cin >> lastName;
//            std::cout << "Enter your email: ";
//            std::cin >> email;
//            std::cout << "Enter your password: ";
//            std::cin >> password;
//
//            if (userManager.registerUser(firstName, lastName, email, password)) {
//                std::cout << "User registered successfully." << std::endl;
//            }
//            else {
//                std::cout << "Failed to register user." << std::endl;
//            }
//            break;
//        }
//        case 2: {
//            // Опция "Login"
//            std::string first_name, password;
//            std::cout << "Enter your email: ";
//            std::cin >> first_name;
//            std::cout << "Enter your password: ";
//            std::cin >> password;
//
//            if (userManager.loginUser(first_name, password)) {
//                std::cout << "Login successful. Welcome!" << std::endl;
//                userMenu(first_name); // Переход к пользовательскому меню
//            }
//            else {
//                std::cout << "Login failed. Please check your email and password." << std::endl;
//            }
//            break;
//        }
//        case 3: {
//            // Опция "Exit"
//            std::cout << "Exiting the chat application." << std::endl;
//            return;
//        }
//        default: {
//            std::cout << "Invalid choice. Please select a valid option." << std::endl;
//            break;
//        }
//        }
//    } while (choice != 3);
//}

void chatMenu() {
    UserManager userManager;
    std::string first_name, password_hash;
    std::cout << "Enter your first name: "; std::cin >> first_name;
    std::cout << "Enter your password hash: "; std::cin >> password_hash;

    if (userManager.loginPass(first_name, password_hash)) {
        std::cout << "Login successful. Welcome, " << first_name << "!" << std::endl;
        chatRoom(first_name); // Вызов функции chatRoom для залогинившегося пользователя
    }
    else {
        std::cout << "Login failed. Please check your credentials." << std::endl;
    }
}



