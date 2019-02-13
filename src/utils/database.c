#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "database.h"

/**
 * Fuctia care aloca memeria pentru database utilizat in program si le
 * citerste/initializeaza
 * @param users_data_file fisierul de unde se citersc datele
 * @return pointerul catre zona de memorie unde a fost alocat
 */
Database *init_database(char *users_data_file) {
    FILE *input = fopen(users_data_file, "r+");
    assert(input != NULL);
    Database *database;
    int i, numberAccounts;

    database = (Database *) malloc(sizeof(Database));
    assert(database != NULL);
    assert(fscanf(input, "%d", &numberAccounts) == 1);
    database->numberAccounts = numberAccounts;
    database->banckAccounts = (BanckAccount *) calloc((size_t) numberAccounts,
                                                      sizeof(BanckAccount));
    assert(database->banckAccounts != NULL);

    for (i = 0; i < database->numberAccounts; i++) {
        memset(&database->banckAccounts[i], 0, sizeof(BanckAccount));
        fscanf(input, "%s", (database->banckAccounts[i].surname));
        fscanf(input, "%s", (database->banckAccounts[i].name));
        fscanf(input, "%s", (database->banckAccounts[i].numberCard));
        fscanf(input, "%s", (database->banckAccounts[i].pin));
        fscanf(input, "%s", (database->banckAccounts[i].secret_password));
        fscanf(input, "%lf", &(database->banckAccounts[i].sold));
    }

    database->socket_client = (int *) malloc(numberAccounts * sizeof(int));
    assert(database->socket_client != NULL);
    for (i = 0; i < database->numberAccounts; i++) {
        database->socket_client[i] = -1;
    }

    database->failed_attempts = (int *) calloc((size_t) numberAccounts,
                                               sizeof(int));
    assert(database->failed_attempts != NULL);

    database->id_unlock = (int *) malloc(numberAccounts * sizeof(int));
    assert(database->id_unlock != NULL);
    for(i = 0; i < database->numberAccounts; i++){
        database->id_unlock[i] = -1;
    }

    database->data_transfers = (Data_Transfer *) calloc((size_t) numberAccounts,
                                                        sizeof(Data_Transfer));
    assert(database->data_transfers != NULL);
    for (i = 0; i < database->numberAccounts; i++) {
        database->data_transfers[i].index_beneficiary = -1;
    }

    fclose(input);
    return database;
}


/**
 * Funtia pentru eliberarea memorie pentru un elemet de tipul Database
 * @param pDatabase adrea unde eliberam memoria
 */
void free_database(Database **pDatabase) {
    free((*pDatabase)->banckAccounts);
    free((*pDatabase)->socket_client);
    free((*pDatabase)->failed_attempts);
    free((*pDatabase)->id_unlock);
    free((*pDatabase)->data_transfers);
    free(*pDatabase);
    *pDatabase = NULL;
}

/**
 * Cauta daca exista repectivul card
 * @param database baza de data unde cautam
 * @param numberCard numarul cardului pe care il cautam
 * @return daca a gasit indicele altfel NOT_CONTAINS
 */
int get_index_numberCard(Database *database, const char *numberCard) {
    int i;

    for (i = 0; i < database->numberAccounts; i++) {
        if (strcmp(numberCard, database->banckAccounts[i].numberCard) == 0)
            return i;
    }

    return NOT_CONTAINS; /** in cazul in care nu am gasit nimic */
}

/**
 * Cauta daca exista repectivul client
 * @param database database baza de data unde cautam
 * @param socket socket-ul clientului care il cautam
 * @return daca a gasit indicele altfel NOT_CONTAINS
 */
int get_index_socket(Database *database, const int socket) {
    int i;

    for (i = 0; i < database->numberAccounts; i++) {
        if (database->socket_client[i] == socket)
            return i;
    }

    return NOT_CONTAINS;
}
