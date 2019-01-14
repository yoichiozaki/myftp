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
void dump_data_message(struct myftph_data *, int);

int
main(int argc, char *argv[]) {

    char *current_dir;
    char dirbuf[SIZE];
    if (argc != 2 && argc != 1) {
        fprintf(stderr, "%s: Usage: %s [path/to/current/dir]\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argc == 1) {
        current_dir = argv[1];
    } else {
        getcwd(dirbuf, sizeof(dirbuf));
        current_dir = dirbuf;
    }
    chdir(current_dir);

    int connected_server_socket;
    int server_socket;

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

    // bind connected_server_socket with server_socket_address
    if (bind(server_socket, (struct sockaddr *)&server_socket_address, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pid_t pid, cpid;
    int status;

    for (;;) {

        // accept
        if ((connected_server_socket = accept(server_socket, (struct sockaddr *)&client_socket_address, &client_socket_address_len)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        if ((pid = fork()) > 0) {
            while ((cpid = waitpid(-1, &status, WNOHANG)) > 0) {
                ;
            }
            if (cpid < 0 && errno != ECHILD) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        } else if (pid == 0) {
            break;
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr, "FORKED(pid = %d): accepted connection from IP: %s(port: %d)\n",
            getpid(), inet_ntoa(client_socket_address.sin_addr), client_socket_address.sin_port);

    for (;;) {
        struct myftph buf;
        memset(&buf, 0, sizeof(buf));
        struct myftph_data buf_data;
        memset(&buf_data, 0, sizeof(buf_data));
        if (recv(connected_server_socket, &buf, sizeof(struct myftph), 0) < 0) {
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
                    reply.Code = CODE_WITH_NO_DATA;
                    reply.Length = 0;
                    if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                        perror("send @ QUIT");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "\t-> send OK message\n");
                    dump_message(&reply);
                    if (close(connected_server_socket) < 0) {
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
                    result.Code = CODE_WITH_NO_DATA;
                    result.Length = (uint16_t) (strlen(pathname));
                    strncpy(result.Data, pathname, result.Length); // trim last '\0'
                    if (send(connected_server_socket, &result, sizeof(struct myftph_data), 0) < 0) {
                        perror("send @ PWD");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "\t-> send OK message with data\n");
                    dump_data_message(&result, CODE_DATA_FOLLOW_S_TO_C);
                    break;
                default:
                    fprintf(stderr, "<- recv unknown message\n");
                    fprintf(stderr, "ERROR: recv unknown message\n");
                    break;
            }
            continue;
        } else {
            buf_data.Type = buf.Type;
            buf_data.Code = buf.Code;
            buf_data.Length = buf.Length;
            if (recv(connected_server_socket, &buf_data.Data, buf_data.Length, 0) < 0) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            char garbage[DATASIZE - buf_data.Length];
            if (recv(connected_server_socket, garbage, (size_t) (DATASIZE - buf_data.Length), 0) < 0) {
                perror("recv @ garbage");
                exit(EXIT_FAILURE);
            }
            char dst[buf.Length+1];
            char target[buf.Length+1];
            struct myftph reply;
            memset(&reply, 0, sizeof(struct myftph));
            char result[DATASIZE*10];
            memset(&result, 0, sizeof(result));
            size_t result_size = 0;
            char result_tmp[DATASIZE*10];
            memset(&result_tmp, 0, sizeof(result_tmp));
            struct myftph_data reply_data;
            memset(&reply_data, 0, sizeof(struct myftph_data));
            ssize_t sent_size = 0;
            char *now;
            switch (buf.Type) {
                case TYPE_CWD:
                    fprintf(stderr, "<- recv CWD message\n");
                    dump_data_message(&buf_data, 99);
                    memset(&dst, 0, sizeof(dst));
                    strncpy(dst, buf_data.Data, buf_data.Length);
                    reply.Length = 0;
                    if (chdir(dst) == 0) {
                        reply.Type = TYPE_OK_COMMAND;
                        reply.Code = CODE_WITH_NO_DATA;
                        if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                            perror("send @ CWD");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "\t-> send OK message\n");
                        dump_message(&reply);
                        break;
                    } else {
                        switch (errno) {
                            case EACCES:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_ACCESS;
                                break;
                            case ENOENT:
                            case ENOTDIR:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_SUCH_FILES;
                                break;
                            default:
                                reply.Type = TYPE_UNKWN_ERR;
                                reply.Code = CODE_UNKNOWN_ERROR;
                                break;
                        }
                    }
                    if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                        perror("send @ CWD");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "\t-> send ERR message\n");
                    dump_message(&reply);
                    break;
                case TYPE_LIST:
                    fprintf(stderr, "<- recv LIST message\n");
                    dump_data_message(&buf_data, 99);
                    memset(&target, 0, sizeof(target));
                    strncpy(target, buf_data.Data, buf_data.Length);
                    DIR *dir;
                    struct dirent *dp;
                    struct stat statbuf;
                    if((dir = opendir(target)) == NULL) {
                        perror("opendir");
                        fprintf(stderr, "ERROR: command execution error\n");
                        memset(&reply, 0, sizeof(struct myftph));
                        switch (errno) {
                            case EACCES:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_ACCESS;
                                break;
                            case ENOENT:
                            case ENOTDIR:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_SUCH_FILES;
                                break;
                            default:
                                reply.Type = TYPE_UNKWN_ERR;
                                reply.Code = CODE_UNKNOWN_ERROR;
                                break;
                        }
                        if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                            perror("send @ LIST ERR");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "\t-> send ERR message\n");
                        dump_message(&reply);
                        break;
                    }
                    for(dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
                        stat(dp->d_name, &statbuf);
                        strncpy(result, (S_ISDIR(statbuf.st_mode)) ? "d" : "-", 1);
                        if(S_ISDIR(statbuf.st_mode)) { // dir
                            strcat(result, (statbuf.st_mode & S_IRUSR) ? "r" : "-");
                            strcat(result, (statbuf.st_mode & S_IWUSR) ? "w" : "-");
                            strcat(result, (statbuf.st_mode & S_IWUSR) ? "x" : "-");
                            strcat(result, (statbuf.st_mode & S_IRGRP) ? "r" : "-");
                            strcat(result, (statbuf.st_mode & S_IWGRP) ? "w" : "-");
                            strcat(result, (statbuf.st_mode & S_IXGRP) ? "x" : "-");
                            strcat(result, (statbuf.st_mode & S_IROTH) ? "r" : "-");
                            strcat(result, (statbuf.st_mode & S_IWOTH) ? "w" : "-");
                            strcat(result, (statbuf.st_mode & S_IXOTH) ? "x" : "-");
                            sprintf(result_tmp, "\t%s/\n\tlast access: %s\tlast modified: %s\tsize: %lld bytes\n",
                                    dp->d_name,
                                    asctime(localtime(&statbuf.st_atime)),
                                    asctime(localtime(&statbuf.st_mtime)),
                                    statbuf.st_size);
                            strncat(result, result_tmp, strlen(result_tmp));
                        } else {
                            strcat(result, (statbuf.st_mode & S_IRUSR) ? "r" : "-");
                            strcat(result, (statbuf.st_mode & S_IWUSR) ? "w" : "-");
                            strcat(result, (statbuf.st_mode & S_IWUSR) ? "x" : "-");
                            strcat(result, (statbuf.st_mode & S_IRGRP) ? "r" : "-");
                            strcat(result, (statbuf.st_mode & S_IWGRP) ? "w" : "-");
                            strcat(result, (statbuf.st_mode & S_IXGRP) ? "x" : "-");
                            strcat(result, (statbuf.st_mode & S_IROTH) ? "r" : "-");
                            strcat(result, (statbuf.st_mode & S_IWOTH) ? "w" : "-");
                            strcat(result, (statbuf.st_mode & S_IXOTH) ? "x" : "-");
                            sprintf(result_tmp, "\t%s/\n\tlast access: %s\tlast modified: %s\tsize: %lld bytes\n",
                                    dp->d_name,
                                    asctime(localtime(&statbuf.st_atime)),
                                    asctime(localtime(&statbuf.st_mtime)),
                                    statbuf.st_size);
                            strncat(result, result_tmp, strlen(result_tmp));
                        }
                    }
                    closedir(dir);

                    memset(&reply, 0, sizeof(struct myftph));
                    reply.Type = TYPE_OK_COMMAND;
                    reply.Code = CODE_DATA_FOLLOW_S_TO_C;
                    if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                        perror("send @ DIR OK");
                        exit(EXIT_FAILURE);
                    }
                    dump_message(&reply);
                    fprintf(stderr, "\t-> send OK message\n");

                    // fprintf(stderr, "[debug] result size: %ld\n", strlen(result));
                    now = result;
                    while (result_size <= strlen(result)) {
                        memset(&reply_data, 0, sizeof(struct myftph_data));
                        reply_data.Type = TYPE_DATA;
                        result_size += sent_size;
                        if (result_size > strlen(result)) {
                            reply_data.Code = CODE_DATA_NO_FOLLOW;
                            reply_data.Length = DATASIZE;
                        } else {
                            reply_data.Code = CODE_DATA_FOLLOW;
                            reply_data.Length = DATASIZE;
                        }
                        strncpy(reply_data.Data, now, DATASIZE);
                        if ((sent_size = send(connected_server_socket, &reply_data, sizeof(struct myftph_data), 0)) < 0 ) {
                            perror("send @ DIR DATA");
                            exit(EXIT_FAILURE);
                        }
                        now += DATASIZE;
                        // fprintf(stderr, "[debug] result_size: %d\n", (int) result_size);
                        fprintf(stderr, "\t-> send DATA message\n");
                        dump_data_message(&reply_data, CODE_DATA_FOLLOW_S_TO_C);
                    }
                    continue;
                case TYPE_RETR:
                    fprintf(stderr, "<- recv RETR message\n");
                    dump_data_message(&buf_data, 99);

                    memset(&target, 0, sizeof(target));
                    strncpy(target, buf_data.Data, buf_data.Length);
                    FILE *target_file;
                    if((target_file = fopen(target, "r")) == NULL) {
                        perror("fopen");
                        fprintf(stderr, "ERROR: command execution error\n");
                        memset(&reply, 0, sizeof(struct myftph));
                        switch (errno) {
                            case EACCES:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_ACCESS;
                                break;
                            case ENOENT:
                            case ENOTDIR:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_SUCH_FILES;
                                break;
                            default:
                                reply.Type = TYPE_UNKWN_ERR;
                                reply.Code = CODE_UNKNOWN_ERROR;
                                break;
                        }
                        if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                            perror("send @ RETR ERR");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "\t-> send ERR message\n");
                        dump_message(&reply);
                        break;
                    }

                    memset(&reply, 0, sizeof(struct myftph));
                    reply.Type = TYPE_OK_COMMAND;
                    reply.Code = CODE_DATA_FOLLOW_S_TO_C;
                    if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                        perror("send @ RETR OK");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "\t-> send OK message\n");
                    dump_message(&reply);
                    size_t read;
                    for (;;) {
                        memset(&reply_data, 0, sizeof(struct myftph_data));
                        reply_data.Type = TYPE_DATA;
                        read = fread(&reply_data.Data, 1, DATASIZE, target_file);
                        // fprintf(stderr, "%s\n", reply_data.Data);
                        if (read < DATASIZE) {
                            if (feof(target_file) != 0) {
                                reply_data.Code = CODE_DATA_NO_FOLLOW;
                            } else {
                                fprintf(stderr, "ERROR: command execution error\n");
                                exit(EXIT_FAILURE);
                            }
                        } else {
                            reply_data.Code = CODE_DATA_FOLLOW;
                        }
                        reply_data.Length = (uint16_t) read;
                        if (send(connected_server_socket, &reply_data, sizeof(struct myftph_data), 0) < 0) {
                            perror("send @ RETR DATA");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "\t-> send DATA message\n");
                        dump_data_message(&reply_data, CODE_DATA_FOLLOW_S_TO_C);
                        if (reply_data.Code == CODE_DATA_NO_FOLLOW) {
                            break;
                        }
                    }
                    break;
                case TYPE_STOR:
                    fprintf(stderr, "<- recv STOR message\n");
                    dump_data_message(&buf_data, 99);
                    memset(&reply, 0, sizeof(struct myftph));
                    FILE *dst_file;
                    memset(&dst, 0, sizeof(dst));
                    strncpy(dst, buf_data.Data, buf_data.Length);
                    if ((dst_file = fopen(dst, "w+")) == NULL) {
                        perror("fopen src file");
                        switch (errno) {
                            case EACCES:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_ACCESS;
                                break;
                            case ENOENT:
                            case ENOTDIR:
                                reply.Type = TYPE_FILE_ERR;
                                reply.Code = CODE_NO_SUCH_FILES;
                                break;
                            default:
                                reply.Type = TYPE_UNKWN_ERR;
                                reply.Code = CODE_UNKNOWN_ERROR;
                                break;
                        }
                        if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                            perror("send @ STOR ERR");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "\t-> send ERR message\n");
                        dump_message(&reply);
                        break;
                    } else {
                        reply.Type = TYPE_OK_COMMAND;
                        reply.Code = CODE_DATA_FOLLOW_C_TO_S;
                        if (send(connected_server_socket, &reply, sizeof(struct myftph), 0) < 0) {
                            perror("send @ STOR ERR");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "\t-> send OK message\n");
                        dump_message(&reply);
                    }
                    for (;;) {
                        memset(&buf_data, 0, sizeof(struct myftph_data));
                        if (recv(connected_server_socket, &buf_data, sizeof(struct myftph_data), 0) < 0) {
                            perror("recv @ DATA");
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, "<- recv DATA message\n");
                        dump_data_message(&buf_data, CODE_DATA_FOLLOW_C_TO_S);
                        fwrite(buf_data.Data, 1, buf_data.Length, dst_file);
                        if (buf_data.Code == CODE_DATA_NO_FOLLOW) {
                            break;
                        }
                    }
                    if (fclose(dst_file) < 0) {
                        perror("fclose @ STOR");
                        exit(EXIT_FAILURE);
                    }
                    break;
                default:
                    fprintf(stderr, "<- recv unknown message\n");
                    fprintf(stderr, "ERROR: recv unknown message\n");
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
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_CMD_ERR:      // client <- server
            fprintf(stderr, "\t+-- [ (->) CMD ERR ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t|\tCode: %s\n", CMD_ERROR_CODE_NAME[message->Code]);
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_FILE_ERR:    // client <- server
            fprintf(stderr, "\t+-- [ (->) FILE ERR ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t|\tCode: %s\n", FILE_ERROR_CODE_NAME[message->Code]);
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_UNKWN_ERR:
            fprintf(stderr, "\t+-- [ (->) UNKWN ERR ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t|\tCode: %s\n", "CODE_UNKNOWN_ERROR(0x05)");
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_DATA:         // client <- server
            break;
        default:
            return;
    }
}

