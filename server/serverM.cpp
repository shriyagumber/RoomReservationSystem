#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <iostream>
#include <netinet/tcp.h>
#include <sstream>
#include <string>

#include "serverM.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cctype>

// Function prototype for the listener function
using namespace std;

void* listener(void* arg);
void* tcp_listener(void* arg);
void send_tcp_message(int sockfd, const std::string& message);
vector<string> split(string str, string delimiter);
std::vector<std::string> parseRoomRequestMessageWith(const std::string& message);
int tcp_sockfd;
int connfd;
int main() {
    int udp_sockfd, tcp_sockfd;
    pthread_t udp_thread, tcp_thread;

    udp_sockfd = create_udp_socket(UDP_PORT_SERVER_M);
    tcp_sockfd = create_tcp_socket(TCP_PORT_SERVER_M);
    if (tcp_sockfd == -1) {
        printf("Failed to create TCP socket.\n");
        return 1;
    }
    if (pthread_create(&udp_thread, NULL, listener, (void*)&udp_sockfd) != 0) {
        perror("Failed to create UDP thread");
        return 1;
    }

    if (pthread_create(&tcp_thread, NULL, tcp_listener, (void*)&tcp_sockfd) != 0) {
        perror("Failed to create TCP thread");
        return 1;
    }
    printf("Main Server is up and running.\n");
    pthread_join(udp_thread, NULL);
    pthread_join(tcp_thread, NULL);

    terminate_socket(udp_sockfd);
    terminate_socket(tcp_sockfd);
    return 0;
}

void* listener(void* arg) {
    int sockfd = *((int*)arg);
    struct sockaddr_in cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    ssize_t msglen;

    while(1) {
        len = sizeof(cliaddr);
        // Receive data from client
        msglen = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (msglen < 0) {
            perror("recvfrom failed");
            continue;
        }
        buffer[msglen] = '\0'; // Null-terminate the received data
        // printf("Received: %s\n", buffer);
        try {
            Message msg = Message::parseMessage(buffer);
            if (msg.messageType == "BOOT") {
                parseBootMessages(msg.sender);
            } else if (msg.messageType == "AUTH_RESPONSE") {
                std::cout << "\nThe main server received the authentication result from Server C using UDP over port " << UDP_PORT_SERVER_M << ".\n";
                send_message_over_tcp(connfd, msg.content, "serverM", " ");
                std::cout << "\nThe main server sent the authentication result to the client.\n";
                shutdown(connfd, SHUT_WR);
            } else if(msg.messageType == "NO_ROOM" || msg.messageType == "ROOM_EXISIT_NO_TIME" || msg.messageType == "ROOM_AVAILABLE") {
                std::cout << "The main server received the response from " << msg.sender << " using UDP over port " << UDP_PORT_SERVER_M << " ." <<endl;
                send_message_over_tcp(connfd, msg.messageType, "serverM", msg.content);
                std::cout << "\nThe main server sent the the availability information the client.\n";
                shutdown(connfd, SHUT_WR);
            } else if(msg.messageType == "RESER_NO_ROOM" || msg.messageType == "RESER_ROOM_EXISIT_NO_TIME" || msg.messageType == "RESER_ROOM_AVAILABLE") {
                std::cout << "The main server received the response from " << msg.sender << " using UDP over port " << UDP_PORT_SERVER_M << " ." <<endl;
                send_message_over_tcp(connfd, msg.messageType, "serverM", msg.content);
                std::cout << "\nThe main server sent the the reservation result the client.\n";
                shutdown(connfd, SHUT_WR);
            }
            else {
                std::cout << "Received:\nType: " << msg.messageType
                          << "\nSender: " << msg.sender
                          << "\nContent: " << msg.content << std::endl;
            }
            
        } catch (std::exception& e) {
            std::cout << "Failed to parse message: " << e.what() << std::endl;
        }
    }
    return NULL;
}


