#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>

using namespace std;

string arrayToString(char* arr, int size){
    string res = "";
    for( int i = 0; i<size; i++ ){
        res = res + arr[i];
    }
    return res;
}

void waitForExit(){
    cout<<"Press ENTER to exit...\n";
    cin.get();
}

int main(){

    cout<<"Server starting\n";
    
    int serverSocket = socket(AF_INET,SOCK_STREAM,0);
    if( serverSocket == -1 ){
        cout<< "Socket creation failed! Exiting...\n";
        close(serverSocket);
        waitForExit();
        return 1;
    }
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if( bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1 ){
        cout<< "Bind failed\n";
        close(serverSocket);
        waitForExit();
        return 1;
    }

    if( listen(serverSocket, 5) == -1 ){
        cout<< "Listen failed\n";
        close(serverSocket);
        waitForExit();
        return 1;
    }

    cout << "Waiting for connections...\n";
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if( clientSocket == -1){
        cout<< "Accept failed\n";
        close(serverSocket);
        waitForExit();
        return 1;
    }
    while(true){
        char buffer[1024] = {0};
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        buffer[bytesReceived] = '\0';
        if( bytesReceived == -1 ){
            cout<< "Recive error\n";
        }else if(bytesReceived == 0){
            cout<< "Client terminated connection\n";
            break;
        }else{
            // FIXME - Potrzebne podczas testów z bash nc. Zamienić na 
            // if(data == "\\exit"){
            string data(buffer);
            if( data.rfind("\\exit") == 0 ){
                cout<< "Client requested exit\n";
                break;
            }
            cout << "Message from client: " << buffer;
        }
    }
    
    close(clientSocket);
    close(serverSocket);

    waitForExit();

    return 0;
}