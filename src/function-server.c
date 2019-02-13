#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <error.h>
#include <assert.h>
#include "utils/database.h"
#include "utils/define.h"

/**
 * funtia crea realizeaza logarea unui client, cauta sa vada daca carul exista
 * daca nu cumva este blocat, daca este logat deja un alt client pentru
 * respentivul cont, daca punul este corect, daca toate conditile sunt
 * indeplinite asaociaza clentului contul
 * @param socket_client socket-ul clientului care face logare
 * @param numberCard pentru care se inceaca logarea
 * @param pin pinul cardului
 * @param database baza de date acuatla
 * @param numberFailedLog numarul de incrcari neresiute consecutiv
 * @return daca atentificare este nu este reusite intoarece codul de eroare
 *          specificat, daca s-a reusit intoarece 0
 */
static int login(int socket_client, char *numberCard, char *pin,
                 Database *database, int numberFailedLog) {
    int index = get_index_numberCard(database, numberCard);

    if (index == NOT_CONTAINS)
        return WRONG_NUMBER_CARD;

    /**daca un clinet incearca de 3 ori sau mai mult de 3 ori sa se logeze
     * dar nu reuserte si blocheaza cardul
     */

    if (database->failed_attempts[index] >= MAX_FAILED_LOGGING)
        return CARD_LOCKED;

    if (database->socket_client[index] != -1)
        return IS_LOGGED;

    if (strcmp(database->banckAccounts[index].pin, pin) == 0) {
        database->socket_client[index] = socket_client;
        database->failed_attempts[index] = 0;
        return index;

    } else {
        if (numberFailedLog == 2) {
            database->failed_attempts[index] = MAX_FAILED_LOGGING;
            return CARD_LOCKED;
        }
        return WRONG_PIN;
    }
}

/**
 * functia care delegeaza un client resetand toate informatiele de logare pt
 * respetivul cont, din cauza  modului de funtionare, niciodata clientul nu
 * o sa trimita logout daca nu este logat , nu va return ERROR
 * @param database baza de datae actala
 * @param socket_client socket-ul clientului care face logare
 * @return din cauza  modului de funtionare, niciodata clientul nu o sa trimita
 *          logout daca nu este logat , nu va return ERROR si doar 0
 */
static int logout(Database *database, int socket_client) {
    int index = get_index_socket(database, socket_client);

    if (index == NOT_CONTAINS)
        return ERROR;

    database->socket_client[index] = -1;
    database->failed_attempts[index] = 0;
    database->data_transfers[index].index_beneficiary = -1;
    return 0;
}

/**
 * Functia care returneza sold-ul curent
 * @param database baza de datae actuala
 * @param socket_client socket-ul clientului care face listsold
 * @return suma de bani disponibila, din cauza  modului de funtionare, niciodata
 * clientul nu o sa trimita istsod daca nu este logat
 */
static double listsold(Database *database, int socket_client) {
    int index = get_index_socket(database, socket_client);

    if (index == NOT_CONTAINS) {
        return ERROR;
    } else {
        return database->banckAccounts[index].sold;
    }
}

/**
 * funtia daca care spune daca se poate realizeza transferul
 * daca da marcheza data_transfer din database corespunzataore contului
 * @param database baza de datae actuala
 * @param socket_client socket-ul clientului care face transfer
 * @param sum socket-ul clientului care face logare
 * @param beneficiary numarului cui vrem sa tranferman
 * @return un cod de erorare daca nu sunt bani/ nu exita numarul cardlui
 *          altfel index-ul beneficiarului
 */
static int canTransfer(Database *database, int socket_client, double sum,
                       char *beneficiary) {
    int index = get_index_socket(database, socket_client);
    int index_beneficiary = get_index_numberCard(database, beneficiary);

    if (index == NOT_CONTAINS)
        return ERROR;

    if (index_beneficiary == NOT_CONTAINS)
        return WRONG_NUMBER_CARD;

    if (database->banckAccounts[index].sold < sum)
        return NO_MONEY;

    database->data_transfers[index].index_beneficiary = index_beneficiary;
    database->data_transfers[index].sum = sum;
    return index_beneficiary;
}

/**
 *  Functia care realizeza efectiv trasferul
 * @param database baza de datae actuala
 * @param socket_client socket-ul clientului care face transfer
 */
static void transfer(Database *database, int socket_client) {
    int index = get_index_socket(database, socket_client);
    int index_beneficiary = database->data_transfers[index].index_beneficiary;
    double sum = database->data_transfers[index].sum;

    database->banckAccounts[index].sold -= sum;
    database->banckAccounts[index_beneficiary].sold += sum;
}

