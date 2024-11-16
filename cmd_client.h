#ifndef CMD_CLIENT_H
#define CMD_CLIENT_H
#include <stdbool.h>
#include "gioco.h"

char* gestore_register(char* ,char* ,char*  );
char* gestore_login(int ,char* ,char* ,char* );
char* gestore_start_room(int ,char* ,char* );
char* gestore_look(int ,char* ,char* );
char* gestore_take(int ,char* ,char* );
char* gestore_use(int ,char* ,char* ,char*);
char* gestore_objs(int ,char* );
char* attiva_guess(int ,int ,char* );
char* gestore_guess(int ,int ,char* );
char* gestore_end(int ,char* );
bool enigma_attivo(int );
bool guess_attivo(int );
char* gestore_risposta(int ,char* );
bool tempo_esaurito(int );
char* gestore_fine_tempo(int );
#endif