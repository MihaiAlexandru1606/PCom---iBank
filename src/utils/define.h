#ifndef TEMA_PC_DEFINE_H
#define TEMA_PC_DEFINE_H

#include <stdio.h>
#include <stdlib.h>


/** constate pentru contul bancar */
/** cu 1 mai mult pentru terminatorul de sir */
#define MAX_SURNAME         13
#define MAX_NAME            13
#define LEN_NUMBER_CARD     7
#define LEN_PIN             5
#define MAX_PASSWORD        9
#define MAX_FAILED_LOGGING  3
#define BUFFER_LEN          100

#define DEFAUL_IP           "127.0.0.1"
#define max(a, b)           ((a < b) ? b : a)
#define MAX_BACKLOG         100
#define COMMAND_LEN         10


/** tip boolean */
#define Bool                int
#define TRUE                1
#define FALSE               0

/** codurile de erorare */
#define NOT_LOGGING         -1
#define IS_LOGGED           -2
#define WRONG_PIN           -3
#define WRONG_NUMBER_CARD   -4
#define CARD_LOCKED         -5
#define FAILED_OPERATION    -6
#define FAILED_UNLOCKING    -7
#define NO_MONEY            -8
#define CANCELED_OPERATION  -9
#define ERROR               -10
#define NOT_CONTAINS        -11

#define CHECK(condition, message, exitProgram)  \
            if (!(condition)) {                 \
                perror(message);                \
                if (exitProgram == TRUE)        \
                    exit(EXIT_FAILURE);         \
            }

#endif