/**
 * Fucunctia care verifica daca se poate face unlock pt un card
 * @param database baza de date acuatla
 * @param numberCard pentru care se inceaca unlock
 * @return 0 daca se poate atfel cod-ul de erorare
 */
static int check_unlock(Database *database, char *numberCard, int id) {
    int index = get_index_numberCard(database, numberCard);


    if (index == NOT_CONTAINS)
        return WRONG_NUMBER_CARD;

    if (database->failed_attempts[index] < MAX_FAILED_LOGGING)
        return FAILED_OPERATION; /** deblocare unui care nu este blocat */

    /** daca un client deja incearca sa deblocheze cardul */
    if (database->id_unlock[index] != -1 && database->id_unlock[index] != id)
        return FAILED_UNLOCKING;

    database->id_unlock[index] = id;

    return 0;
}

/**
 * Functia care realizeaza unlock
 * @param database baza de date acuatla
 * @param numberCard pentru care se inceaca unlock
 * @param secret_password parola secreta
 * @return daca unlock avut loc cu succes
 */
static int unlock(Database *database, char *numberCard, char *secret_password) {
    int index = get_index_numberCard(database, numberCard);

    if (index == NOT_CONTAINS)
        return WRONG_NUMBER_CARD;

    /**"eliberare" pentru increcari de deblocare pentru card */
    database->id_unlock[index] = -1;

    if (database->failed_attempts[index] < MAX_FAILED_LOGGING)
        return ERROR; /** deblocare unui care nu este blocat */

    if (strcmp(database->banckAccounts[index].secret_password,
               secret_password) == 0) {
        database->failed_attempts[index] = 0;
        return 0;
    } else {
        return FAILED_UNLOCKING;
    }
}

/**
 * functia care execta comenzile primita pe TCP de la client
 * @param socket_client socket-utul clientului care a trimis comanda sau
 *                      raspunsul (ex. raspunsul la transfer)
 * @param buffer primit de la recv
 * @param database baza de date actula
 * @param quit data tremui sa inchidem respentivul client
 */
static void run_command(int socket_client, char *buffer, Database *database,
                        Bool *quit) {
    char command[COMMAND_LEN];
    memcpy(command, buffer, COMMAND_LEN);


    if (strcmp(command, "login") == 0) {
        char numberCard[LEN_NUMBER_CARD], pin[LEN_PIN];
        int numberFailedLog;

        memset(numberCard, 0, LEN_NUMBER_CARD);
        memset(pin, 0, LEN_PIN);
        memcpy(numberCard, buffer + 10, LEN_NUMBER_CARD);
        memcpy(pin, buffer + 20, LEN_PIN);
        memcpy(&numberFailedLog, buffer + 30, sizeof(int));
        int rez_login = login(socket_client, numberCard, pin, database,
                              numberFailedLog);
        memset(buffer, 0, BUFFER_LEN);
        memcpy(buffer, &rez_login, sizeof(int));
        printf("Comanda primita de la client %d : login %s %s\n", socket_client,
               numberCard, pin);

        if (rez_login >= 0) {
            memcpy(buffer + 10, database->banckAccounts[rez_login].surname,
                   MAX_SURNAME);
            memcpy(buffer + 30, database->banckAccounts[rez_login].name,
                   MAX_NAME);
        }

    } else if (strcmp(command, "logout") == 0) {
        int rez_logout = logout(database, socket_client);
        memset(buffer, 0, BUFFER_LEN);
        memcpy(buffer, &rez_logout, sizeof(int));
        printf("Comanda primita de la client %d : logout\n", socket_client);

    } else if (strcmp(command, "listsold") == 0) {
        double rez_listsold = listsold(database, socket_client);
        memset(buffer, 0, BUFFER_LEN);
        memcpy(buffer, &rez_listsold, sizeof(double));
        printf("Comanda primita de la client %d : listsold \n", socket_client);

    } else if (strcmp(command, "transfer") == 0) {
        char beneficiary[LEN_NUMBER_CARD];
        double sum;
        memcpy(beneficiary, buffer + 10, LEN_NUMBER_CARD);
        memcpy(&sum, buffer + 20, sizeof(double));
        printf("Comanda primita de la client %d : transfer %s %lf\n",
               socket_client, beneficiary, sum);
        int rez_canTransfer = canTransfer(database, socket_client, sum,
                                          beneficiary);

        memset(buffer, 0, BUFFER_LEN);
        memcpy(buffer, &rez_canTransfer, sizeof(int));
        if (rez_canTransfer >= 0) {
            memcpy(buffer + 10,
                   database->banckAccounts[rez_canTransfer].surname,
                   MAX_SURNAME);
            memcpy(buffer + 30, database->banckAccounts[rez_canTransfer].name,
                   MAX_NAME);
        }

    } else if (strcmp(command, "quit") == 0) {
        *quit = TRUE;
        logout(database, socket_client);
        printf("Comanda primita de la client %d : quit\n", socket_client);
    } else {
        /** singura posibilitate este raspunsul pentru transfer */
        memset(buffer, 0, BUFFER_LEN);
        printf("Raspunsul clientului %d prentru transferul banilor este %c\n",
               socket_client, command[0]);
        int rez;
        if (command[0] == 'y') {
            rez = 0; /** semnifica ca operatia a avut loc cu success */
            transfer(database, socket_client);
            memcpy(buffer, &rez, sizeof(int));
        } else {
            rez = CANCELED_OPERATION;
            memcpy(buffer, &rez, sizeof(int));
        }
    }
}

