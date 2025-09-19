#include <iostream>

#include "serverC.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <cctype>

void* listener(void* arg);
std::vector<AuthenticatedUser> users;

using namespace std;
int main() {
    // Loading username and password. 
    try {

        users = readAndParseUsers("members.txt");
        // std::cout << "Loaded Users:\n";
        // for (const auto& user : users) {
            
        //     std::cout << "Username: " << (user.username) << ", Password: " << (user.password) << std::endl;
        // }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    int udp_sockfd;
    struct sockaddr_in cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    ssize_t msglen;
    pthread_t thread_id;

    udp_sockfd = create_udp_socket(UDP_PORT_SERVER_C); // Call the utility function to create and bind a UDP socket
    std::cout << "The ServerC is up and running using UDP on port " << UDP_PORT_SERVER_C << "\n";
    
    if (pthread_create(&thread_id, NULL, listener, (void*)&udp_sockfd) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    // Wait for the thread to finish (optional)
    int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_M);
    if (sockfd < 0) {
        std::cerr << "Unable to connect to the server" << std::endl;
        return 1;
    }
    std::string message = "Hello, from serverC ";
    std::string message_type = "BOOT";
    std::string sender = "ServerC";
    
    send_message_over_udp(sockfd, message_type, sender, message);
    std::cout << "\nThe ServerC has informed the main server." << endl;
    terminate_socket(sockfd);
    // Send a message to the serverM
    pthread_join(thread_id, NULL);

    
    return 0;
}


void* listener(void* arg) {
    int sockfd = *((int*)arg);
    struct sockaddr_in cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    ssize_t msglen;

    while (1) {
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
            // Parse the encrypted message
            Message msg = Message::parseMessage(buffer);
            std::string response_to_serverM = "";
            if (msg.messageType == "CLIENT_AUTH") {
                printf("\nThe Server C received an authentication request from the main server.\n");
                AuthenticatedUser user = parseAUTHMessage(msg.content);
                int response = authenticateUser(users, user);
                if (response == 1) {
                    response_to_serverM = "SUCCESS";
                    std::cout << "\nSuccessful authentication.\n";
                } else if (response == -1) {
                    std::cout << "\nUsername does not exist.\n";
                    response_to_serverM = "NO_USERNAME";
                } else if (response == 100) {
                    std::cout << "\nPassword does not match.\n";
                    response_to_serverM = "INCORRECT_PASSWORD";
                } else {
                    std::cout << "";
                    response_to_serverM = "FAIL";
                }
            }
            int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_M);
            if (sockfd < 0) {
                std::cerr << "Unable to connect to the server" << std::endl;
            }
            std::string message_type = "AUTH_RESPONSE";
            std::string sender = "ServerC";
            send_message_over_udp(sockfd, message_type, sender, response_to_serverM);
            cout << "\nThe Server C finished sending the response to the main server." << endl ;
            terminate_socket(sockfd);

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }



    return NULL;
}

std::string readFileIntoString(const std::string& path) {
    std::ifstream input_file(path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Could not open the file - '" + path + "'");
    }
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    return buffer.str();
}


std::vector<AuthenticatedUser> readAndParseUsers(const std::string& path) {
    std::string fileContent = readFileIntoString(path);
    std::vector<AuthenticatedUser> users;
    std::istringstream iss(fileContent);
    std::string line;

    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string username, password;
        std::getline(lineStream, username, ',');  // Extract username up to the comma
        std::getline(lineStream, password);       // Extract password after the comma
        ltrim(username);
        ltrim(password);
        users.emplace_back(username, password);  // Create a new AuthenticatedUser and add to the list
    }
    return users;
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



int terminate_socket(int sockfd) {
    return close(sockfd);
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


void parseBootMessages(const std::string& sender) {
    std::cout << "The main server has received the notification from Server "
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

int authenticateUser(const std::vector<AuthenticatedUser>& users, const AuthenticatedUser& input_user) {
    // Check if the password is an empty string
    // cout << endl << input_user.username << endl; 
    if (input_user.password.empty()) {
        return 0;  // Return 0 if password is empty
    }
    // Iterate over the list of users to find a match
    for (const auto& user : users) {
        if (user.username == input_user.username) {
            if (user.password == input_user.password) {
                return 1;  // Return 1 if both username and password match
            }
            else {
                return 100;
            }
        }
    }

    return -1;  // Return -1 if no match is found
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
