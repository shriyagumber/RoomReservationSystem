
#include <iostream>

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
#include "serverEEB.h"



using namespace std;
void* listener(void* arg);
std::vector<BookingSlot> bookings;
int udp_sockfd;


int main() {
    try {
        bookings = readBookingsFromFile("EEB.txt");
        // std::cout << "Bookings loaded:\n";
        // displayBooking(bookings);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    pthread_t thread_id;

    struct sockaddr_in cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    ssize_t msglen;
    udp_sockfd = create_udp_socket(UDP_PORT_SERVER_EEB); // Call the utility function to create and bind a UDP socket
    cout << "The Server EEB is up and running using UDP on port " << UDP_PORT_SERVER_EEB << "\n";
    
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

    std::string message = "Hello, from serverEEB ";
    std::string message_type = "BOOT";
    std::string sender = "ClientEEB";
    
    send_message_over_udp(sockfd, message_type, sender, message);
    std::cout << "\nThe Server EEB has informed the main server." << endl;
    // Send a message to the serverM
    terminate_socket(sockfd);

    pthread_join(thread_id, NULL);
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
        Message msg = Message::parseMessage(buffer);
        RoomRequest received_req = RoomRequest::parseMessage(msg.content);
        if (msg.messageType == "AVAIL_REQ") {
            cout << "\nThe Server EEB received an availability request from the main server." << endl;
            std::string response;
            response = checkRoomAvailability(bookings, received_req);
            if (response == "NO_ROOM") {
                std::cout << "\nNot able to find the room "<< received_req.roomNumber << endl;
            } else if(response == "ROOM_EXISIT_NO_TIME") {
                std::cout << "\nRoom " << received_req.roomNumber << " is not available at " << received_req.requestTime << " on " << received_req.day + " ." << endl;  
            } else if (response == "ROOM_AVAILABLE") {
                std::cout << "\nRoom " << received_req.roomNumber << " is available at " << received_req.requestTime << " on " << received_req.day + " ." << endl;  
            }
            int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_M);
            if (sockfd < 0) {
                std::cerr << "Unable to connect to the server" << std::endl;
            }
            std::string message_type = response;
            std::string sender = "serverEEB";
            send_message_over_udp(sockfd, message_type, sender, msg.content);
            std::cout << "\nThe Server EEB finished sending the response to the main server." << endl;
            terminate_socket(sockfd);
        } else if (msg.messageType == "RESER_REQ") {
            cout << "\nThe Server EEB received an reservation request from the main server." << endl;
            std::string response;
            response = checkRoomReservation(bookings, received_req);
            // if (response == "RESER_NO_ROOM") {
            //     std::cout << "\nNot able to find the room "<< received_req.roomNumber << endl;
            // } else if(response == "RESER_ROOM_EXISIT_NO_TIME") {
            //     std::cout << "\nRoom " << received_req.roomNumber << " is not available at " << received_req.requestTime << " on" << received_req.day + " ." << endl;  
            // } else if (response == "RESER_ROOM_AVAILABLE") {
            //     std::cout << "\nRoom " << received_req.roomNumber << " is available at " << received_req.requestTime << " on" << received_req.day + " ." << endl;  
            // }
            int sockfd = connect_to_udp_server("127.0.0.1", UDP_PORT_SERVER_M);
            if (sockfd < 0) {
                std::cerr << "Unable to connect to the server" << std::endl;
            }
            std::string message_type = response;
            std::string sender = "serverEEB";
            send_message_over_udp(sockfd, message_type, sender, msg.content);
            std::cout << "\nThe Server EEB finished sending the response to the main server." << endl;
            terminate_socket(sockfd);
        }

    }

    return NULL;
}


using namespace std;
std::string readFileIntoString(const std::string& path) {
    std::ifstream input_file(path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Could not open the file - '" + path + "'");
    }
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    return buffer.str();
}


int displayBooking(std::vector<BookingSlot> bookings){
    cout << "\n --------------------\n";
    try {
        for (const auto& booking : bookings) {
            std::cout << "Room " << booking.room << "#" << booking.day << "#" << booking.time << "#" << booking.meridian << std::endl;
        }
        cout << "\n --------------------\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
std::vector<BookingSlot> readBookingsFromFile(const std::string& path) {
    std::string fileContent = readFileIntoString(path);
    std::vector<BookingSlot> bookings;
    std::istringstream iss(fileContent);
    std::string line;

    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string room, day, time, meridian;
        std::getline(lineStream, room, ',');
        std::getline(lineStream, day, ',');
        std::getline(lineStream, time, ',');
        std::getline(lineStream, meridian, ' ');
        ltrim(room);
        rtrim(room);
        day.erase(0, day.find_first_not_of(" \n\r\t"));
        ltrim(day);
        rtrim(day);
        time.erase(0, time.find_first_not_of(" \n\r\t"));
        ltrim(time);
        rtrim(time);
        meridian.erase(0, meridian.find_first_not_of(" \n\r\t"));
        ltrim(meridian);
        rtrim(meridian);
        bookings.emplace_back(room, day, time, meridian);
    }
    return bookings;
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



std::string checkRoomAvailability(std::vector<BookingSlot>& booking_slots, RoomRequest room_request){
    std::string response = "NO_ROOM";
    std::string slot_time;
    for (const auto& slot : booking_slots) {
        if (slot.room == room_request.roomNumber) {
            response = "ROOM_EXISIT_NO_TIME";
            if(slot.day == room_request.day) {
                // cout << "\n" << slot.time + " "+ slot.meridian << "#" << room_request.requestTime;
                slot_time = slot.time + " " + slot.meridian;
                rtrim(slot_time);
                cout << "\n" << slot_time << "$" << room_request.requestTime;
                if( slot_time == room_request.requestTime && slot.isAvailable == "TRUE") {
                    response = "ROOM_AVAILABLE";
                    return response;
                }
            }
        }
    }
    return response;
}

std::string checkRoomReservation(std::vector<BookingSlot>& booking_slots, RoomRequest room_request){
    std::string response = "RESER_NO_ROOM";
    std::string slot_time;
    for (auto& slot : booking_slots) {
        if (slot.room == room_request.roomNumber) {
            response = "RESER_ROOM_EXISIT_NO_TIME";
            if(slot.day == room_request.day) {
                slot_time = slot.time + " " + slot.meridian;
                rtrim(slot_time);
                // cout << "\n" << slot_time << "$" << room_request.requestTime;
                if( slot_time == room_request.requestTime) 
                    if (slot.isAvailable == "TRUE") {
                        response = "RESER_ROOM_AVAILABLE";
                        slot.isAvailable = "FALSE";
                        cout << "\nSuccessful reservation. The status of room " << room_request.roomNumber << " is updated." << endl;
                        return response;
                    } 
            }
        }
    }
    if (response == "RESER_NO_ROOM") {
        cout << "\nCannot make a reservation. Not able to find room layout." << endl ;
    }
    else if (response == "RESER_ROOM_EXISIT_NO_TIME") {
        response = "RESER_ROOM_EXISIT_NO_TIME";
        cout << "\nCannot make a reservation. Room  " << room_request.roomNumber << " is not available." << endl;
    }
    return response;
}