/**
 * functia care efectual efectiv comportamentul servarului
 * initializeaza conxiune TCP si UDP, accepta noi ceriri de conectare, primeste
 * comenzile de pe TCP si le paseaza lui run_comman, daca primeste pe UDP atunci
 * incepe procedura de unlock, iar daca citeste de la tastarul "quit" se inchide
 * @param IP ip server-ului
 * @param port port-ul server-ului
 * @param database baza de date citita
 */
static void run_server(char *IP, char *port, Database *database) {
    char buffer[BUFFER_LEN] = {0};
    fd_set read_fileDescriptors;
    fd_set temp_fileDescriptors;
    int portNumber = atoi(port);
    int i, fdmax;
    struct sockaddr_in severAddressTCP, severAddressUDP, clientAdrress;
    Bool finish = FALSE;

    /** crearea socket-urilor pentru UDP si TCP */
    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    int socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(socket_udp != -1, "-10 : Open socket UDP", TRUE);
    CHECK(socket_tcp != -1, "-10 : Open socket TCP", TRUE);

    severAddressUDP.sin_family = AF_INET;
    severAddressUDP.sin_port = (in_port_t) portNumber;
    CHECK(inet_aton(IP, &severAddressUDP.sin_addr) > 0,
          "-10 : IP Sever Address", TRUE);

    severAddressTCP.sin_family = AF_INET;
    severAddressTCP.sin_port = (in_port_t) portNumber;
    severAddressTCP.sin_addr.s_addr = INADDR_ANY;  //foloseste adresa IP masinii

    int bindUDP = bind(socket_udp, (const struct sockaddr *) &severAddressUDP,
                       sizeof(struct sockaddr_in));
    CHECK(bindUDP != -1, "Bind UDP", TRUE);
    int bindTCP = bind(socket_tcp, (const struct sockaddr *) &severAddressTCP,
                       sizeof(struct sockaddr_in));
    CHECK(bindTCP != -1, "Bind TCP", TRUE);


    CHECK(listen(socket_tcp, MAX_BACKLOG) != -1, "-10 : Listen TCP socket",
          TRUE);

    /** golirea multimulor */
    FD_ZERO(&read_fileDescriptors);
    FD_ZERO(&temp_fileDescriptors);
    /** adaugarea file descriptorilor pentru TCP, UDP, stdin */
    FD_SET(STDIN_FILENO, &read_fileDescriptors);
    FD_SET(socket_udp, &read_fileDescriptors);
    FD_SET(socket_tcp, &read_fileDescriptors);
    fdmax = max(socket_tcp, socket_udp);

    while (TRUE) {
        temp_fileDescriptors = read_fileDescriptors;
        CHECK(select(fdmax + 1, &temp_fileDescriptors, NULL, NULL, NULL) !=
              -1, "-10 : Select Error", TRUE);

        for (i = 0; i <= fdmax; i++) {

            memset(buffer, 0, BUFFER_LEN);
            if (FD_ISSET(socket_tcp, &temp_fileDescriptors) &&
                i == socket_tcp) {
                /** daca am primit o nou cerere pe parte de TCP si aduaga un nou
                 * clinet*/
                int client_socket;
                int clilen = sizeof(clientAdrress);
                client_socket = accept(socket_tcp,
                                       (struct sockaddr *) &clientAdrress,
                                       (socklen_t *) &clilen);
                CHECK(client_socket != -1, "-10 : Accept Error", FALSE);

                /** adaugare efectiva a clientului */
                if (client_socket != -1) {
                    FD_SET(client_socket, &read_fileDescriptors);
                    fdmax = max(fdmax, client_socket);
                }
                printf("Noua conexiune de la %s, port %d, socket_client %d\n",
                       inet_ntoa(clientAdrress.sin_addr),
                       ntohs(clientAdrress.sin_port),
                       client_socket);

            } else if (FD_ISSET(socket_udp, &temp_fileDescriptors) &&
                       i == socket_udp) {

                /** daca am primit ceva pe conexiunea UDP  pt unlock*/
                socklen_t socklen;
                int rez;
                char command[COMMAND_LEN];
                ssize_t read_len = recvfrom(socket_udp, buffer, BUFFER_LEN, 0,
                                            (struct sockaddr *) &clientAdrress,
                                            &socklen);

                CHECK(read_len != -1, "Error from Read Data On UDP", TRUE);
                memcpy(command, buffer, COMMAND_LEN);

                printf("Noua conexiune UDP de la %s, port %d\n",
                       inet_ntoa(clientAdrress.sin_addr),
                       ntohs(clientAdrress.sin_port));

                ssize_t send;
                if (strcmp(command, "unlock") != 0) {

                    char numberCard[LEN_NUMBER_CARD] = {0};
                    char secret_password[MAX_PASSWORD] = {0};

                    memcpy(numberCard, buffer, LEN_NUMBER_CARD);
                    memcpy(secret_password, buffer + 10, MAX_PASSWORD);
                    rez = unlock(database, numberCard, secret_password);
                    printf("Deblocare card %s cu parola secreta %s\n",
                           numberCard, secret_password);

                } else {
                    char numberCard[LEN_NUMBER_CARD];
                    int id;

                    memcpy(numberCard, buffer + 10, LEN_NUMBER_CARD);
                    memcpy(&id, buffer + 20, sizeof(int));

                    rez = check_unlock(database, numberCard, id);
                    memset(buffer, 0, BUFFER_LEN);
                    memcpy(buffer, &rez, sizeof(int));
                    printf("Cerere de deblocare pentru card %s cu rezultat%d\n",
                           numberCard, rez);
                }

                memset(buffer, 0, BUFFER_LEN);
                memcpy(buffer, &rez, sizeof(int));
                send = sendto(socket_udp, buffer, BUFFER_LEN, 0,
                              (const struct sockaddr *) &clientAdrress,
                              socklen);
                CHECK(send != -1, "-10 : sendto UDP", TRUE);

            } else if (FD_ISSET(STDIN_FILENO, &temp_fileDescriptors) &&
                       i == STDIN_FILENO) {
                scanf("%100s", buffer);
                if (strcmp(buffer, "quit") == 0) {
                    finish = TRUE;
                    break;
                }

            } else if (FD_ISSET(i, &temp_fileDescriptors)) {
                /** am primit ceva de la unul dintre client */

                memset(buffer, 0, BUFFER_LEN);
                ssize_t read_len = recv(i, buffer, BUFFER_LEN, 0);
                if (read_len <= 0) {
                    if (read_len == 0) {
                        printf("./server: socket %d hung up\n", i);
                    } else {
                        perror("ERROR in recv");
                        exit(EXIT_FAILURE);
                    }

                    close(i);
                    FD_CLR(i, &read_fileDescriptors);
                } else {
                    Bool quit = FALSE;
                    run_command(i, buffer, database, &quit);
                    if (quit == TRUE) {
                        close(i);
                        FD_CLR(i, &read_fileDescriptors);
                    } else {
                        CHECK(send(i, buffer, BUFFER_LEN, 0) != -1,
                              "-10 : Error send TCP", TRUE);
                    }
                }
            }
        }

        if (finish == TRUE)
            break;
    }

    /** trmiterea mesajelor de inchidere */
    for (i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fileDescriptors) && i != socket_tcp &&
            i != socket_udp && i != STDIN_FILENO) {
            memset(buffer, 0, BUFFER_LEN);
            sprintf(buffer, "quit");

            CHECK(send(i, buffer, BUFFER_LEN, 0) != -1,
                  "-10 : Send QUIT SERVER", TRUE);
        }
    }

    /** asteptam ca clienti sa primeasca mesajele de la server*/
    /** este doar o masurea de siguranta */
    sleep(2);

    /** inchiderea comunicatie */
    for (i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fileDescriptors) && i != socket_tcp &&
            i != socket_udp && i != STDIN_FILENO) {
            FD_CLR(i, &read_fileDescriptors);
            close(i);
        }
    }

    FD_CLR(socket_udp, &read_fileDescriptors);
    FD_CLR(socket_tcp, &read_fileDescriptors);
    FD_ZERO(&read_fileDescriptors);
    FD_ZERO(&temp_fileDescriptors);
    close(socket_udp);
    close(socket_tcp);
    return;
}


/**
 * functi crear initializeza database, porneste servarul si elibereza database
 * @param IP ip server
 * @param port port server
 * @param users_data_file fisierul de unde se citest datele
 * @return 0
 */
int start_server(char *IP, char *port, char *users_data_file) {
    Database *database = init_database(users_data_file);

    run_server(IP, port, database);

    free_database(&database);

    return 0;
}
