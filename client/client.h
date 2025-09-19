#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

#define MESSAGE_SEP '*'
#define AUTH_SEP "<"
#define REQUEST_SEP '!'
#define TCP_PORT_SERVER_M 35263
#define BUFFER_SIZE 1024


class AuthenticatedUser {
public:
    std::string username;
    std::string password;
    AuthenticatedUser(std::string uname, std::string pwd) : username(uname), password(pwd) {}
};

class Message { //Required
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
int create_tcp_socket(int port);
std::string read_tcp_socket_in_client(int sockfd); // Required
int connect_to_tcp_server(const char* server_ip, int port); // Required
void send_message_over_tcp(int sockfd, const std::string& type, const std::string& sender, const std::string& message); // Required
std::string encryptString(const std::string& input);
int terminate_socket(int sockfd);
int getLocalPort(int sockfd);
void ltrim(std::string &s);
void rtrim(std::string &s);

#endif // CLIENT_H
