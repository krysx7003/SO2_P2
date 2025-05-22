#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 2048

int clientSocket;
mutex mtx;
string userName;

// Zwraca timestamp jako string w formacie YYYY-MM-DD HH:MM:SS
string getCurrentTimestamp() {
    auto now = chrono::system_clock::now();
    time_t timeNow = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&timeNow), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Zapisuje wiadomość JSON do pliku o nazwie nadawcy
void saveMessageToFile(const json& msg) {
    string sender = msg.value("sender", "unknown");
    string fileName = sender + ".json";

    lock_guard<mutex> lock(mtx);
    ofstream outFile(fileName, ios::app);
    if (outFile.is_open()) {
        outFile << msg.dump() << "\n";
        outFile.close();
    } else {
        cerr << "[ERROR] Could not write to file: " << fileName << "\n";
    }
}

// Wątek odbierający wiadomości od serwera
void receiveMessages() {
    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            cout << "\n[INFO] Disconnected from server.\n";
            break;
        }

        buffer[bytesReceived] = '\0';

        try {
            json received = json::parse(buffer);

            string type = received.value("type", "unknown");

            if (type == "chat") {
                cout << "\n[" << received["timestamp"] << "] "
                     << received["sender"] << ": "
                     << received["message"] << "\n";
                saveMessageToFile(received);

            } else if (type == "server_message") {
                cout << "\n[SERVER] " << received["message"] << "\n";

            } else if (type == "query") {
                cout << "\n[QUERY] " << received["message"] << "\n";

            } else {
                cout << "\n[UNKNOWN TYPE] " << buffer << "\n";
            }

        } catch (json::exception& e) {
            cerr << "\n[ERROR] Failed to parse JSON: " << e.what() << "\nRaw: " << buffer << "\n";
        }
    }
}

// Wysyła wiadomość tekstową lub polecenie jako JSON
void sendMessage(const string& content) {
    json msg;

    if (content.rfind("\\", 0) == 0) {  // polecenie (np. \exit, \users)
        msg["type"] = "command";
        msg["command"] = content;
    } else {
        msg["type"] = "chat";
        msg["sender"] = userName;
        msg["timestamp"] = getCurrentTimestamp();
        msg["message"] = content;
    }

    string serialized = msg.dump();
    send(clientSocket, serialized.c_str(), serialized.length(), 0);
}

int main() {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Socket creation failed.\n";
        return 1;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed.\n";
        return 1;
    }

    cout << "Enter your name: ";
    getline(cin, userName);

    // Przesyłamy nazwę użytkownika jako pierwszy pakiet
    json initMsg;
    initMsg["type"] = "register";
    initMsg["sender"] = userName;
    send(clientSocket, initMsg.dump().c_str(), initMsg.dump().length(), 0);

    // Wątek odbioru wiadomości
    thread receiver(receiveMessages);

    string input;
    while (true) {
        cout << "\n> ";
        getline(cin, input);

        if (input == "\\exit") {
            sendMessage(input);
            break;
        }

        sendMessage(input);
    }

    receiver.join();
    close(clientSocket);
    return 0;
}
