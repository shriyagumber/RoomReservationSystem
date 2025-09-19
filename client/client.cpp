#include <iostream>
#include <cstring>  // For strlen
#include <sys/socket.h>  // For socket functions
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>   // For inet_addr
#include <netinet/tcp.h> // For TCP_NODELAY
#include <unistd.h>      // for read, close
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include "client.h"

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

using namespace std; 
int main() {
    std::string username, password;
    int sockfd = connect_to_tcp_server(SERVER_IP, TCP_PORT_SERVER_M);
    if (sockfd < 0) {
        std::cerr << "Unable to connect to the server" << std::endl;
        return 1;
    }

    int flag = 1;
    int is_guest = 0;
    
    std::cout << "\nClient is up and running.\n" ;

    // Authentication block
    while(1)
    {
        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
        std::cout << "\nPlease enter the username:";
        std::getline(std::cin, username);
        std::cout << "Please enter the password (“Enter” to skip for guests):";
        std::getline(std::cin, password);
        std::string message = encryptString(username) + AUTH_SEP + encryptString(password);
        std::string message_type = "AUTH";
        std::string sender = "Client";
        int localPort;
        localPort = getLocalPort(sockfd);
        if (localPort == -1) {
            std::cerr << "Failed to retrieve local port number." << std::endl;
            return 1;  // or handle error appropriately
        }
        
        send_message_over_tcp(sockfd, message_type, sender, message);
        if (password.empty()) {
            is_guest = 1;
            std::cout << username << " sent a guest request to the main server using TCP over port " << localPort << endl;
        } else {
            std::cout << username << " sent an authentication request to the main server.";
        }
        // Optionally shutdown the sending side of the socket to flush the transmission
        shutdown(sockfd, SHUT_WR);

        std::string response_from_serverM = read_tcp_socket_in_client(sockfd);
        Message msg = Message::parseMessage(response_from_serverM);
        // cout << "\n" << msg.messageType << "\n";
        if (msg.messageType == "SUCCESS") {
            cout << "\nWelcome member " << username << "!" << endl;
            break;
        } else if (msg.messageType == "NO_USERNAME"){
            cout << "\nFailed login: Username does not exist." << endl;
        } else if (msg.messageType == "INCORRECT_PASSWORD") {
            cout << "\nFailed login: Password does not match." << endl;
        } else if (msg.messageType == "GUEST") {
            cout << "\nWelcome guest " << username << " !" << endl;
            is_guest = 1;
            break;
        } 
        else {
            cout << "";
        }
        sockfd = connect_to_tcp_server(SERVER_IP, TCP_PORT_SERVER_M);
        if (sockfd < 0) {
            std::cerr << "Unable to connect to the server" << std::endl;
            return 1;
        }

    }
    std::cin.clear();  // Clear any error flags that might be set
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Ignore the rest of the current line


    // Reservation Query 
    std::string roomNumber, day, request_time, request_type;
    sockfd = connect_to_tcp_server(SERVER_IP, TCP_PORT_SERVER_M);
    if (sockfd < 0) {
        std::cerr << "Unable to connect to the server" << std::endl;
        return 1;
    }

 
    while(1) 
    {
        std::cout << "\nPlease enter the room number:";
        std::getline(std::cin, roomNumber);
        std::cout << "\nPlease enter the day:";
        std::getline(std::cin, day);
        if(day.empty()) {
            cout << "\nMissing argument.\n\n -----Start a new request-----" << endl;
            continue;
        }
        std::cout << "\nPlease enter the time: ";
        std::getline(std::cin, request_time);
        if(request_time.empty()) {
            cout << "\nMissing argument.\n\n -----Start a new request-----" << endl;
            continue;
        }
        std::cout << "\nWould you like to search for the availability or make a reservation?\n";
        std::getline(std::cin, request_type);
        if(request_type.empty()) {
            cout << "\nMissing argument.\n" << endl;
            continue;
        }
        std::string message_type = "ROOM_REQUEST";
        std::string auth_token = "YES";
        if (is_guest == 1) {
            auth_token = "NO";
        }
        std::string sender = encryptString(username);
        std::string message = roomNumber + REQUEST_SEP + day + REQUEST_SEP + request_time + REQUEST_SEP + request_type + REQUEST_SEP + auth_token;
        if (request_type == "reservation") {
            cout << endl << username << " sent a reservation request to the main server" << endl;
        } else if (request_type == "availability") {
            cout << endl << username << " sent an availability request to the main server" << endl;
        }
        send_message_over_tcp(sockfd, message_type, sender, message);
        int localPort;
        localPort = getLocalPort(sockfd);
        if (localPort == -1) {
            std::cerr << "Failed to retrieve local port number." << std::endl;
            return 1;  // or handle error appropriately
        }

        shutdown(sockfd, SHUT_WR);

        std::string response_from_serverM = read_tcp_socket_in_client(sockfd);
        Message msg = Message::parseMessage(response_from_serverM);
        cout << "\n " << response_from_serverM << endl;
        if (msg.messageType == "NOT_AUTH_FOR_BOOKING") {
            cout << "\nPermission denied: Guest cannot make reservation." << endl;
        } else if (msg.messageType == "NO_ROOM" || msg.messageType == "RESER_NO_ROOM" || msg.messageType == "ROOM_EXISIT_NO_TIME" || msg.messageType == "RESER_ROOM_EXISIT_NO_TIME" || msg.messageType == "ROOM_AVAILABLE" || msg.messageType == "RESER_ROOM_AVAILABLE" || msg.messageType == "INVALID_ROOM_NUMBER") {
            cout << "\nThe client received the response from the main server server using TCP over port " << localPort << " ."<< endl;
            if (msg.messageType == "ROOM_AVAILABLE" || msg.messageType == "RESER_ROOM_AVAILABLE") {
                if (msg.messageType == "RESER_ROOM_AVAILABLE") {
                    cout << "\nCongratulation! The reservation for Room " << roomNumber <<" has been made. " << endl ;
                } else {
                    cout << "\nThe requested room is available." << endl << endl ;
                }
            } else if (msg.messageType == "ROOM_EXISIT_NO_TIME" || msg.messageType == "RESER_ROOM_EXISIT_NO_TIME")  {
                if ( msg.messageType == "RESER_ROOM_EXISIT_NO_TIME") {
                    cout << "\nSorry! The requested room is not available";
                } else {
                    cout << "\nThe requested room is not available" << endl  << endl;
                }
            } else if (msg.messageType == "NO_ROOM" || msg.messageType == "RESER_NO_ROOM") {
                if (msg.messageType == "RESER_NO_ROOM") {
                    cout << "\nOops! Not able to find the room." << endl << endl;
                } else {
                    cout << "\nNot able to find the room." << endl  << endl;
                }
            } else if (msg.messageType == "INVALID_ROOM_NUMBER") {
                cout << "\nNot able to find the room. Invalid Room number." << endl << endl;
            }
        }


        sockfd = connect_to_tcp_server(SERVER_IP, TCP_PORT_SERVER_M);
        if (sockfd < 0) {
            std::cerr << "Unable to connect to the server" << std::endl;
            return 1;
        }
        std::cout << "\n-----Start a new request-----" << endl;

        // if extra credit is not implimented check whether input is null or not.
    }

    // std::cout << "\nCredentials sent to the server successfully." << std::endl;
    close(sockfd);  // Always ensure to close the socket
    return 0;
}

