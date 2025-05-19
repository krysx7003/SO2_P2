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
#define USER_FILE filesystem::absolute("../data/users.json").string()
#define CONV_DIR filesystem::absolute("../data/conversations").string()
#define STATE_FILE filesystem::absolute("../data/state.json").string()

pthread_t thread_pool[THREAD_POOL_SIZE];
queue<client> cilent_pool;
vector<client> client_table;
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
json& findUser( string name, json& users );
void sendJson( string name, client currClient );
void sendMessage( string message, vector<string> names );

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
        client newClient = { clientSocket, clientName };
        {
            lock_guard<mutex> lock(jsonMtx);
            ifstream in_file( USER_FILE );
            json usersJson; 
            newClient.name.erase(std::remove(newClient.name.begin(), newClient.name.end(), '\n'), newClient.name.end());
            if(in_file.good()){
                in_file>>usersJson;
                in_file.close();
                json& user = findUser( newClient.name, usersJson );
                if( user != nullptr ){
                    if( !user["conversation_id"].is_null() ){
                        for (const auto& conv_id : user["conversation_id"]) {
                            int id = conv_id.get<int>();
                            sendJson( to_string(id), newClient );
                        }
                    }
                }else{
                    json new_user = {
                        { "name", newClient.name },
                        { "conversation_id", {} }
                    };
                    usersJson.push_back( new_user );
                    ofstream of_file( USER_FILE );
                    of_file << usersJson.dump(4);
                    of_file.close();
                }
            }else{
                logEvent("Could not open config file!");
                newClient.name = "";
            }
        }
        if( newClient.name != "" ){
            lock_guard<mutex> lock(que);
            cilent_pool.push( newClient );
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

json& findUser( string name, json& users ){
    for( auto& user : users){
        if( user["name"] == name ){
            return user;
        }
    }
    static json nullJson = nullptr;
    return nullJson;
}

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
                //Ehhhhhhhh
                client_table.push_back(currClient);
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
        vector<string> client_names;
        if( data.rfind("\\send") == 0 ){
            lock_guard<mutex> lock(jsonMtx);
            string str;
            stringstream ss(data);
            //Skip \send command
            getline(ss,str,' ');
            getline(ss,str,' ');
            int id = stoi(str);
            string messageText;
            getline( ss, messageText );

            ifstream in_file( CONV_DIR + "/" + to_string( id ) + ".json" );
            json conversation, message;

            if( in_file.good() ){
                in_file >> conversation;
                in_file.close();
                if( conversation["messages_log"].is_null() ){
                    conversation["messages_log"] = json::array();
                }
                for ( auto& user : conversation["users"]) {
                    str = user.get<string>();
                    if( str != currClient.name ){
                        client_names.push_back( str );
                    }
                }
                message = {
                    { "sender", currClient.name },
                    { "timestamp", logTime() },
                    { "text", messageText }
                };
                
                conversation["messages_log"].push_back( message );
                
                ofstream of_file( CONV_DIR + "/" + to_string( id ) + ".json" );
                of_file << conversation.dump(4);
                of_file.close();
            }

            string message_str = message.dump(4);
            sendMessage( message_str, client_names );

        } else if( data.rfind("\\create") == 0 ){
            lock_guard<mutex> lock(jsonMtx);
            string response = to_string(-1);

            string str;
            vector<string> users;
            stringstream ss(data);
            //Skip \create command
            getline(ss,str,' ');
            while ( getline(ss,str,' ') ){
                str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
                users.push_back(str);
            }
            int id = -1;
            
            //Client list must start with currClient.name
            if( users[0] != currClient.name ){
                continue;
            }

            ifstream state_in( STATE_FILE );            
            if ( state_in.good() ){
                json data;
                state_in >> data;
                id = data["id"].get<int>();
                response = "\\new_conversation " + to_string(id);
            }
            
            json new_conversation = {
                { "id", id },
                { "users", users },
                { "messages_log", {} }
            };
            ofstream of_file( CONV_DIR + "/" + to_string( id ) + ".json" );
            of_file << new_conversation.dump(4);
            of_file.close();
            logEvent( "Created new conversation with id: " + to_string( id ) );

            ifstream in_file( USER_FILE );
            json usersJson; 
            if(in_file.good()){
                in_file>>usersJson;
                in_file.close();
                for( string userName : users ){
                    json& user = findUser( userName, usersJson );
                    if( user == nullptr){
                        continue;
                    }
                    if( user["conversation_id"].is_null() ){
                        user["conversation_id"] = json::array();
                    }
                    user["conversation_id"].push_back(id);
                }
                ofstream of_file( USER_FILE );
                of_file << usersJson.dump(4);
                of_file.close();
            }
            logEvent( "Updated users.json" );
            
            id++;
            json state_data;
            state_data["id"] = id;
            ofstream state_of( STATE_FILE );
            state_of << state_data.dump(4);
            state_of.close();
            logEvent( "Updated state.json with id: " + to_string( id ) );
            check( send( currClient.socket , response.c_str(), response.length(), 0 ), "Send failed" );
            users.erase( users.begin() );

            sendMessage( response, users );

        } else if( data.rfind("\\exit") == 0 ){
            lock_guard<mutex> lock(que);
            logEvent( "Disconnecting: " + currClient.name );
            close(currClient.socket);

            for (auto it = client_table.begin(); it != client_table.end(); ) {
                if (it->name == currClient.name) {
                    it = client_table.erase(it);
                } else {
                    ++it;
                }
            }
            break;
        }
        lock_guard<mutex> lock(que);
        data.erase(std::remove(data.begin(), data.end(), '\n'), data.end());
        logEvent(currClient.name + ": " + data);
    }
}

void sendJson( string name, client currClient ){
    ifstream in_file( CONV_DIR + "/" + name + ".json" );
    json conversation;
    if( in_file.good() ){
        in_file >> conversation;
        in_file.close();
        string conversation_str = conversation.dump(4);
        check( send( currClient.socket , conversation_str.c_str(), conversation_str.length(), 0 ), "Send failed" );
        logEvent( "Sent file: " + name + ".json to client: " + currClient.name );
    }else{
        logEvent( "Failed to open file" );
    }
}

void sendMessage( string message, vector<string> names ){
    for( client connected : client_table ){
        for( string name : names ){
            if( connected.name == name ){
                check( send( connected.socket, message.c_str(), message.length(), 0 ), "Send failed" );
            }            
        }
    }
}
