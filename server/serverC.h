#ifndef SERVERC_H
#define SERVERC_H

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
#define UDP_PORT_SERVER_C 31263
#define BUFFER_SIZE 1024


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
std::vector<AuthenticatedUser> readAndParseUsers(const std::string& path);

int create_udp_socket(int port);
int connect_to_udp_server(const char* server_ip, int port);

void send_message_over_udp(int sockfd, const std::string& message_type, const std::string& sender, const std::string& message);
std::string encryptString(const std::string& input);

int terminate_socket(int sockfd);
std::string readFileIntoString(const std::string& path);
void parseBootMessages(const std::string& sender);
AuthenticatedUser parseAUTHMessage(const std::string& message);

int authenticateUser(const std::vector<AuthenticatedUser>& users, const AuthenticatedUser& input_user);
void ltrim(std::string &s);
void rtrim(std::string &s);


#endif // UTIL_H
