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
vector<json> conversations;

// Zwraca timestamp jako string w formacie YYYY-MM-DD HH:MM:SS
string logTime(){
    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    tm tm = *localtime(&time);
    ostringstream oss;
    oss << "[" 
        << std::put_time(&tm, "%d:%m:%Y")
        << "|"    
        << std::put_time(&tm, "%H:%M:%S") 
        << "]";
    return oss.str(); 
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
                cout <<  received["timestamp"].get<string>()
                     << received["sender"].get<string>()<<": "
                     << received["text"].get<string>() <<" "
                     << "\n"<< flush;
                for(json& conversation: conversations ){
                    if( conversation["id"] == received["id"] ){
                        conversation["messages_log"].push_back(received);
                    }
                }
                
            } else if (type == "server_message") {
                bool should_display = received["display"].get<bool>();
                if( should_display ){
                    cout << "\n[SERVER] " << buffer << "\n";
                }else{
                    conversations.push_back(received);
                    cout<< "id:"<< to_string( received["id"].get<int>() )
                        << " users: "<< received["users"]
                        <<"\n"<< flush;
                }
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
    stringstream ss(content);
    if (content.rfind("\\", 0) == 0) {  // polecenie (np. \exit, \users)
        string command,message; 
        getline(ss,command,' ');
        msg["type"] = "command";
        msg["command"] = command;
        if(command == "\\create"){
            json users_arr = json::array();
            string user;
            while(ss >> user){
                users_arr.push_back(user);
            }
            msg["users"] = users_arr;
        }else if(command == "\\list"){
            for(json conversation: conversations ){
                cout<< "id:"<< to_string( conversation["id"].get<int>() )
                    << " users: "<< conversation["users"]
                    <<"\n"<< flush;
            }
        }else if(command == "\\history"){
            string id;
            getline(ss,id,' ');
            for(json conversation: conversations ){
                if( conversation["id"] == stoi(id) ){
                    for (const auto& message : conversation["messages_log"]) {
                        cout<< message["timestamp"].get<string>()
                            << message["sender"].get<string>()<<": "
                            << message["text"].get<string>() <<" "
                            << "\n"<< flush;        
                    }
                }
            }
        }
        else{
            getline(ss,message);
            msg["message"] = message;
        }
    } else {
        string id,message; 
        json privateMsg;
        getline(ss,id,' ');
        
        getline(ss,message,' ');
        msg["type"] = "chat";
        msg["id"] = stoi( id );
        privateMsg = msg;
        msg["message"] = message;
        msg["command"] = "\\send";
        privateMsg["text"] = message;
        privateMsg["timestamp"] = logTime();
        privateMsg["sender"] = userName;
        for(json& conversation: conversations ){
            if( conversation["id"] == stoi(id) ){
                conversation["messages_log"].push_back(privateMsg);
            }
        }
             
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
