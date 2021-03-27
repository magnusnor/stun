#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>
#include <map>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "easylogging++.h"



/* ---------------------------------------------------------------------------------------------------- */



/* CONSTANTS */

INITIALIZE_EASYLOGGINGPP // LOGGING

// Use default logger
el::Logger* defaultLogger = el::Loggers::getLogger("default");

const char IPv4_ADDRESS_FAMILY  = 0x01;
const char IPv6_ADDRESS_FAMILY = 0x02; // Not supported currently

const short UDP_TCP_PORT = 3478; // UDP & TCP
const short TLS_PORT = 5349; // TLS
const int BUFFER_SIZE = 1024;
const char ERROR = (-1);
const char SERVER_BACKLOG = 1;
const int THREAD_POOL_SIZE = 20;
const int DATA_BUFFER_SIZE = 1024;
const int MAXLINE = 200;



/* ---------------------------------------------------------------------------------------------------- */



/* DEFINITIONS */

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;



/* ---------------------------------------------------------------------------------------------------- */



/* HELPER METHOD */

// Method for checking errors
int check(int exp, const char *message) {
    if (exp == ERROR) {
        defaultLogger->error(message);
        exit(1);
    }
    return exp;
}

// Method for checking binding request attributes
int check_binding(char exp, char value, const char *message) {
    if (exp != value) {
        defaultLogger->error(message);
        return ERROR;
    }
    return 1; // OK
}

/* ---------------------------------------------------------------------------------------------------- */



/* STUN CLASSES */

class STUNServer {

    public:
        /* Unbound UDP / TCP socket

        AF_INET: Address family for IPv4. Alternative for IPv6 is AF_INET6
        SOCK_STREAM: Specifies TCP. SOCK_DGRAM: Specifies UDP
        0: Use default protocol for address family

        */
        int stun_server_socket;

        // IP address
        SA_IN stun_server_addr;

    public:
        int runUDP();
        int runTCP();
};

class MessageHeader {

    public:

        // Message type (Binding Request / Response)
        unsigned short type = htons(0x0001);

        // Payload length of this message
        unsigned short length;

        // Magic cookie
        unsigned int magic_cookie = htonl(0x2112A442);

        // Unique transaction ID
        unsigned int identifier[3];
};

class AttributeHeader {

    public:

        // Attribute type
        unsigned short type;

        // Payload length of this attribute
        unsigned short length;

        // Value
        unsigned int value;
};

class XORMappedAddress {

    public:

        // Reserved bits at the start, set to 0
        unsigned char reserved = htons(0x00);

        // IPvX family
        unsigned char family = IPv4_ADDRESS_FAMILY;

        // Port
        unsigned short x_port;

        // Address
        char *x_addr;
};

class Response {

    public:

        unsigned short type;

        MessageHeader message_header;

        AttributeHeader attribute_header;

        XORMappedAddress xor_mapped_address;
};



/* ---------------------------------------------------------------------------------------------------- */



/* METHODS */

Response handle_binding_request_udp(char binding_request[20]) {

    // Create response
    Response response;

    // Handle binding request

    // Check response type
    if (check_binding(response.message_header.type, binding_request[0], "Binding request: Message type") == ERROR) {
        response.type = htons(0x0111); // 0b11 | Error Response
        response.attribute_header.type = htons(0x0009); // ERROR-CODE
        response.attribute_header.length = htons(0x0008);
    }

    else {
        response.type = htons(0x0101); // Success Response
        response.attribute_header.type = htons(0x0020); // XOR-MAPPED-ADDRESS
        response.attribute_header.length = htons(0x0008);
    }

    // Set message length
    response.message_header.length = htons(0x000c);

    // Check magic cookie
    if (check_binding(response.message_header.magic_cookie, binding_request[4], "Binding request: Magic cookie") == ERROR) {
        response.type = htons(0x0111); // 0b11 | Error Response
        response.attribute_header.type = htons(0x0009); // ERROR-CODE
        response.attribute_header.length = htons(0x0008);
    }
    else {
        response.type = htons(0x0101); // Success Response
        response.attribute_header.type = htons(0x0020); // XOR-MAPPED-ADDRESS
        response.attribute_header.length = htons(0x0008);
    }

    // Set transaction ID
    response.message_header.identifier[0] = * (int *) (&binding_request[8]);
    response.message_header.identifier[1] = * (int *) (&binding_request[12]);
    response.message_header.identifier[2] = * (int *) (&binding_request[16]);


    return response;
}

