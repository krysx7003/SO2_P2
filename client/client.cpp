#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <fstream>
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

// Funkcja pomocnicza: zapisuje pojedynczy JSON do pliku
void saveMessageToFile(const json& msg) {
    string sender = msg.value("sender", "unknown");
    string fileName = sender + ".json";

    lock_guard<mutex> lock(mtx);
    ofstream outFile(fileName, ios::app);
    if (outFile.is_open()) {
        outFile << msg.dump() << "\n";
        outFile.close();
    } else {
        cerr << "Failed to open file: " << fileName << "\n";
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
                cout << "\n[" << received["sender"] << "] "
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

// Wysyłanie wiadomości do serwera
void sendMessage(const string& msg) {
    send(clientSocket, msg.c_str(), msg.length(), 0);
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
    sendMessage(userName);  // Rejestrujemy się na serwerze

    thread receiver(receiveMessages);

    string input;
    while (true) {
        cout << "\n> ";
        getline(cin, input);

        if (input == "\\exit") {
            sendMessage(input);
            break;
        }

        sendMessage(input);  // Na razie wysyłamy wiadomość jako czysty tekst
    }

    receiver.join();
    close(clientSocket);
    return 0;
}
