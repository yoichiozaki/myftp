//
// Created by 尾崎耀一 on 2019-01-13.
//

#ifndef MYFTP_MYFTP_H
#define MYFTP_MYFTP_H

#include <stdint.h>

#define MYFTP_PORT  50021

#define DATASIZE    1024
#define SIZE        256

// MYFTP message definition
struct myftph_data {
    uint8_t     Type;
    uint8_t     Code;
    uint16_t    Length;
    char        Data[DATASIZE];
};
struct myftph {
    uint8_t     Type;
    uint8_t     Code;
    uint16_t    Length;
};

// MYFTP header type
#define TYPE_QUIT           0x01
#define TYPE_PWD            0x02
#define TYPE_CWD            0x03
#define TYPE_LIST           0x04
#define TYPE_RETR           0x05
#define TYPE_STOR           0x06
#define TYPE_OK_COMMAND     0x10
#define TYPE_CMD_ERR        0x11
#define TYPE_FILE_ERR      0x12
#define TYPE_UNKWN_ERR      0x13
#define TYPE_DATA           0x20

// MYFTP header code
#define CODE_WITH_NO_DATA           0x00
#define CODE_DATA_FOLLOW_S_TO_C     0x01
#define CODE_DATA_FOLLOW_C_TO_S     0x02
#define CODE_SYNTAX_ERROR           0x00
#define CODE_UNDEFINED_COMMAND      0x01
#define CODE_PROTOCOL_ERROR         0x02
#define CODE_NO_SUCH_FILES          0x00
#define CODE_NO_ACCESS              0x01
#define CODE_UNKNOWN_ERROR          0x05
#define CODE_DATA_NO_FOLLOW         0x00
#define CODE_DATA_FOLLOW            0x01

// table for message type
char *MESSAGE_TYPE_NAME[] = {
        "ERROR(0x00)",
        "QUIT(0x01)",
        "PWD(0x02)",
        "CWD(0x03)",
        "LIST(0x04)",
        "RETR(0x05)",
        "STOR(0x06)",
        "ERROR(0x07)",
        "ERROR(0x08)",
        "ERROR(0x09)",
        "ERROR(0xa)",
        "ERROR(0xb)",
        "ERROR(0xc)",
        "ERROR(0xd)",
        "ERROR(0xe)",
        "ERROR(0xf)",
        "OK_COMMAND(0x10)",
        "CMD_ERR(0x11)",
        "FILE_ERR(0x12)",
        "UNKWN_ERR(0x13)",
        "ERROR(0x14)",
        "ERROR(0x15)",
        "ERROR(0x16)",
        "ERROR(0x17)",
        "ERROR(0x18)",
        "ERROR(0x19)",
        "ERROR(0x1a)",
        "ERROR(0x1b)",
        "ERROR(0x1c)",
        "ERROR(0x1d)",
        "ERROR(0x1e)",
        "ERROR(0x1f)",
        "DATA(0x20)"
};

// table for message OK code
char *OK_CODE_NAME[] = {
        "CODE_WITH_NO_DATA(0x00)",
        "CODE_DATA_FOLLOW_S_TO_C(0x01)",
        "CODE_DATA_FOLLOW_C_TO_S(0x02)"
};

// table for message CMD-ERROR code
char *CMD_ERROR_CODE_NAME[] = {
        "CODE_SYNTAX_ERROR(0x00)",
        "CODE_UNDEFINED_COMMAND(0x01)",
        "CODE_PROTOCOL_ERROR(0x02)"
};

// table for message FILE-ERROR code
char *FILE_ERROR_CODE_NAME[] = {
        "CODE_NO_SUCH_FILES(0x00)",
        "CODE_NO_ACCESS(0x01)"
};

// table for message DATA code
char *DATA_CODE_NAME[] = {
        "CODE_DATA_NO_FOLLOW(0x00)",
        "CODE_DATA_FOLLOW(0x01)"
};

#endif //MYFTP_MYFTP_H
