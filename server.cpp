//Example code: A simple server side code, t363t3
//Handle multiple socket connections with select and fd_set on Linux
#include <arpa/inet.h>  //close
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <sys/socket.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h>  //close

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>

#include "http.h"
#include "sha1.h"
#include "websocket.h"

#define BUFFER_SIZE 65535
#define TRUE 1
#define FALSE 0
#define PORT 4004
#define baseUrl "https://screenshare.pro/"

#define RED(string) "\x1b[31m" string "\x1b[0m"
#define GREEN(string) "\x1b[32m" string "\x1b[0m"
#define YELLOW(string) "\x1b[33m" string "\x1b[0m"
#define BLUE(string) "\x1b[34m" string "\x1b[0m"
#define MAGENTA(string) "\x1b[35m" string "\x1b[0m"
#define CYAN(string) "\x1b[36m" string "\x1b[0m"
#define COLOR_RESET "\x1b[0m"

#define BACK_YELLOW(string) "\x1b[43m\x1b[34m" string "\x1b[0m\x1b[0m"

typedef struct
{
    int socket_descriptor;
    int startedWS;
    std::string url;
    int last_ping;
} Client;

int main(int argc, char *argv[]) {
    int opt = TRUE;
    int master_socket, addrlen, new_socket,
        max_clients = 30, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;

    bool doPrintSocketFrame = false;
    bool doPrintSocketFrameStats = false;

    // yes
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-pf") == 0) {
                doPrintSocketFrame = true;
            } else if (strcmp(argv[i], "-pfs") == 0) {
                doPrintSocketFrameStats = true;
            }
        }
    }

    unsigned long long totalFramesRecieved = 1;
    unsigned long long totalFramesSent = 1;
    unsigned long long lastFrameLength = 0;
    unsigned long long totalFrameLength = 0;

    std::unordered_map<std::string, int> urlToSocketDesc;
    std::unordered_map<int, std::vector<Client>> socketDescToClients;
    std::unordered_map<int, int> clientSocketDescToStreamerSocketDesc;
    std::unordered_map<int, Client> clients;

    char buffer[BUFFER_SIZE + 1];       //data buffer of 1K
    char send_buffer[BUFFER_SIZE + 1];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //bind the socket to localhost port 4004
    if (0 > bind(master_socket, (struct sockaddr *)&address, sizeof(address))) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf(COLOR_RESET "Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for (auto &clientFromClients : clients) {
            Client client = clientFromClients.second;
            //socket descriptor
            sd = client.socket_descriptor;

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;

            if (client.last_ping != -1) {
                if (client.last_ping != -1) {
                }
            }
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if (0 > (new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen))) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf(GREEN("New connection , socket fd is %d , ip is : %s , port : %d  \n"), new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //puts("Welcome message sent successfully");

            clients[new_socket] = {new_socket, 0, "", -1};
            // clients.push_back({new_socket, 0});
            //printf("Adding to list of sockets as %d\n", i);
        }

        //else its some IO operation on some other socket
        // for (auto socketClientPair = clients.begin(); socketClientPair != clients.end(); ++socketClientPair) { //&socketClientPair : clients)
        auto it = clients.begin();
        while (it != clients.end()) {
            if (doPrintSocketFrameStats) {
                printf(COLOR_RESET "Total frames recieved: %llu\n", totalFramesRecieved);
                printf(COLOR_RESET "Total frames sent: %llu\n", totalFramesSent);
                printf(COLOR_RESET "Last frame length: %llu\n", lastFrameLength);
                printf(COLOR_RESET "Average frame length: %llu\n\n", totalFrameLength / totalFramesRecieved);
            }

            Client client = it->second;
            auto previous_it = it;
            ++it;
            //socket descriptor
            sd = client.socket_descriptor;
            if (FD_ISSET(sd, &readfds)) {
                //Check if it was for closing , and also read the
                //incoming message
                memset(buffer, 0, sizeof(buffer));

                int read_size = BUFFER_SIZE;

                if (client.startedWS == 1) {
                    read_size = 10;
                }
                if ((valread = read(sd, buffer, read_size)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf(MAGENTA("Host disconnected , ip %s , port %d \n"), inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    //socketDescToClients[sd].erase(std::remove(socketDescToClients[sd].begin(), socketDescToClients[sd].end(), clients[sd]), socketDescToClients[sd].end());
                    it = clients.erase(previous_it);
                    continue;
                } else {
                    //set the string terminating NULL byte on the end
                    //of the data read
                    buffer[valread] = '\0';

                    if (client.startedWS == 1) {
                        unsigned int fin = (unsigned int)(buffer[0] & 0b10000000) == 0b10000000;
                        unsigned int opcode = (unsigned int)(buffer[0] & 0b00001111);
                        if (opcode > 2) {
                            //printf(BACK_YELLOW(RED("Something is very wrong: 1.") "\n=============================================================\n"));
                            //continue;
                        } else if (fin == 0 || opcode == 0) {
                            //printf(BACK_YELLOW(RED("Something is very wrong: 2.") "\n=============================================================\n"));
                            continue;
                        }
                        unsigned long payload_length = (unsigned int)(buffer[1] & 0b01111111);
                        unsigned int bit_offset = 2;
                        if (payload_length == 126) {
                            bit_offset += 2;
                            payload_length = (unsigned int)(buffer[2] & 0b11111111);
                            payload_length = payload_length << 8;
                            payload_length += (unsigned int)(buffer[3] & 0b11111111);
                        } else if (payload_length == 127) {
                            bit_offset += 8;
                            payload_length = (unsigned int)(buffer[2] & 0b11111111);
                            for (int shift_i = 0; shift_i < 7; shift_i++) {
                                payload_length = payload_length << 8;
                                payload_length += (unsigned int)(buffer[3 + shift_i] & 0b11111111);
                            }
                        }
                        if (doPrintSocketFrame) {
                            printf(BACK_YELLOW("================= Recieved websocket Frame ==================") ".");
                            printf("\n" BACK_YELLOW(" ") " Fin: %x ", fin);
                            printf(("Opcode: %x "), opcode);
                            printf(("Is masked: %d "), (unsigned int)((buffer[1] & 0b10000000) == 0b10000000));
                            printf(("Payload length: %lu"), payload_length);
                            printf(("\n" BACK_YELLOW(" ") " Mask: %x %x %x %x "),
                                   (unsigned int)(buffer[bit_offset + 0] & 0b11111111),
                                   (unsigned int)(buffer[bit_offset + 1] & 0b11111111),
                                   (unsigned int)(buffer[bit_offset + 2] & 0b11111111),
                                   (unsigned int)(buffer[bit_offset + 3] & 0b11111111));
                        }

                        bit_offset += 4;

                        totalFramesRecieved++;

                        int totalread = valread;
                        if (payload_length + bit_offset + 1 > BUFFER_SIZE) {
                            while (totalread < payload_length + bit_offset && (valread = read(sd, buffer, payload_length + bit_offset - totalread)) != 0) {
                                totalread += valread;
                            }
                            lastFrameLength = totalread;
                            totalFrameLength += totalread;
                            printf(BACK_YELLOW(RED("Dropped frame.")) " (Frame size is greater than %d bytes)\n", BUFFER_SIZE);
                            continue;
                        };
                        while (totalread < payload_length + bit_offset) {
                            valread = read(sd, buffer + totalread, payload_length + bit_offset - totalread);
                            totalread += valread;
                        }
                        lastFrameLength = totalread;
                        totalFrameLength += totalread;
                        buffer[totalread] = '\0';
                        if (doPrintSocketFrame) {
                            printf(("Total read length: %d\n"), totalread);
                            printf(BACK_YELLOW("=============================================================\n"));
                        }

                        //printf("Unmasked payload: \n>");
                        for (int payload_i = 0; payload_i < payload_length; payload_i++) {
                            buffer[bit_offset + payload_i] = buffer[bit_offset + payload_i] ^ buffer[bit_offset - 4 + payload_i % 4];
                            //printf("%c", buffer[bit_offset + payload_i]);
                        }

                        if (payload_length > 7 && buffer[bit_offset + 2] == 'h' && buffer[bit_offset + 3] == 'r' && buffer[bit_offset + 4] == 'e' && buffer[bit_offset + 5] == 'f') {
                            std::string url = std::string(buffer + bit_offset + 9, buffer + bit_offset - 2 + payload_length);
                            printf("%s\n", url.c_str());
                            if (url.compare(baseUrl) == 0) {
                                std::string stream_url = url + std::to_string(sd);
                                urlToSocketDesc[stream_url] = sd;
                                socketDescToClients[sd] = std::vector<Client>();

                                int send_len = generate_ws_frame(send_buffer, "{\"url\":\"" + stream_url + "\",\"streamer\":" + std::to_string(sd) + "}");

                                if (send(sd, send_buffer, send_len, 0) != send_len) {
                                    perror("send");
                                }

                                continue;
                            } else {
                                previous_it->second.url = url;
                                clientSocketDescToStreamerSocketDesc[sd] = urlToSocketDesc[url];
                                socketDescToClients[urlToSocketDesc[url]].push_back(previous_it->second);

                                int send_len = generate_ws_frame(send_buffer, "{\"watcher\":" + std::to_string(sd) + ",\"streamer\":" + std::to_string(urlToSocketDesc[url]) + "}");

                                if (send(sd, send_buffer, send_len, 0) != send_len) {
                                    perror("send");
                                }
                            }
                            continue;
                        }

                        if (opcode != 1 && opcode != 2) {
                            continue;
                        }

                        //printf("Bit offset: %d\n", bit_offset);

                        for (int payload_i = 0; payload_i < payload_length; payload_i++) {
                            buffer[bit_offset - 4 + payload_i] = buffer[bit_offset + payload_i];
                        }

                        buffer[0] = buffer[0] & 0b10001111;
                        buffer[1] = buffer[1] & 0b01111111;

                        opcode = (unsigned int)(buffer[0] & 0b00001111);

                        buffer[bit_offset - 4 + payload_length] = '\0';
                        buffer[bit_offset - 3 + payload_length] = '\0';
                        buffer[bit_offset - 2 + payload_length] = '\0';
                        buffer[bit_offset - 1 + payload_length] = '\0';

                        int target_sd_bit_offset = bit_offset - 4;
                        if (buffer[target_sd_bit_offset] == 't' && buffer[target_sd_bit_offset + 1] == 'o') {
                            int target_sd = 0;
                            target_sd_bit_offset += 2;
                            while (buffer[target_sd_bit_offset] != '{') {
                                target_sd = target_sd * 10 + buffer[target_sd_bit_offset++] - '0';
                            }
                            if (send(target_sd, buffer, bit_offset - 4 + payload_length, 0) != bit_offset - 4 + payload_length) {
                                perror("send");
                            } else {
                                totalFramesSent++;
                            }
                        } else if (clientSocketDescToStreamerSocketDesc[sd] && clientSocketDescToStreamerSocketDesc[sd] != sd) {
                            if (send(clientSocketDescToStreamerSocketDesc[sd], buffer, bit_offset - 4 + payload_length, 0) != bit_offset - 4 + payload_length) {
                                perror("send");
                            } else {
                                totalFramesSent++;
                            }
                        } else {
                            for (auto clientForSocket = socketDescToClients[sd].begin(); clientForSocket != socketDescToClients[sd].end();) {
                                if (send(clientForSocket->socket_descriptor, buffer, bit_offset - 4 + payload_length, 0) != bit_offset - 4 + payload_length) {
                                    perror("send");
                                    clientForSocket = socketDescToClients[sd].erase(clientForSocket);
                                } else {
                                    totalFramesSent++;
                                    clientForSocket++;
                                }
                            }
                        }

                        continue;
                    }

                    std::unordered_map<std::string, std::string> headerMap = parse_header(buffer, buffer + valread);

                    if (headerMap.count("Sec-WebSocket-Key") == 1) {
                        printf(GREEN("Recieved ws request\n"));
                        printf(CYAN("%s\n"), buffer);

                        char sha1_handshake[40];
                        size_t handshake_sz;

                        memset(sha1_handshake, 0, sizeof(sha1_handshake));
                        if (ws_compute_handshake(headerMap["Sec-WebSocket-Key"].c_str(), sha1_handshake, &handshake_sz) != 0) {
                            /* failed to compute handshake. */
                            printf("failed to compute handshake\n");
                            continue;
                        }

                        //printf("%s\n", sha1_handshake);

                        std::string strWSResponse = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\n";
                        strWSResponse += "Sec-WebSocket-Accept: ";
                        strWSResponse += sha1_handshake;
                        strWSResponse += "\r\n";
                        const char *strWSResponseMsg = strWSResponse.c_str();

                        //send websocket response
                        if (send(sd, strWSResponseMsg, strlen(strWSResponseMsg), 0) != strlen(strWSResponseMsg)) {
                            perror("send");
                        }

                        previous_it->second.startedWS = 1;

                    } else {
                        printf(GREEN("Recieved http request\n"));
                        printf(RED("%s\n"), buffer);
                    }

                    /*for (auto x : headerMap)
                    {
                        //cout << x.first << " " << x.second << endl;
                        printf(">%s< -> >%s<\n", x.first.c_str(), x.second.c_str());
                    }*/
                    //send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}
