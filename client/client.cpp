#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

std::atomic<bool> running(true);

// Funkcja do odbierania danych
void receiveData(int socket) {
    char buffer[1024];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            std::cout << "Received from server: " << buffer << std::endl;
        } else if (bytesReceived == 0) {
            std::cout << "Server closed the connection.\n";
            running = false;
            break;
        } else {
            std::cerr << "Error receiving data.\n";
            running = false;
            break;
        }
    }
}

// Funkcja do wysyłania danych
void sendData(int socket) {
    std::string message;
    while (running) {
        std::getline(std::cin, message);
        if (message == "exit") {
            running = false;
            break;
        }
        send(socket, message.c_str(), message.length(), 0);
    }
}

int main() {
    // tworzenie gniazda
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    // określanie adresu serwera
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // połączenie z serwerem
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Connection failed.\n";
        close(clientSocket);
        return 1;
    }

    std::cout << "Connected to server. Type 'exit' to quit.\n";

    // uruchomienie wątków do wysyłania i odbierania
    std::thread sender(sendData, clientSocket);
    std::thread receiver(receiveData, clientSocket);

    // czekamy na zakończenie obu wątków
    sender.join();
    receiver.join();

    // zamknięcie gniazda
    close(clientSocket);
    std::cout << "Connection closed.\n";

    return 0;
}