std::string read_tcp_socket_in_client(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    std::string results;
    while ((bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        results += buffer;
    }
    if (bytes_read < 0) {
        perror("Read error");
    }
    close(sockfd);
    return results;
}


std::string encryptString(const std::string& input) {
    std::string encrypted = input;  // Start with a copy of the input to maintain original special characters

    for (size_t i = 0; i < encrypted.length(); ++i) {
        char c = encrypted[i];
        int offset = i + 1;  // Offset is position-based starting from 1

        if (isalpha(c)) {  // Check if the character is alphabetic
            char base = islower(c) ? 'a' : 'A';
            // Ensure positive modulus result
            c = ((c - base + offset) % 26 + 26) % 26 + base;
        } else if (isdigit(c)) {  // Check if the character is a digit
            // Ensure positive modulus result
            c = ((c - '0' + offset) % 10 + 10) % 10 + '0';
        }
        // Special characters remain unchanged
        encrypted[i] = c;
    }

    return encrypted;
}

int terminate_socket(int sockfd) {
    return close(sockfd);
}

int connect_to_tcp_server(const char* server_ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Cannot create socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }
    return sockfd;
}

void send_message_over_tcp(int sockfd, const std::string& type, const std::string& sender, const std::string& message) {
    std::string full_message = type + MESSAGE_SEP + sender + MESSAGE_SEP + message;
    if (send(sockfd, full_message.c_str(), full_message.length(), 0) < 0) {
        perror("Send failed");
    }
}


int getLocalPort(int sockfd) {
    struct sockaddr_in localAddr;
    socklen_t addrLength = sizeof(localAddr);
    memset(&localAddr, 0, sizeof(localAddr));  // Initialize the address structure

    if (getsockname(sockfd, (struct sockaddr *)&localAddr, &addrLength) == -1) {
        perror("getsockname() failed");
        return -1;  // Return -1 on error
    }

    return ntohs(localAddr.sin_port);  // Convert network byte order to host byte order
}

Message Message::parseMessage(const std::string& message) {
    std::vector<std::string> parts;
    std::stringstream ss(message);
    std::string part;

    while (getline(ss, part, MESSAGE_SEP)) {
        parts.push_back(part);
    }

    // Check if we have the right amount of parts
    if (parts.size() != 3) {
        throw std::runtime_error("Invalid message format");
    }

    // Create a Message object with the parsed data
    return Message(parts[0], parts[1], parts[2]);
}


void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}