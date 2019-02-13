#include <stdlib.h>
#include <stdio.h>
#include "function-server.h"
#include "utils/define.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage : %s <port_sever> <users_data_file>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    start_server(DEFAUL_IP, argv[1], argv[2]);

    return 0;
}