void* tcp_listener(void* arg) {
    tcp_sockfd = *((int*)arg);
    struct sockaddr_in cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    int flag = 1;
    setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    while (1) {
        len = sizeof(cliaddr);
        connfd = accept(tcp_sockfd, (struct sockaddr*)&cliaddr, &len);
        if (connfd < 0) {
            perror("server accept failed");
            continue;
        }

        std::string received_msg;
        int msglen;
        while ((msglen = recv(connfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[msglen] = '\0';  // Null-terminate the string
            received_msg += buffer;
        }
        if (msglen < 0) {
            perror("recv failed");
        } else {
            // Process the complete message received
            Message msg = Message::parseMessage(received_msg);
            if (msg.messageType == "AUTH") {
                int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_C);
                if (sockfd < 0) {
                    std::cerr << "Unable to connect to the server" << std::endl;
                }
                AuthenticatedUser auth = parseAUTHMessage(msg.content);
                // std::cout << auth.username << " " << auth.password ; 
                if (auth.password.empty()) {
                    std::cout << "\nThe main server received the guest request for " << auth.username << " using TCP over port " << TCP_PORT_SERVER_M << std::endl;
                    std::cout << "\nThe main server accepts " << auth.username << " as a guest\n";
                    send_message_over_tcp(connfd, "GUEST", "serverM", " ");
                    std::cout << "\nThe main server sent the guest response to the client.\n";
                    shutdown(connfd, SHUT_WR);
                } else {
                    std::cout << "\nThe main server received the authentication for " << auth.username << " using TCP over port " << TCP_PORT_SERVER_M << std::endl;
                    std::string message = msg.content;
                    std::string message_type = "CLIENT_AUTH";
                    std::string sender = "ClientM";
                    send_message_over_udp(sockfd, message_type, sender, message);
                    std::cout << "\nThe main server forwarded the authentication for " << auth.username << " using UDP over port " << UDP_PORT_SERVER_M << std::endl;
                }                
            } else if (msg.messageType == "ROOM_REQUEST") {
                // RoomRequest room_request = RoomRequest::parseMessage(msg.content);
                RoomRequest clientRequest = RoomRequest::parseMessage(msg.content);
                if (clientRequest.requestType == "reservation") {
                    std::cout << "\nThe main server has received the reservation request on Room " << clientRequest.roomNumber << " at " << clientRequest.requestTime << " on " << clientRequest.day  << " from " << msg.sender  << " using TCP over port " << TCP_PORT_SERVER_M << "." << endl;
                
                    if(clientRequest.isReserAuth == "NO") { 
                        std::cout << "\nPermission denied." << endl << encryptString(msg.sender) << " cannot make a reservation.";
                        send_message_over_tcp(connfd, "NOT_AUTH_FOR_BOOKING", "client", " ");
                        std::cout << "\nThe main server sent the error message to the client." << endl;
                        shutdown(connfd, SHUT_WR);
                    } else {
                        if (clientRequest.roomNumber.substr(0,3) == "RTH") {
                            int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_RTH);
                            std::string message = msg.content;
                            std::string message_type = "RESER_REQ";
                            std::string sender = "serverM";
                            send_message_over_udp(sockfd, message_type, sender, message);
                            std::cout << "\nThe main server sent request to Server RTH." << std::endl;
                        
                    } else if (clientRequest.roomNumber.substr(0,3) == "EEB") {
                            int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_EEB);
                            std::string message = msg.content;
                            std::string message_type = "RESER_REQ";
                            std::string sender = "serverM";
                            send_message_over_udp(sockfd, message_type, sender, message);
                            std::cout << "\nThe main server sent request to Server EEB." << std::endl;

                    } else {
                        std::cout << "\n Invalid room number." << std::endl; 
                        send_message_over_tcp(connfd, "INVALID_ROOM_NUMBER", "client", " ");
                        cout << "\n Error message sent to client" << endl;

                        // send_message_over_tcp(connfd, "INVALID_ROOM_NUMBER", "serverM", msg.content);
                        shutdown(connfd, SHUT_WR);
                    }
                        
                    }
                } else if (clientRequest.requestType == "availability") {
                    std::cout << "\nThe main server has received the availability request on Room " << clientRequest.roomNumber << " at " << clientRequest.requestTime << " on " << clientRequest.day  << " from " << msg.sender  << " using TCP over port " << TCP_PORT_SERVER_M << "." << endl;
                    if (clientRequest.roomNumber.substr(0,3) == "RTH") {
                        int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_RTH);
                        std::string message = msg.content;
                        std::string message_type = "AVAIL_REQ";
                        std::string sender = "serverM";
                        send_message_over_udp(sockfd, message_type, sender, message);
                        std::cout << "\nThe main server sent request to Server RTH." << std::endl;
                        
                    } else if (clientRequest.roomNumber.substr(0,3) == "EEB") {
                        int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_EEB);
                        std::string message = msg.content;
                        std::string message_type = "AVAIL_REQ";
                        std::string sender = "serverM";
                        send_message_over_udp(sockfd, message_type, sender, message);
                        std::cout << "\nThe main server send request to Server EEB." << std::endl;

                    } else {
                        std::cout << "\n Invalid room number." << std::endl; 
                        send_message_over_tcp(connfd, "INVALID_ROOM_NUMBER", "client", " ");
                        cout << "\n Error message sent to client" << endl;
                        // send_message_over_tcp(connfd, "INVALID_ROOM_NUMBER", "serverM", msg.content);
                        shutdown(connfd, SHUT_WR);
                    }
                }
                
            } else {
                cout << " \n else" << msg.messageType;
            }
        }
        
    }
    terminate_socket(connfd);
    return NULL;
}


