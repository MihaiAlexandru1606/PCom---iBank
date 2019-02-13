
#ifndef _FUNCTION_CLIENT_H
#define _FUNCTION_CLIENT_H

#pragma pack(1)

#include "utils/define.h"

typedef struct {
    Bool isLogged;
    char currentNumberCard[LEN_NUMBER_CARD];
} Logging;

#pragma pack()
void start_client(const char *IP_sever, const char *port);

#endif

