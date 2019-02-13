#include <stdlib.h>
#include <stdio.h>
#include "function-client.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage : ./%s <IP_sever> <port_sever> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    start_client(argv[1], argv[2]);

    return 0;
}