void * handle_binding_request_tcp(void *arg) {
    // TODO: Handle the binding request from a client over TCP
    // See RFC 5389 document

    // Binding request
    unsigned char binding_request[20];

    return NULL;
}

int STUNServer::runUDP() {

    // Setup UDP Socket Address for client
    SA_IN udp_client_addr;

    socklen_t addr_size = sizeof(SA_IN);

    int ip_header_length = 20;

    // Buffer
    char buffer[BUFFER_SIZE];
    
    // UDP STUN Server Socket
    check(stun_server_socket = socket(AF_INET, SOCK_DGRAM, 0), "Create UDP socket failed!\n");

    // UDP STUN Server
    memset(&stun_server_addr, 0, sizeof(stun_server_addr));

    stun_server_addr.sin_addr.s_addr = INADDR_ANY;
    stun_server_addr.sin_family = AF_INET;
    stun_server_addr.sin_port = htons(UDP_TCP_PORT);

    // Bind UDP socket
    check(bind(stun_server_socket, (SA *) &stun_server_addr, sizeof(stun_server_addr)), "Bind failed!\n");

    defaultLogger->info("STUN Server Running");

    defaultLogger->info("Connection: UDP");

    defaultLogger->info("UDP Socket Address: %v, Port: %v", inet_ntoa(stun_server_addr.sin_addr), UDP_TCP_PORT);

    while (true) {

        defaultLogger->info("Client Connected! Address: %v, Port: %v", inet_ntoa(udp_client_addr.sin_addr), ntohs(udp_client_addr.sin_port));

        // Clear buffer
        memset(buffer, 0, sizeof(buffer));

        check(recvfrom(stun_server_socket, (char *) buffer, sizeof(buffer), MSG_WAITALL, (SA *) (&udp_client_addr), &addr_size), "Receive binding response failed!\n");

        if (* (short *) (&buffer[0]) == htons(0x0001)) {
            defaultLogger->info("Request: Binding");

            Response response = handle_binding_request_udp(buffer);

            response.xor_mapped_address.x_addr = inet_ntoa(udp_client_addr.sin_addr);
            response.xor_mapped_address.x_port = htons(udp_client_addr.sin_port);

            // Tokenize the IP Address
            unsigned char ip_tokens[4] = {0};

            sscanf(response.xor_mapped_address.x_addr, "%hu.%hu.%hu.%hu", &ip_tokens[0], &ip_tokens[1], &ip_tokens[2], &ip_tokens[3]);
            
            // XOR IP Address Tokens and X-PORT
            ip_tokens[0] ^= 0x21;
            ip_tokens[1] ^= 0x12;
            ip_tokens[2] ^= 0xA4;
            ip_tokens[3] ^= 0x42;

            response.xor_mapped_address.x_port ^= 0x2112;

            response.xor_mapped_address.x_port = htons(response.xor_mapped_address.x_port);

            // Serialize response class
            unsigned char binding_response[32];
            
            * (unsigned short *) (&binding_response[0]) = response.type;
            * (unsigned short *) (&binding_response[2]) = response.message_header.length;
            * (int *) (&binding_response[4]) = response.message_header.magic_cookie;
            * (int *) (&binding_response[8]) = response.message_header.identifier[0];
            * (int *) (&binding_response[12]) = response.message_header.identifier[1];
            * (int *) (&binding_response[16]) = response.message_header.identifier[2];
            * (unsigned short *) (&binding_response[20]) = response.attribute_header.type;
            * (unsigned short *) (&binding_response[22]) = response.attribute_header.length;
            * (unsigned short *) (&binding_response[24]) = response.xor_mapped_address.reserved;
            * (unsigned short *) (&binding_response[25]) = response.xor_mapped_address.family;
            * (unsigned short *) (&binding_response[26]) = response.xor_mapped_address.x_port;
            * (unsigned char *) (&binding_response[28]) = ip_tokens[0];
            * (unsigned char *) (&binding_response[29]) = ip_tokens[1];
            * (unsigned char *) (&binding_response[30]) = ip_tokens[2];
            * (unsigned char *) (&binding_response[31]) = ip_tokens[3];

            // Send response to socket
            check(sendto(stun_server_socket, &binding_response, sizeof(binding_response), MSG_CONFIRM, (SA *) (&udp_client_addr), sizeof(udp_client_addr)), "Send RESPONSE data to client failed!\n");
        }
        else {
            defaultLogger->error("Request: None / Unknown");
        }
        
    }

    // Close socket
    close(stun_server_socket);
    
    return 0;
}

