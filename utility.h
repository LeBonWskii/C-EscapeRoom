#ifndef UTILITY_H
#define UTILITY_H




#include "cmd_client.h"

void mostra_comandi_server();
int ricevi(int, char*);
int invia(int, char*);
void gestisci_comandi_client(int ,int , char*);
char* lista_comandi();

#endif