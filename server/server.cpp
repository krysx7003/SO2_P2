#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem> 
#include "json.hpp"
#include "client.h"

using json = nlohmann::json;
using namespace std;

#define SOCKET_ERROR (-1)
#define SERVER_SOCKET 8080
#define SERVER_BACKLOG 20
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 10
#define JSON_FILE filesystem::absolute("../users/users.json").string()

pthread_t thread_pool[THREAD_POOL_SIZE];
queue<client> cilent_pool;
mutex out,que,logT,jsonMtx;
condition_variable condition_var;
bool new_client = false;

void check( int res, const char* message );
void waitForExit();
void* thread_func(void* args);
void handleConnection(client cilentSocket);
string logTime();
void logEvent(string message);
void cleanup();
string reciveMsg(int clientSocket);
json findUser( string name, json users );

int main(){
    int serverSocket, clientSocket;
    logEvent( "Server starting" );
    
    for( int i = 0; i < THREAD_POOL_SIZE; i++ ){
        pthread_create( &thread_pool[i], NULL, thread_func, NULL);
    }

    
    check( ( serverSocket = socket( AF_INET, SOCK_STREAM, 0 ) ),
             "Socket creation failed!\n" );
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_SOCKET);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    check( bind( serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress) ),
            "Bind failed" );

    check( listen( serverSocket, SERVER_BACKLOG ) , "Listen failed" );

    logEvent( "Waiting for connections..." );

    while(true){

        check( ( clientSocket = accept( serverSocket, nullptr, nullptr ) ), "Accept failed" );
        string clientName = reciveMsg(clientSocket);
        {
            lock_guard<mutex> lock(jsonMtx);
            ifstream in_file( JSON_FILE );
            // logEvent( JSON_FILE );
            json users; 
            clientName.erase(std::remove(clientName.begin(), clientName.end(), '\n'), clientName.end());
            if(in_file.is_open()){
                in_file>>users;
                in_file.close();
                json user = findUser( clientName, users );
                if( user != nullptr ){
                    if( !user["conversation_id"].is_null() ){
                        //Send any conversation needed
                    }
                }else{
                    json new_user = {
                        { "name", clientName },
                        { "conversation_id", {} }
                    };
                    users.push_back( new_user );
                    ofstream of_file( JSON_FILE );
                    of_file << users.dump(4);
                    of_file.close();
                }
            }else{
                logEvent("Could not open config file!");
                clientName = "";
            }
        }
        if( clientName != "" ){
            lock_guard<mutex> lock(que);
            cilent_pool.push( { clientSocket, clientName, 0 } );
            new_client = true;
            condition_var.notify_one();
        }
    }

    close(clientSocket);
    close(serverSocket);
    cleanup();
    waitForExit();

    return 0;
}

json findUser( string name, json users ){
    for( const auto& user : users){
        if( user["name"] == name ){
            return user;
        }
    }
    return nullptr;
}

string logTime(){
    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    tm tm = *localtime(&time);
    ostringstream oss;
    oss << "[" << std::put_time(&tm, "%H:%M:%S") << "]";
    return oss.str(); 
}

void logEvent(string message){
    lock_guard<mutex> lock(out);
    cout<< logTime() << message << "\n" << flush;
}

void signalHandler(int signum) {
    pthread_exit(nullptr); 
}

void cleanup() {
    for (auto thread : thread_pool ) {
        pthread_kill(thread,SIGTERM);
    }
    for (auto thread : thread_pool ) {
        pthread_join(thread, nullptr);
    }
    logEvent("Threads stopped");
}

void check( int res, const char* message ){
    if( res == SOCKET_ERROR ){
        string errorMessage = string(message) + ". Exiting...";
        logEvent( errorMessage );
        cleanup();
        waitForExit();
        exit( 1 );
    }
}

void waitForExit(){
    logEvent( "Press ENTER to exit..." );
    cin.get();
}

void* thread_func(void* args){
    while(true){
        client currClient;
        signal(SIGTERM, signalHandler); 
        {
            unique_lock<mutex> lock(que);
            condition_var.wait( lock, [] { return new_client;  } );
            if( !cilent_pool.empty() ){
                currClient = cilent_pool.front();
                cilent_pool.pop();
                logEvent( "Connecting to client: " + currClient.name );
            }
            new_client = false;
        }
        if(currClient.socket != -1){
            handleConnection(currClient);
        }
    }
}

string reciveMsg(int clientSocket){
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytesReceived = recv( clientSocket, buffer, sizeof(buffer) - 1, 0 );
    buffer[bytesReceived] = '\0';
    if( bytesReceived == -1 ){
        lock_guard<mutex> lock(que);
        close( clientSocket );
        logEvent( "Recive error" );
        return "";
    }else{
        return buffer;    
    }
}

void handleConnection( client currClient ){
    while(true){
        string data = reciveMsg(currClient.socket);
        //TODO - When starts with \\request_file send file back 
        //TODO - When starts with \\send {conversation id} append the conversation log and send to other clients
        //TODO - When starts with \\create {client_names} create new conversation and add other clients
        //FIXME - Potrzebne podczas testów z bash nc. Zamienić na 
        // if(data == "[exit]"){
        if( data.rfind("\\exit") == 0 ){
            lock_guard<mutex> lock(que);
            logEvent( "Disconnecting: " + currClient.name );
            close(currClient.socket);
            break;
        }
        lock_guard<mutex> lock(que);
        logEvent(currClient.name + ": " + data);
        //TODO - Send Message to other clients
        //Save message to conversation log
        //If target client/clients is online send the message
    }
}   