void
dump_data_message(struct myftph_data *message, int direction)
{
    char tmp[DATASIZE+1];
    memset(&tmp, 0, sizeof(tmp));
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
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "+-- [ (<-) CWD ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "|\tLength: %d\n", message->Length);
            fprintf(stderr, "|\tData: %s\n", tmp);
            fprintf(stderr, "+------------------------\n");
            break;
        case TYPE_LIST:
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "+-- [ (<-) LIST ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "|\tLength: %d\n", message->Length);
            fprintf(stderr, "|\tData: %s\n", tmp);
            fprintf(stderr, "+------------------------\n");
            break;
        case TYPE_RETR:
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "+-- [ (<-) RETR ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "|\tLength: %d\n", message->Length);
            fprintf(stderr, "|\tData: %s\n", tmp);
            fprintf(stderr, "+------------------------\n");
            break;
        case TYPE_STOR:
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "+-- [ (<-) STOR ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "|\tLength: %d\n", message->Length);
            fprintf(stderr, "|\tData: %s\n", tmp);
            fprintf(stderr, "+------------------------\n");
            break;
        case TYPE_OK_COMMAND:
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "\t+-- [ (->) OK ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "\t|\tLength: %d\n", message->Length);
            fprintf(stderr, "\t|\tData: %s\n", tmp);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_CMD_ERR:
            break;
        case TYPE_FILE_ERR:
            break;
        case TYPE_UNKWN_ERR:
            break;
        case TYPE_DATA:
            strncpy(tmp, message->Data, message->Length);
            if (direction == CODE_DATA_FOLLOW_S_TO_C) {
                strncpy(tmp, message->Data, message->Length + 1);
                fprintf(stderr, "\t+-- [ (->) DATA ]--------\n");
                fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
                fprintf(stderr, "\t|\tCode: %s\n", DATA_CODE_NAME[message->Code]);
                fprintf(stderr, "\t|\tLength: %ld\n", strlen(tmp));
                fprintf(stderr, "\t|\tData: %.*s(omitted below...)\n", 30, tmp);
                fprintf(stderr, "\t+------------------------\n");
                break;
            } else {
                strncpy(tmp, message->Data, message->Length + 1);
                fprintf(stderr, "+-- [ (<-) DATA ]--------\n");
                fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
                fprintf(stderr, "|\tCode: %s\n", DATA_CODE_NAME[message->Code]);
                fprintf(stderr, "|\tLength: %ld\n", strlen(tmp));
                fprintf(stderr, "|\tData: %.*s(omitted below...)\n", 30, tmp);
                fprintf(stderr, "+------------------------\n");
                break;
            }

        default:
            return;
    }
}