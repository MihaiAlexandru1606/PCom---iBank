#include "define.h"

#ifndef BANK_ACCOUNT_H
#define BANK_ACCOUNT_H

#pragma pack(1)

/**
 * strunctura pentru contul bancar
 * surname          -> nume
 * name             -> prenume
 * numberCard       -> numarul cardului
 * pin              -> pin
 * secret_password  -> parola secreta
 * sold             -> sold-ul disponibil
 */
typedef struct {
    char surname[MAX_SURNAME];
    char name[MAX_NAME];
    char numberCard[LEN_NUMBER_CARD];
    char pin[LEN_PIN];
    char secret_password[MAX_PASSWORD];
    double sold;
} BanckAccount;

/**
 * strcutura care memoreaza informatile pentru un transfer
 * index_beneficiary    ->  index-ul contului carulia dorim sa tarnsferm
 *                          indicele din vectorului banckAccounts din Database
 * sum                  ->  suma pe care vrem sa o transferam
 */
typedef struct {
    int index_beneficiary;
    double sum;
}Data_Transfer;

/**
 * structura pentru memeorarea informatilor utile despre un cont:
 * banckAccounts    -> informatile despreconturile bancare citite din fisier
 * socket_client    -> socket - ul clientui care reuserte sa logeze cu succes
 *                      pentru respectivul cont
 * failed_attempts  -> numarul de incercari nereusite, el indica daca un cont
 *                     este blocat sau nu
 * id_unlock        -> id-ul clientului care incearca sa debloche contul
 * data_transfers   -> datele privind transfer-ul in curs de desfesurare
 *                      se foloste acest vector pentru a nu bloca servarul pana
 *                      primim raspuns de la client
 * numberAccounts   -> numarul total de conturi bancare
 *
 */
typedef struct {
    BanckAccount *banckAccounts;
    int *socket_client;
    int *failed_attempts;
    int *id_unlock;
    Data_Transfer *data_transfers;
    int numberAccounts;
}Database;

#pragma pack()

Database *init_database(char *users_data_file);

void free_database(Database **pDatabase);

int get_index_numberCard(Database *database, const char *numberCard);

int get_index_socket(Database *database, const int socket);

#endif
