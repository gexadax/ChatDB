#pragma once
#include <string>

class ChatManager {
public:
    void displayUserChat(const std::string& username);
    bool sendMessage(const std::string& senderFirstName, const std::string& receiverFirstName, const std::string& messageText);
};