void send_tcp_message(int sockfd, const std::string& message) {
    if (send(sockfd, message.c_str(), message.length(), 0) < 0) {
        perror("send failed");
    }
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


int create_udp_socket(int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}



int create_tcp_socket(int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        return -1;
    }

    // Clear structure
    memset(&servaddr, 0, sizeof(servaddr));

    // Assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // Bind newly created socket to given IP
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        perror("socket bind failed");
        close(sockfd);
        return -1;
    }
    // Start listening on the socket
    if (listen(sockfd, 5) != 0) {
        perror("Listen failed");
        close(sockfd);
        return -1;
    }
    return sockfd;
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


int connect_to_udp_server(const char* server_ip, int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);

    // For UDP, connect is optional but helps simplify sending
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connect failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}


void send_message_over_udp(int sockfd, const std::string& message_type, const std::string& sender, const std::string& message) {
    std::string formattedMessage = message_type + MESSAGE_SEP + sender + MESSAGE_SEP + message;
    if (send(sockfd, formattedMessage.c_str(), formattedMessage.size(), 0) < 0) {
        perror("Send failed");
    }
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

RoomRequest RoomRequest::parseMessage(const std::string& message){
    std::vector<std::string> parts;
    std::stringstream ss(message);
    std::string part;

    try {
        while (getline(ss, part, REQUEST_SEP)) {
            parts.push_back(part);
        }
        if (parts.size() != 5) {
            throw std::runtime_error("Invalid message format: expected exactly four parts");
        }
        // for (const auto& p : parts) {
        //     std::cout << "Part: " << p << std::endl;
        // }
    } catch (const std::exception& e) {
        std::cout << "Error during parsing: " << e.what() << std::endl;
        throw;  // Re-throw the exception after logging
    }
    return RoomRequest(parts[0], parts[1], parts[2], parts[3], parts[4]);
}

void parseBootMessages(const std::string& sender) {
    std::cout << "The main server has received the notification from "
              << sender << " using UDP over port " << UDP_PORT_SERVER_M << std::endl;
}

AuthenticatedUser parseAUTHMessage(const std::string& message) {
    // Find the delimiter's position in the message
    size_t delimiterPos = message.find(AUTH_SEP);
    if (delimiterPos == std::string::npos) {
        throw std::invalid_argument("Malformed authentication message: Missing delimiter '~'");
    }

    // Split the message into encrypted username and password
    std::string encryptedUsername = message.substr(0, delimiterPos);
    std::string encryptedPassword = message.substr(delimiterPos + 1);

    // Decrypt the username and password
    // Assuming decryptString is defined and correctly implemented
    std::string username = encryptedUsername; // decryptString(encryptedUsername);
    std::string password = encryptedPassword; // decryptString(encryptedPassword);

    // Return the authenticated user object
    return AuthenticatedUser(username, password);
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
