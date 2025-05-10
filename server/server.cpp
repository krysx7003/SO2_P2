#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "client.h"

using namespace std;

#define SOCKET_ERROR (-1)
#define SERVER_SOCKET 8080
#define SERVER_BACKLOG 20
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 10

pthread_t thread_pool[THREAD_POOL_SIZE];
queue<client> cilent_pool;
mutex out,que;
condition_variable condition_var;
bool new_client = false;

string arrayToString( char* arr, int size );
void check( int res, const char* message );
void waitForExit();
void* thread_func(void* args);
void handleConnection(client cilentSocket);

int main(){
    int serverSocket, clientSocket;
    cout<< "Server starting\n";
    
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

    out.lock();
    cout << "Waiting for connections...\n";
    out.unlock();

    while(true){
        check( ( clientSocket = accept( serverSocket, nullptr, nullptr ) ), "Accept failed" );
        //TODO - Client sends initiall message containing username
        //Check list of existing users (json???)
        //If name exists in list give saved id 
        //Else generate and give unique id
        {
            lock_guard<mutex> lock(que);
            cilent_pool.push( { clientSocket, "Test", 0 } );
            new_client = true;
            condition_var.notify_one();
        }
    }

    close(clientSocket);
    close(serverSocket);

    waitForExit();

    return 0;
}

string arrayToString( char* arr, int size ){
    string res = "";
    for( int i = 0; i<size; i++ ){
        res = res + arr[i];
    }
    return res;
}

void check( int res, const char* message ){
    if( res == SOCKET_ERROR ){
        { 
            lock_guard<mutex> lock(que);
            cout<< message << ". Exiting...\n";
        }
        waitForExit();
        exit(-1);
    }
}

void waitForExit(){
    cout<< "Press ENTER to exit...\n";
    cin.get();
}

void* thread_func(void* args){
    while(true){
        client currClient;
        {
            unique_lock<mutex> lock(que);
            condition_var.wait( lock, [] { return new_client;  } );
            if( !cilent_pool.empty() ){
                currClient = cilent_pool.front();
                cilent_pool.pop();
                cout<< "Connecting to client: " + currClient.name + "\n"<< flush;
            }
            new_client = false;
        }
        if(currClient.socket != -1){
            handleConnection(currClient);
        }
    }
}

void handleConnection( client currClient ){
    while(true){
        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytesReceived = recv( currClient.socket, buffer, sizeof(buffer) - 1, 0 );
        buffer[bytesReceived] = '\0';
        if( bytesReceived == -1 ){
            lock_guard<mutex> lock(que);
            cout<< "Recive error\n";
        }else{
            // FIXME - Potrzebne podczas testów z bash nc. Zamienić na 
            // if(data == "\\exit"){
            string data(buffer);
            if( data.rfind("\\exit") == 0 || bytesReceived == 0 ){
                lock_guard<mutex> lock(que);
                cout<< "Client " + currClient.name + " requested exit\n";
                close(currClient.socket);
                break;
            }
            //TODO - Add timestamp
            lock_guard<mutex> lock(que);
            cout << "Message from " + currClient.name + " : " << buffer << flush;
            //TODO - Send Message to other clients
            //Save message to conversation log
            //If target client/clients is online send the message
        }
    }
}   
