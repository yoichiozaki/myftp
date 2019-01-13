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

#define COMMANDLINE_LEN         1024
#define COMMANDLINE_MAX_ARGC    128
#define DELIM                   " \t\n"

void print_welcome();
void print_help();

void dump_message(struct myftph *);
void dump_data_message(struct myftph_data *);

int
main(int argc, char *argv[])
{
    char *server_host_name;
    struct sockaddr_in server_socket_address;
    int client_socket;
    unsigned int **addrptr;
    int connected = 0;

    if (argc != 2) {
        fprintf(stderr, "%s: Usage: %s <server host name>\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }
    server_host_name = argv[1];

    // create client's socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // prepare server_socket_address and connect to server
    server_socket_address.sin_family = AF_INET;
    server_socket_address.sin_port = htons(MYFTP_PORT);
    server_socket_address.sin_addr.s_addr = inet_addr(server_host_name);
    if (server_socket_address.sin_addr.s_addr == 0xffffffff) {
        struct hostent *host;
        host = gethostbyname(server_host_name);
        if (host == NULL) {
            perror("gethostbyname");
            exit(EXIT_FAILURE);
        }
        addrptr = (unsigned int **)host->h_addr_list;
        while (*addrptr != NULL) {
            server_socket_address.sin_addr.s_addr = *(*addrptr);
            connected = connect(client_socket, (struct sockaddr *)&server_socket_address, sizeof(struct sockaddr_in));
            if (connected == 0) {
                break;
            }
            addrptr++;
        }
    } else {
        if (connect(client_socket, (struct sockaddr *)&server_socket_address, sizeof(struct sockaddr_in)) != 0) {
            perror("connect");
            exit(EXIT_FAILURE);
        }
    }
    if (connected < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    char current_dir[SIZE];

    for (;;) {
        char commandline[COMMANDLINE_LEN];
        char *commandline_argv[COMMANDLINE_MAX_ARGC];
        memset(commandline, 0, sizeof(commandline));
        fprintf(stdout, "[local@%s]\n", getcwd(current_dir, SIZE));
        fprintf(stdout, "myftp%% ");
        if (fgets(commandline, COMMANDLINE_LEN, stdin) == NULL) {
            fprintf(stderr, "Bye\n");
            exit(EXIT_SUCCESS);
        }
        char *cp = commandline;
        int commandline_argc;
        for (commandline_argc = 0; commandline_argc < COMMANDLINE_MAX_ARGC; commandline_argc++) {
            if ((commandline_argv[commandline_argc] = strtok(cp, DELIM)) == NULL) {
                break;
            }
            cp = NULL;
        }

        if (commandline_argv[0] == NULL) {
            continue;
        }

        // QUIT command
        if (strcmp(commandline_argv[0], "quit") == 0) {
            // fprintf(stderr, "[DEBUG] QUIT\n");
            if (commandline_argc != 1) {
                fprintf(stderr, "ERROR: command syntax error\n");
                continue;
            }
            struct myftph quit_message;
            memset(&quit_message, 0, sizeof(quit_message));
            quit_message.Type = TYPE_QUIT;
            if (send(client_socket, &quit_message, sizeof(quit_message), 0) < 0) {
                perror("send @ QUIT");
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "\t-> send QUIT message\n");
            dump_message(&quit_message);
            struct myftph reply;
            memset(&reply, 0, sizeof(reply));
            if (recv(client_socket, &reply, sizeof(reply), 0) < 0) {
                perror("recv @ QUIT");
                exit(EXIT_FAILURE);
            }
            if (reply.Length != 0) {
                fprintf(stderr, "<- recv unexpected message\n");
                fprintf(stderr, "ERROR: recv unexpected message, so do not quit\n");
                continue;
            } else {
                if (reply.Type == TYPE_OK_COMMAND && reply.Code == CODE_NO_DATA_FOLLOW) {
                    if (close(client_socket) < 0) {
                        perror("close @ QUIT");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr, "<- recv OK message\n");
                    dump_message(&reply);
                    fprintf(stderr, "... quiting ...\n");
                    exit(EXIT_SUCCESS);
                } else {
                    fprintf(stderr, "<- recv unexpected message\n");
                    fprintf(stderr, "ERROR: recv unexpected message, so do not quit\n");
                    continue;
                }
            }
        }

        // PWD command
        if (strcmp(commandline_argv[0], "pwd") == 0) {
            // fprintf(stderr, "[DEBUG] PWD\n");
            if (commandline_argc != 1) {
                fprintf(stderr, "ERROR: command syntax error\n");
                continue;
            }
            struct myftph pwd_message;
            memset(&pwd_message, 0, sizeof(pwd_message));
            pwd_message.Type = TYPE_PWD;
            pwd_message.Length = 0;
            if (send(client_socket, &pwd_message, sizeof(struct myftph), 0) < 0) {
                perror("send @ PWD");
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "\t-> send PWD message\n");
            dump_message(&pwd_message);
            struct myftph_data reply;
            memset(&reply, 0, sizeof(reply));

            ssize_t recved;
            if ((recved = recv(client_socket, &reply, sizeof(struct myftph), 0)) < 0) {
                perror("recv @ PWD header");
                exit(EXIT_FAILURE);
            }
            // fprintf(stderr, "recved: %ld\n", recved);

            if (reply.Length == 0) {
                fprintf(stderr, "<- recv unexpected message\n");
                fprintf(stderr, "ERROR: recv unexpected message, so do not quit\n");
                continue;
            }

            fprintf(stderr, "<- recv OK message\n");
            if ((recved = recv(client_socket, &reply.Data, reply.Length, 0)) < 0) {
                perror("recv @ PWD data");
                exit(EXIT_FAILURE);
            }
            char garbage[DATASIZE - reply.Length];
            if ((recved = recv(client_socket, garbage, (size_t) (DATASIZE - reply.Length), 0)) < 0) {
                perror("recv @ PWD garbage");
                exit(EXIT_FAILURE);
            }

            dump_data_message(&reply);

            if (reply.Type == TYPE_OK_COMMAND && reply.Code == CODE_NO_DATA_FOLLOW) {
                char tmp[reply.Length + 1];
                strncpy(tmp, reply.Data, reply.Length+1);
                fprintf(stderr, "%s\n", reply.Data);
                continue;
            } else {
                fprintf(stderr, "<- recv unexpected message\n");
                fprintf(stderr, "ERROR: recv unexpected message, so do not quit\n");
                continue;
            }
        }

        // CD command
        if (strcmp(commandline_argv[0], "cd") == 0) {
            fprintf(stderr, "[DEBUG] CD\n");

        }

        // DIR command
        if (strcmp(commandline_argv[0], "dir") == 0) {
            fprintf(stderr, "[DEBUG] DIR\n");

        }

        // LPWD command
        if (strcmp(commandline_argv[0], "lpwd") == 0) {
            // fprintf(stderr, "[DEBUG] LPWD\n");
            if (commandline_argc != 1) {
                fprintf(stderr, "ERROR: command syntax error\n");
                continue;
            }
            char pathname[SIZE];
            memset(pathname, 0, SIZE);
            getcwd(pathname, SIZE);
            fprintf(stdout,"%s\n", pathname);
            continue;
        }

        // LCD command
        if (strcmp(commandline_argv[0], "lcd") == 0) {
            // fprintf(stderr, "[DEBUG] LCD\n");
            if (commandline_argc != 2) {
                fprintf(stderr, "ERROR: command syntax error\n");
                continue;
            }
            chdir(commandline_argv[1]);
            char path[SIZE];
            memset(path, 0, SIZE);
            getcwd(path, SIZE);
            fprintf(stdout,"%s\n", path);
            continue;
        }

        // LDIR command
        if (strcmp(commandline_argv[0], "ldir") == 0) {
            // fprintf(stderr, "[DEBUG] LDIR\n");
            if (!(commandline_argc == 2 || commandline_argc == 1)) {
                fprintf(stderr, "ERROR: command syntax error\n");
                continue;
            }
            DIR *dir;
            struct dirent *dp;
            struct stat statbuf;
            char path[SIZE];
            memset(path, '\0', SIZE);
            if (commandline_argc == 1) { // ldir
                strcpy(path,".");
            } else { // ldir path/to/file/or/dir
                strcpy(path, commandline_argv[1]);
            }
            if((dir = opendir(path)) == NULL) {
                perror("opendir");
                fprintf(stderr, "ERROR: command execution error\n");
                continue;
            }
            for(dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
                stat(dp->d_name, &statbuf);
                printf((S_ISDIR(statbuf.st_mode)) ? "d" : "-");
                if(S_ISDIR(statbuf.st_mode)) { // dir
                    printf((statbuf.st_mode & S_IRUSR) ? "r" : "-");
                    printf((statbuf.st_mode & S_IWUSR) ? "w" : "-");
                    printf((statbuf.st_mode & S_IXUSR) ? "x" : "-");
                    printf((statbuf.st_mode & S_IRGRP) ? "r" : "-");
                    printf((statbuf.st_mode & S_IWGRP) ? "w" : "-");
                    printf((statbuf.st_mode & S_IXGRP) ? "x" : "-");
                    printf((statbuf.st_mode & S_IROTH) ? "r" : "-");
                    printf((statbuf.st_mode & S_IWOTH) ? "w" : "-");
                    printf((statbuf.st_mode & S_IXOTH) ? "x" : "-");
                    printf("\t%s/\n\tlast access: %s\tlast modified: %s\tsize: %lld bytes\n",
                           dp->d_name,
                           asctime(localtime(&statbuf.st_atimespec.tv_sec)),
                           asctime(localtime(&statbuf.st_mtimespec.tv_sec)),
                           statbuf.st_size
                    );
                } else {
                    printf((statbuf.st_mode & S_IRUSR) ? "r" : "-");
                    printf((statbuf.st_mode & S_IWUSR) ? "w" : "-");
                    printf((statbuf.st_mode & S_IXUSR) ? "x" : "-");
                    printf((statbuf.st_mode & S_IRGRP) ? "r" : "-");
                    printf((statbuf.st_mode & S_IWGRP) ? "w" : "-");
                    printf((statbuf.st_mode & S_IXGRP) ? "x" : "-");
                    printf((statbuf.st_mode & S_IROTH) ? "r" : "-");
                    printf((statbuf.st_mode & S_IWOTH) ? "w" : "-");
                    printf((statbuf.st_mode & S_IXOTH) ? "x" : "-");
                    printf("\t%s\n\tlast access: %s\tlast modified: %s\tsize: %lld bytes\n",
                           dp->d_name,
                           asctime(localtime(&statbuf.st_atimespec.tv_sec)),
                           asctime(localtime(&statbuf.st_mtimespec.tv_sec)),
                           statbuf.st_size
                    );
                }
            }
            closedir(dir);
            continue;
        }

        // GET command
        if (strcmp(commandline_argv[0], "get") == 0) {
            fprintf(stderr, "[DEBUG] GET\n");
        }

        // PUT command
        if (strcmp(commandline_argv[0], "put") == 0) {
            fprintf(stderr, "[DEBUG] PUT\n");

        }

        // HELP command
        if (strcmp(commandline_argv[0], "help") == 0) {
            // fprintf(stderr, "[DEBUG] HELP\n");
            if (commandline_argc != 1) {
                fprintf(stderr, "command execution error: Usage: help\n");
                continue;
            }
            print_help();
            continue;
        }
    }
}

void
print_welcome()
{
    fprintf(stderr, "\t\n");
    fprintf(stderr, "\t __       __                  ________  ________  _______  \n");
    fprintf(stderr, "\t/  \\     /  |                /        |/        |/       \\ \n");
    fprintf(stderr, "\t$$  \\   /$$ | __    __       $$$$$$$$/ $$$$$$$$/ $$$$$$$  |\n");
    fprintf(stderr, "\t$$$  \\ /$$$ |/  |  /  |      $$ |__       $$ |   $$ |__$$ |\n");
    fprintf(stderr, "\t$$$$  /$$$$ |$$ |  $$ |      $$    |      $$ |   $$    $$/ \n");
    fprintf(stderr, "\t$$ $$ $$/$$ |$$ |  $$ |      $$$$$/       $$ |   $$$$$$$/  \n");
    fprintf(stderr, "\t$$ |$$$/ $$ |$$ \\__$$ |      $$ |         $$ |   $$ |      \n");
    fprintf(stderr, "\t$$ | $/  $$ |$$    $$ |      $$ |         $$ |   $$ |      \n");
    fprintf(stderr, "\t$$/      $$/  $$$$$$$ |      $$/          $$/    $$/       \n");
    fprintf(stderr, "\t             /  \\__$$ |                                    \n");
    fprintf(stderr, "\t             $$    $$/                                     \n");
    fprintf(stderr, "\t              $$$$$$/                                      \n");
    fprintf(stderr, "\t\n");
}

void
print_help()
{
    print_welcome();
    fprintf(stderr, "ABSTRACT\n");
    fprintf(stderr, "\tThis is the client software that implements the myFTP protocol.\n");
    fprintf(stderr, "\tmyftpc establishes a TCP connection with myfptd running on the host specified by the argument. \n"
                    "\tWhen the TCP connection is established, myftpc displays \"myFTP%%\" as a prompt \n"
                    "\tand waits for command input from the user. When a command is input, it sends a command message \n"
                    "\tto the server as necessary and waits for reception of a reply message. \n"
                    "\tThen it displays the prompt again and waits for command input from the user.\n\n");

    fprintf(stderr, "USAGE\n");
    fprintf(stderr, "\t./myftpc <server host name>\n\n");

    fprintf(stderr, "PORT\n");
    fprintf(stderr, "\tmyFTP protocol uses 50021 port.\n\n");

    fprintf(stderr, "COMMANDS\n");
    fprintf(stderr, "\tThe command list is as follows.\n");
    fprintf(stderr, "\tcommand                     arguments                                                              functionality                                         \n");
    fprintf(stderr, "\t-------- ------------------------------------------------ -----------------------------------------------------------------------------------------------\n");
    fprintf(stderr, "\tquit      -                                                Quit myftpc.                                                                                  \n");
    fprintf(stderr, "\tpwd       -                                                Print working directory at the server.                                                        \n");
    fprintf(stderr, "\tcwd       path/to/directory                                Change working directory at the server.                                                       \n");
    fprintf(stderr, "\tdir       [path/to/directory/or/file]                      Show some info of the file/directory at the server.                                           \n");
    fprintf(stderr, "\tlpwd      -                                                Print working directory at local.                                                             \n");
    fprintf(stderr, "\tlcd       path/to/directory                                Change directory at local.                                                                    \n");
    fprintf(stderr, "\tldir      [path/to/directory/or/file]                      Show some info of the file/directory at local.                                                \n");
    fprintf(stderr, "\tget       path/to/file/at/server [path/to/file/at/local]   Transfer the file on the server specified by path/to/file/at/server to path/to/file/at/local. \n");
    fprintf(stderr, "\tput       path/to/file/at/local [path/to/file/at/server]   Transfer the local file specified by path/to/file/at/local to path/to/file/at/server.         \n");
}

void
dump_message(struct myftph *message)
{
    switch (message->Type) {
        case TYPE_QUIT:         // client -> server
            fprintf(stderr, "\t+-- [ (->) QUIT ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_PWD:          // client -> server
            fprintf(stderr, "\t+-- [ (->) PWD ]---------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t+------------------------\n");
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
            fprintf(stderr, "+-- [ (<-) OK ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[TYPE_OK_COMMAND]);
            fprintf(stderr, "|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "|\tLength: %d\n", message->Length);
            fprintf(stderr, "+------------------------\n");
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
        case TYPE_QUIT:         // client -> server
            fprintf(stderr, "\t+-- [ (->) QUIT ]--------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t+------------------------\n");
            break;
        case TYPE_PWD:          // client -> server
            fprintf(stderr, "\t+-- [ (->) PWD ]---------\n");
            fprintf(stderr, "\t|\tType: %s\n", MESSAGE_TYPE_NAME[message->Type]);
            fprintf(stderr, "\t+------------------------\n");
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
            strncpy(tmp, message->Data, message->Length + 1);
            fprintf(stderr, "+-- [ (<-) OK ]--------\n");
            fprintf(stderr, "|\tType: %s\n", MESSAGE_TYPE_NAME[TYPE_OK_COMMAND]);
            fprintf(stderr, "|\tCode: %s\n", OK_CODE_NAME[message->Code]);
            fprintf(stderr, "|\tLength: %d\n", message->Length);
            fprintf(stderr, "|\tData: %s\n", message->Data);
            fprintf(stderr, "+------------------------\n");
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