int STUNServer::runTCP() {

    // Thread Pool
    pthread_t thread_pool[THREAD_POOL_SIZE];

    // Setup TCP Socket Address for client
    SA_IN tcp_client_addr;

    socklen_t addr_size = sizeof(SA_IN);

    // TCP STUN Server Socket
    check((stun_server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Create TCP socket failed!\n");

    // TCP STUN Server
    memset(&stun_server_addr, 0, sizeof(stun_server_addr));

    stun_server_addr.sin_family = AF_INET;
    stun_server_addr.sin_addr.s_addr = INADDR_ANY;
    stun_server_addr.sin_port = htons(UDP_TCP_PORT);

    // Bind the server socket to address
    check(bind(stun_server_socket, (SA *) &stun_server_addr, sizeof(stun_server_addr)), "Bind failed!\n");

    defaultLogger->info("STUN Server Running");

    defaultLogger->info("Connection: TCP");

    // Listen on the socket
    check(listen(stun_server_socket, SERVER_BACKLOG), "Listen failed!\n");

    while (true) {
        defaultLogger->info("Waiting for connection...");

        // Accept
        check(accept(stun_server_socket, (SA *) &tcp_client_addr, (socklen_t *) &addr_size), "Accept failed!\n");

        defaultLogger->info("Client Connected! Address: %v, Port: %v", inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port));

        // Create thread
        int i = 0;
        check(pthread_create(&thread_pool[i++], NULL, handle_binding_request_tcp, NULL), "Create thread failed!\n");

        while (i < THREAD_POOL_SIZE) {
            pthread_join(thread_pool[i++], NULL);
        }

        // Reset counter
        i = 0;

    }

    close(stun_server_socket);

    return 0;
    
};




/* ---------------------------------------------------------------------------------------------------- */



/* MAIN */



int main(int argc, char* argv[]) {

    // STUN Server
    STUNServer stun_server;

    std::map<std::string, std::string> opts;
    opts["run"] = "udp";

    // Create variables to hold your parameters
    const struct option longopts[] =
    {
        {"run", required_argument, 0, 'r'},
        {0,0,0,0} // This tells getopt that this is the end
    };

    // Some parameters for getopt_long
    int c(0);

    // Get the options from the command line
    while (c != -1) {
        int option_index(-1) ;

        // Read the next command line option
        // Note here that the ':' after the 'r' denotes that
        // it requires an argument
        c = getopt_long(argc, argv, "r:", longopts, &option_index) ;

        switch (c) {
            case 'r':
                opts["run"] = optarg;
                
                if (!opts["run"].compare("udp")) {
                    stun_server.runUDP();
                }
                else if (!opts["run"].compare("tcp")) {
                    stun_server.runTCP();
                }

            case '?':
                // getopt_long printed an error message
                break ;
        }
    }

    close(stun_server.stun_server_socket);
    
    return 0;
}
