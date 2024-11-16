#ifndef GIOCO_H
#define GIOCO_H

#include <stdbool.h>
#include <time.h>

#define N_OGGETTI 16
#define N_ENIGMI 4
#define N_TOKEN 2
#define MAX_OGGETTI_RACCOLTI 3
#define N_LOCAZIONI 6

//in questa struttura verranno memorizzati solo gli utenti che hanno effettuato il login
//viene memorizzato il descrittore del socket e il puntatore al prossimo utente
struct user_on { 
    int sd;
    struct user_on* next;
};

//struttura che rappresenta un oggetto
struct oggetto {
    char* nome; //nome dell'oggetto
    char* descrizione_bloccato; //descrizione dell'oggetto se bloccato
    char* descrizione_libero;   //descrizione dell'oggetto se libero
    bool bloccato; //impostato a true se l'oggetto è bloccato
    bool usato; //viene settato a true quando l'oggetto viene usato o se l'oggetto non può essere usato 
    bool prendibile; //impostato a true se l'oggetto può essere raccolto
    int pos_enigma; //sa all'oggetto è associato a un enigma contiene la posizione dell'enigma associato, altrimenti -1
};

//struttura che rappresenta una locazione
//viene memorizzato il nome e la descrizione della locazione
struct locazione {
    char* nome;
    char* descrizione;
};

//struttura che rappresenta un enigma
//viene memorizzata la domanda(l'enigma in se) e la risposta
struct enigma {
    char* domanda;
    char* risposta;
};

//struttura che rappresenta una partita
struct partita{
    int sd; //descrittore del socket
    int token_raccolti;//numero di token raccolti
    struct oggetto oggetti[N_OGGETTI];//array degli oggetti presenti nella partita.
    char* oggetti_raccolti[MAX_OGGETTI_RACCOLTI];//array degli oggetti raccolti
    bool enigma_attivo;//impostato a true se c'è un enigma attivo
    int pos_enigma_attivo;//posizione dell'enigma attivo
    bool guess_usata;//impostato a true se la funzione speciale è stata usata
    bool guess_attivo;//impostato a true se c'è la funzionalità speciale è attiva
    time_t fine_partita;//tempo di fine partita
    time_t fine_guess;//tempo di fine della funzione speciale
    struct partita* next;//puntatore alla prossima partita
};

struct user_on* new_user_on(struct user_on** , int );
struct user_on* trova_utente(struct user_on* , int ) ;
void rimuovi_utente(struct user_on** , int ) ;
struct partita* new_partita(struct partita**, int ) ;
void rimuovi_partita(struct partita**, int ) ;
struct partita* trova_partita(struct partita*, int ) ;
char* trova_locazione(char* ) ;
struct oggetto* trova_oggetto(struct partita*,int , char* ) ;
struct enigma* trova_enigma(int ) ;
char* stato_partita(struct partita* ) ;
bool oggetto_raccolto(struct partita* , char* ) ;

#endif
