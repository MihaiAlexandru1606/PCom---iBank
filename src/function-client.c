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
#include <assert.h>
#include "utils/define.h"
#include "function-client.h"

/**
 * funtia care transmite servarlui login-ul astfel:
 * in primi 10 bytes se afla comnada login
 * in urmatori : numaurul cardului
 * in urmatori : pin-ul
 * in urmatori : numberFailedLog
 * daca nu suntem logati daca suntem logati printeza mesajul de eroare
 * primeste raspunsul de la server si afisseaza mesajul corespunzator
 * daca s-a reusit login-ul cu succes marcheza isLogged TRUE
 * @param socket_tcp pentru primiterea datelor
 * @param logging starea clientului
 * @param output fiserul de iesire
 * @param numberFailedLog numarul de incercari esuate
 */
static void login(int socket_tcp, Logging *logging, FILE *output,
                  int *numberFailedLog) {
    char display[BUFFER_LEN];
    char buffer[BUFFER_LEN], numberCard[LEN_NUMBER_CARD], pin[LEN_PIN];
    scanf("%6s %4s", numberCard, pin);

    if (logging->isLogged == TRUE) {
        sprintf(display, " -2 : Sesiune deja deschisa");
    } else {

        /** daca se incerca logare pe un alt card */
        if (strcmp(numberCard, logging->currentNumberCard) != 0) {
            (*numberFailedLog) = 0;
        }

        /** daca nu este logat */
        memcpy(logging->currentNumberCard, numberCard, LEN_NUMBER_CARD);
        memset(buffer, 0, BUFFER_LEN);
        sprintf(buffer, "login");
        memcpy(buffer + 10, numberCard, LEN_NUMBER_CARD);
        memcpy(buffer + 20, pin, LEN_PIN);
        memcpy(buffer + 30, numberFailedLog, sizeof(int));
        CHECK(send(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error send login", TRUE);
        CHECK(recv(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error recv login", TRUE);

        int rez;
        memcpy(&rez, buffer, sizeof(int));

        if (rez == IS_LOGGED) {
            sprintf(display, " -2 : Sesiune deja deschisa");
        } else if (rez == WRONG_NUMBER_CARD) {
            sprintf(display, " -4 : Numar card inexistent");
        } else if (rez == WRONG_PIN) {
            sprintf(display, " -3 : Pin gresit");
            (*numberFailedLog)++;
        } else if (rez == CARD_LOCKED) {
            sprintf(display, " -5 : Card blocat");
            (*numberFailedLog) = MAX_FAILED_LOGGING;
        } else if (rez >= 0) {
            char surname[MAX_SURNAME], name[MAX_NAME];
            memcpy(surname, buffer + 10, MAX_NAME);
            memcpy(name, buffer + 30, MAX_NAME);
            sprintf(display, " Welcome %s %s", surname, name);
            logging->isLogged = TRUE;
        } else {
            fprintf(stderr, " -10 : Intern error\n");
        }
    }

    fprintf(output, "login %s %s\n", numberCard, pin);
    fprintf(output, "IBANK>%s\n", display);
    printf("IBANK> %s\n\n", display);
}

/**
 * functia care realizeza delogare, transmite mesajul si primiteza rezulatul
 * @param socket_tcp pentru primiterea datelor
 * @param logging  starea clientului
 * @param output fiserul de iesire
 */
static void logout(int socket_tcp, Logging *logging, FILE *output) {
    char display[BUFFER_LEN];
    char buffer[BUFFER_LEN];

    if (logging->isLogged == FALSE) {
        sprintf(display, " -1 : Clientul nu este autentificat");
    } else {
        memset(buffer, 0, BUFFER_LEN);
        sprintf(buffer, "logout");
        CHECK(send(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error send logout", TRUE);
        CHECK(recv(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error recv logout", TRUE);

        int rez;
        memcpy(&rez, buffer, sizeof(int));
        if (rez >= 0) {
            sprintf(display, " Cleintul a fost deconectat");
            logging->isLogged = FALSE;
        } else {
            fprintf(stderr, " -10 : Intern error\n");
        }
    }

    fprintf(output, "logout\n");
    fprintf(output, "IBANK>%s\n", display);
    printf("IBANK> %s\n\n", display);
}

/**
 * functia care realizeza listsold, transmite mesajul si primiteza rezulatul
 * @param socket_tcp pentru primiterea datelor
 * @param logging  starea clientului
 * @param output fiserul de iesire
 **/
static void listsold(int socket_tcp, Logging logging, FILE *output) {
    char display[BUFFER_LEN];
    char buffer[BUFFER_LEN];

    if (logging.isLogged == FALSE) {
        sprintf(display, " -1 : Clientul nu este autentificat");
    } else {
        memset(buffer, 0, BUFFER_LEN);
        sprintf(buffer, "listsold");
        CHECK(send(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error send listsold", TRUE);
        CHECK(recv(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error recv listsold", TRUE);

        double rez;
        memcpy(&rez, buffer, sizeof(double));
        if (rez >= 0) {
            sprintf(display, " %.2lf", rez);
        } else {
            fprintf(stderr, " -10 : Intern error\n");
        }
    }

    fprintf(output, "listsold\n");
    fprintf(output, "IBANK>%s\n", display);
    printf("IBANK> %s\n\n", display);
}

/**
 * functia transite  servarului comnada transfer astfel:
 * in primi 10 bytes : "trasfer"
 * in urmatori : numarul cardului beneficiarului
 * in urmatori : suma de trasferat
 * daca raspunsul este pozitiv de la server transmite raspunsul utilizatorui
 * @param socket_tcp pentru primiterea datelor
 * @param logging  starea clientului
 * @param output fiserul de iesire
 */
void transfer(int socket_tcp, Logging logging, FILE *output) {
    char beneficiary[LEN_NUMBER_CARD];
    double sum;
    char buffer[BUFFER_LEN];
    scanf("%100s", beneficiary);
    scanf("%lf", &sum);
    fprintf(output, "tranfer %s %.2lf\n", beneficiary, sum);

    if (logging.isLogged == FALSE) {
        fprintf(output, "IBANK> -1 : Clientul nu este autentificat\n");
        printf("IBANK>  -1 : Clientul nu este autentificat\n\n");

    } else {
        memset(buffer, 0, BUFFER_LEN);
        sprintf(buffer, "transfer");
        memcpy(buffer + 10, beneficiary, LEN_NUMBER_CARD);
        memcpy(buffer + 20, &sum, sizeof(double));
        CHECK(send(socket_tcp, buffer, BUFFER_LEN, 0) != -1,
              "-10 : Error send transfer", TRUE);
        CHECK(recv(socket_tcp, buffer, BUFFER_LEN, 0),
              "-10 : Error recv transfer", TRUE);

        int rez;
        memcpy(&rez, buffer, sizeof(int));
        if (rez == WRONG_NUMBER_CARD) {
            fprintf(output, "IBANK> -4 : Numar card inexistent\n");
            printf("IBANK> -4 : Numar card inexistent\n\n");

        } else if (rez == NO_MONEY) {
            fprintf(output, "IBANK> -8 : Fonduri insuficiente\n");
            printf("IBANK> -8 : Fonduri insuficiente\n\n");

        } else if (rez >= 0) {
            char surname[MAX_SURNAME], name[MAX_NAME];
            memcpy(surname, buffer + 10, MAX_NAME);
            memcpy(name, buffer + 30, MAX_NAME);
            fprintf(output, "IBANK> Transfer %.2lf catre %s %s ? [y/n]\n", sum,
                    surname, name);
            printf("IBANK> Transfer %.2lf catre %s %s ? [y/n]\n\n", sum,
                   surname, name);
            scanf("%100s", buffer); /** answer */
            assert(send(socket_tcp, buffer, BUFFER_LEN, 0) != -1);
            assert(recv(socket_tcp, buffer, BUFFER_LEN, 0) != -1);
            memcpy(&rez, buffer, sizeof(int));

            if (rez == CANCELED_OPERATION) {
                fprintf(output, "IBANK> -9 : Operatie anulata\n");
                printf("IBANK> -9 : Operatie anulata\n\n");
            } else {
                fprintf(output, "IBANK> Transfer realizat cu succees\n");
                printf("IBANK> Transfer realizat cu succees\n\n");
            }
        }
    }
}

/**
 * Funtia care realizeza unlock
 * pentru cerea de unlock trimite astfe in buffer:
 * primi 10 "unlock"
 * apoi unmatori numarul cardului
 * apoit id-ul procesului curent
 * Daca este afirmativ tramite catre server astfel:
 * primi 10 numarul cardului
 * apoi paroloa secreta
 * @param socket_udp  pentru primiterea datelor
 * @param logging  starea clientului
 * @param output fiserul de iesire
 * @param IP_sever ip-ul serverului
 * @param port port-ul server-ului
 */
void unlock(int socket_udp, Logging logging, FILE *output,
            const char *IP_sever, const char *port) {
    int portNumber = atoi(port);
    int id = getpid();
    struct sockaddr_in severAddress;
    char buffer[200] = {0};
    char display[BUFFER_LEN] = {0};
    int server_addr_len = sizeof(struct sockaddr_in);
    int rez;


    /** initializarea datelor despre server */
    severAddress.sin_family = AF_INET;
    severAddress.sin_port = (in_port_t) portNumber;
    CHECK(inet_aton(IP_sever, &severAddress.sin_addr) > 0,
          "-10 : IP Sever Address", TRUE);

    memset(buffer, 0, 200);
    sprintf(buffer, "unlock");
    memcpy(buffer + 10, logging.currentNumberCard, LEN_NUMBER_CARD);
    memcpy(buffer + 20, &id, sizeof(int));

    ssize_t send;
    send = sendto(socket_udp, buffer, BUFFER_LEN, 0,
                  (const struct sockaddr *) &severAddress,
                  (socklen_t) server_addr_len);
    assert(send != -1);

    memset(buffer, 0, BUFFER_LEN);
    ssize_t read;

    read = recvfrom(socket_udp, buffer, BUFFER_LEN, 0,
                    (struct sockaddr *) &severAddress,
                    (socklen_t *) &server_addr_len);
    CHECK(read != -1, "-10 : Recvfrom", TRUE);

    memcpy(&rez, buffer, sizeof(int));
    fprintf(output, "unlock\n");
    if (rez == WRONG_NUMBER_CARD) {
        sprintf(display, " -4 : Numar card inexistent.");
    } else if (rez == FAILED_OPERATION) {
        sprintf(display, " -6 : Operatie esuata.");
    } else if (rez == FAILED_UNLOCKING) {
        sprintf(display, " -7 : Deblocare esuata.");
    } else {
        char secret_password[MAX_PASSWORD];
        memset(secret_password, 0, MAX_PASSWORD);
        fprintf(output, "UNLOCK> Trimite parola secreta\n");
        printf("UNLOCK> Trimite parola secreta\n\n");

        scanf("%8s", secret_password);
        fprintf(output, "%s\n", secret_password);
        memset(buffer, 0, BUFFER_LEN);
        memcpy(buffer, logging.currentNumberCard, LEN_NUMBER_CARD);
        memcpy(buffer + 10, secret_password, MAX_PASSWORD);

        send = sendto(socket_udp, buffer, BUFFER_LEN, 0,
                      (const struct sockaddr *) &severAddress,
                      (socklen_t) server_addr_len);
        assert(send != -1);

        read = recvfrom(socket_udp, buffer, BUFFER_LEN, 0,
                        (struct sockaddr *) &severAddress,
                        (socklen_t *) &server_addr_len);
        CHECK(read != -1, "-10 : Recvfrom", TRUE);

        memcpy(&rez, buffer, sizeof(int));
        if (rez == FAILED_UNLOCKING) {
            fprintf(output, "UNLOCK> -7 : Deblocare esuata\n");
            printf("UNLOCK> -7 : Deblocare esuata\n\n");
        } else {
            fprintf(output, "UNLOCK> Client deblocat\n");
            printf("UNLOCK> Client deblocat\n\n");

        }
        return;
    }

    fprintf(output, "UNLOCK>%s\n", display);
    printf("UNLOCK>%s\n\n", display);
}

/**
 * functia care citeste din la STDIN si trmite respentiva comneta servarului
 * in functie de ce primeste de la server afiseaza mesajul la stdout si il
 * scire in fisier, initializeaza conesiunea cu servarul
 * @param IP_sever ip-ul serverului
 * @param port port-ul server-ului
 */
void start_client(const char *IP_sever, const char *port) {
    int portNumber = atoi(port);
    struct sockaddr_in severAddress;
    char buffer[BUFFER_LEN] = {0};
    Logging logging = (Logging) {FALSE, {0}};
    int id = getpid(), fdmax;
    int numberFailedLog = 0;
    sprintf(buffer, "client-%d.log", id);
    FILE *output = fopen(buffer, "w+");
    assert(output != NULL);
    fd_set read_fileDescriptors;
    fd_set temp_fileDescriptors;


    /** crearea socket-urilor pentru UDP si TCP */
    int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    int socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(socket_udp != -1, "-10 : Open socket UDP", TRUE);
    CHECK(socket_tcp != -1, "-10 : Open socket TCP", TRUE);

    /** initializarea datelor despre server */
    severAddress.sin_family = AF_INET;
    severAddress.sin_port = (in_port_t) portNumber;
    CHECK(inet_aton(IP_sever, &severAddress.sin_addr) > 0,
          "-10 : IP Sever Address", TRUE);

    /** creare conexiuni pentru protocoloul TCP */
    int coneectTCP = connect(socket_tcp,
                             (const struct sockaddr *) &severAddress,
                             sizeof(struct sockaddr_in));
    CHECK(coneectTCP != -1, "-10 : Coneect TCP", TRUE);

    /** golirea multimulor */
    FD_ZERO(&read_fileDescriptors);
    FD_ZERO(&temp_fileDescriptors);
    /** adaugarea file descriptorilor pentru TCP, UDP, stdio */
    FD_SET(STDIN_FILENO, &read_fileDescriptors);
    FD_SET(socket_tcp, &read_fileDescriptors);
    fdmax = socket_tcp;

    while (TRUE) {
        temp_fileDescriptors = read_fileDescriptors;
        CHECK(select(fdmax + 1, &temp_fileDescriptors, NULL, NULL, NULL) !=
              -1, "-10 : Select Error", FALSE);

        if (FD_ISSET(socket_tcp, &temp_fileDescriptors)) {
            assert(recv(socket_tcp, buffer, BUFFER_LEN, 0) != -1);
            if (strcmp(buffer, "quit") == 0) {
                fprintf(output, "IBANK> SERVER IS QUIT\n");
                printf("IBANK> SERVER IS QUIT\n");
            } else {
                fprintf(output, "IBANK> Intern error\n");
                printf("IBANK> Intern error\n");
            }
            break;
        } else if (FD_ISSET(STDIN_FILENO, &temp_fileDescriptors)) {
            memset(buffer, 0, BUFFER_LEN);
            scanf("%s", buffer);

            if (strcmp(buffer, "login") == 0) {
                login(socket_tcp, &logging, output, &numberFailedLog);
            } else if (strcmp(buffer, "logout") == 0) {
                logout(socket_tcp, &logging, output);
                numberFailedLog = 0;
            } else if (strcmp(buffer, "listsold") == 0) {
                listsold(socket_tcp, logging, output);
                numberFailedLog = 0;
            } else if (strcmp(buffer, "transfer") == 0) {
                transfer(socket_tcp, logging, output);
                numberFailedLog = 0;
            } else if (strcmp(buffer, "quit") == 0) {
                send(socket_tcp, buffer, BUFFER_LEN, 0);
                fprintf(output, "quit\n");
                break;
            } else if (strcmp(buffer, "unlock") == 0) {
                unlock(socket_udp, logging, output, IP_sever, port);
                numberFailedLog = 0;
            }
        }
    }

    FD_CLR(STDIN_FILENO, &read_fileDescriptors);
    FD_CLR(socket_tcp, &read_fileDescriptors);
    FD_ZERO(&read_fileDescriptors);
    FD_ZERO(&temp_fileDescriptors);
    close(socket_tcp);
    close(socket_udp);
    fclose(output);
}
