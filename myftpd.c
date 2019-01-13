//
// Created by 尾崎耀一 on 2019-01-13.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include "myftp.h"

void dump_message(struct myftph *);
void dump_data_message(struct myftph_data *);

int
main(int argc, char *argv[]) {

    char *current_dir;
    if (argc != 2) {
        fprintf(stderr, "%s: Usage: %s path/to/current/dir\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }
    current_dir = argv[1];
    chdir(current_dir);

    int server_socket;
    int client_socket;
    struct sockaddr_in server_socket_address;
    struct sockaddr_in client_socket_address;
    socklen_t client_socket_address_len = sizeof(struct sockaddr_in);

    // create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // prepare server_socket_address
    server_socket_address.sin_family = AF_INET;
    server_socket_address.sin_port = htons(MYFTP_PORT);
    server_socket_address.sin_addr.s_addr = INADDR_ANY;

    // bind server_socket with server_socket_address
    if (bind(server_socket, (struct sockaddr *)&server_socket_address, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept
    if ((server_socket = accept(server_socket, (struct sockaddr *)&client_socket_address, &client_socket_address_len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "accepted connection from IP: %s(port: %d)\n",
            inet_ntoa(client_socket_address.sin_addr), client_socket_address.sin_port);

    for (;;) {
        struct myftph buf;
        memset(&buf, 0, sizeof(buf));
        struct myftph_data buf_data;
        memset(&buf_data, 0, sizeof(buf_data));
        if (recv(server_socket, &buf, sizeof(struct myftph), 0) < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        if (buf.Length == 0) {
            switch (buf.Type) {
                case TYPE_QUIT:
                    fprintf(stderr, "<- recv QUIT message\n");
                    dump_message(&buf);
                    struct myftph reply;
                    memset(&reply, 0, sizeof(struct myftph));
                    reply.Type = TYPE_OK_COMMAND;
                    reply.Code = CODE_NO_DATA_FOLLOW;
                    reply.Length = 0;
                    if (send(server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                        perror("send @ QUIT");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "\t-> send OK message\n");
                    dump_message(&reply);
                    if (close(server_socket) < 0) {
                        perror("close");
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_SUCCESS);
                case TYPE_PWD:
                    fprintf(stderr, "<- recv PWD message\n");
                    dump_message(&buf);
                    char pathname[SIZE];
                    memset(pathname, 0, SIZE);
                    getcwd(pathname, SIZE);
                    struct myftph_data result;
                    memset(&result, 0, sizeof(result));
                    result.Type = TYPE_OK_COMMAND;
                    result.Code = CODE_NO_DATA_FOLLOW;
                    result.Length = (uint16_t) (strlen(pathname));
                    strncpy(result.Data, pathname, result.Length); // trim last '\0'
                    if (send(server_socket, &result, sizeof(struct myftph_data), 0) < 0) {
                        perror("send @ PWD");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "\t-> send OK message with data\n");
                    dump_data_message(&result);
                    break;
                default:
                    fprintf(stderr, "<- recv unknown message\n");
                    fprintf(stderr, "ERROR: recv unknown message, so do not quit\n");
                    break;
            }
        } else {
            buf_data.Type = buf.Type;
            buf_data.Code = buf.Code;
            buf_data.Length = buf.Length;
            if (recv(server_socket, &buf_data, buf_data.Length, 0) < 0) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            switch (buf.Type) {
                case TYPE_CWD:
                    break;
                case TYPE_LIST:
                    break;
                case TYPE_RETR:
                    break;
                case TYPE_STOR:
                    break;
                default:
                    fprintf(stderr, "<- recv unknown message\n");
                    fprintf(stderr, "ERROR: recv unknown message, so do not quit\n");
                    break;
            }
        }
    }
}

void
dump_message(struct myftph *message)
{
    switch (message->Type) {
        case TYPE_QUIT:
            fprintf(stderr, "+-- [ (<-) QUIT ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "+------------------------\n");
            break;
        case TYPE_PWD:
            fprintf(stderr, "+-- [ (<-) PWD ]---------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "+------------------------\n");
            break;
        case TYPE_CWD:          // client -> server
            break;
        case TYPE_LIST:         // client -> server
            break;
        case TYPE_RETR:         // client -> server
            break;
        case TYPE_STOR:         // client -> server
            break;
        case TYPE_OK_COMMAND:   // client <- server
            fprintf(stderr, "\t+-- [ (->) OK ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[TYPE_OK_COMMAND]);
            fprintf(stderr, "\t|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_CMD_ERR:      // client <- server
            break;
        case TYPE_OTHER_ERR:    // client <- server
            break;
        case TYPE_DATA:         // client <- server
            break;
        default:
            return;
    }
}

void
dump_data_message(struct myftph_data *message)
{
    char tmp[SIZE];
    switch (message->Type) {
        case TYPE_QUIT:
            fprintf(stderr, "\t+-- [ (->) QUIT ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_PWD:
            fprintf(stderr, "\t+-- [ (->) PWD ]---------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_CWD:
            break;
        case TYPE_LIST:
            break;
        case TYPE_RETR:
            break;
        case TYPE_STOR:
            break;
        case TYPE_OK_COMMAND:
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "\t+-- [ (->) OK ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[TYPE_OK_COMMAND]);
            fprintf(stderr, "\t|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t|\tData: %s\n", message->Data);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_CMD_ERR:
            break;
        case TYPE_OTHER_ERR:
            break;
        case TYPE_DATA:
            break;
        default:
            return;
    }
}