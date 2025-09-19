#ifndef SERVEREEB_H
#define SERVEREEB_H

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

// USC ID - 1665235263
#define MESSAGE_SEP '*'
#define AUTH_SEP "<"
#define REQUEST_SEP '!'
#define UDP_PORT_SERVER_M 34263
#define TCP_PORT_SERVER_M 35263
#define UDP_PORT_SERVER_C 31263
#define UDP_PORT_SERVER_EEB 33263
#define UDP_PORT_SERVER_RTH 32263

#define BUFFER_SIZE 1024


class BookingSlot {
public:
    std::string room;
    std::string day;
    std::string time;
    std::string meridian;
    std::string isAvailable;

    BookingSlot(std::string room, std::string day, std::string time, std::string meridian, std::string is="TRUE")
        : room(room), day(day), time(time), meridian(meridian), isAvailable(is) {}
};

class RoomRequest {
public:
    std::string roomNumber;
    std::string day;
    std::string requestTime;
    std::string requestType;
    std::string isReserAuth;
    

    RoomRequest(std::string rn, std::string d, std::string rt, std::string rtype, std::string is) {
        roomNumber = rn;
        day = d;
        requestTime = rt;
        requestType = rtype;
        isReserAuth = is;
    }

    static RoomRequest parseMessage(const std::string& message);
};


class AuthenticatedUser {
public:
    std::string username;
    std::string password;
    AuthenticatedUser(std::string uname, std::string pwd) : username(uname), password(pwd) {}
};

class Message {
public:
    std::string messageType;
    std::string sender;
    std::string content;

    // Constructor
    Message(std::string type = "", std::string sndr = "", std::string cnt = "")
        : messageType(type), sender(sndr), content(cnt) {}

    // Function to parse incoming message string
    static Message parseMessage(const std::string& message);
};

// Function to parse a file containing encrypted usernames and passwords

int create_udp_socket(int port);
int connect_to_udp_server(const char* server_ip, int port);

void send_message_over_udp(int sockfd, const std::string& message_type, const std::string& sender, const std::string& message);

int terminate_socket(int sockfd);
std::vector<BookingSlot> readBookingsFromFile(const std::string& path);
std::string readFileIntoString(const std::string& path);
int displayBooking(std::vector<BookingSlot> bookings);

void ltrim(std::string &s);
void rtrim(std::string &s);
std::string checkRoomAvailability(std::vector<BookingSlot>& booking_slots, RoomRequest room_request);
std::string checkRoomReservation(std::vector<BookingSlot>& booking_slots, RoomRequest room_request);


#endif // UTIL_H
