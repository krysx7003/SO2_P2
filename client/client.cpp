#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 2048

int clientSocket;
mutex mtx;

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

        lock_guard<mutex> lock(mtx);
        cout << "\n[SERVER] " << buffer << "\n";
    }
}

// Wysyłanie wiadomości do serwera
void sendMessage(const string& msg) {
    send(clientSocket, msg.c_str(), msg.length(), 0);
}

int main() {
    // Konfiguracja adresu serwera
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // Tworzenie gniazda
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Socket creation failed.\n";
        return 1;
    }

    // Próba połączenia z serwerem
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed.\n";
        return 1;
    }

    // Pobieranie imienia użytkownika i wysyłanie do serwera
    string name;
    cout << "Enter your name: ";
    getline(cin, name);
    sendMessage(name);

    // Uruchomienie wątku odbierającego wiadomości
    thread receiver(receiveMessages);

    // Główna pętla wysyłania wiadomości
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
