#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include "gioco.h"


//array delle locazioni presenti nel gioco. Le locazioni non possono essere modificate.
struct locazione locazioni[N_LOCAZIONI] = {
    {"principale", "Sei nell'atrio di ingresso della villa. Davanti a te vedi un ampio ++soggiorno++,alla tua sinistra una porta con su scritto ++cucina++ mentre alla tua destra un lungo corridio.\n"
                    "Lungo il corridoio ci sono due porte. Quella sulla sinistra porta al ++bagno++, quella sulla destra ad un ++ripostiglio++.\n"
                    "Proseguendo fino in fono si arriva allo ++studio++."},

    {"cucina", "Entri nella cucina. E' tutto molto pulito e in ordine.\n"
                "Sull' isola al centro delle stanza c'è un **libro** di ricette "},
    {"studio", "Ti trovi nello studio. Davanti a te c'è una grossa libreria piena di manuali che sembrano molto antichi\n"
                "Al centro della stanza si trova un **pianoforte** di mogano.\n"
                "In disparte si trova invece una **scrivania** con una luce accesa.\n"
                "Vedi chiaramente una **teca** illuminata in disparte. "},
    {"soggiorno", "C'è un ampio soggiorno con un grande divano. Davanti a te c'è una grande **televisore**. Appesi alle pareti vedi dei quadri. Un quadro di un **girasole**, un quadro di un **cavallo** e un quadro di un **paesaggio**"},
    {"bagno", "Sei nel bagno. C'è uno **specchio**  con sotto il lavandino. In fondo trovi il **water**"},
    {"ripostiglio", "Ti trovi in uno spazio un po' angusto. E' tutto molto disordinato. C'è una piccola **cassetta** degli attrezzi chiusa da un lucchetto"}
};

//array degli enigmi presenti nel gioco. Gli enigmi non possono essere modificati.
struct enigma enigmi[N_ENIGMI] = {
    {"Sul libro compare una scritta\n"
      "Se la frase completerai allora la ricetta otterai\n"
      "\"Gira gira il ......., tira su il .........\n"
        "Fuoco fuoco notte e dì\n"
        "Le streghe fan così\"\n" ,"mestolo coperchio"},
    {"Sono il più grande compositore e tutti mi conoscono\n"
      "Ma in pochi sanno cos'è che sta in mezzo al mio nome completo.\n"
      "Vediamo se lo sai tu!", "Amadeus"},
    {"Il famoso numero di gatti che erano in fila\n"
      "A cui viene sommato il numero dei cuccioli bianchi e neri rapiti da una donna crudele\n"
      "Devi moltiplicare per il triplo del numero di nipoti, avventurose giovani marmotte, di quello sfortunato papero.", "1305"},
    {"Se sei arrivato fino a qui puoi allora saprai dirmi l'ingrediente segreto!\n", "La cipolla"}

    }; 



//funzione che permette di creare un nuovo utente loggato
//viene allocata la memoria per la struttura user_on e viene fatto un inserimento in testa
struct user_on* new_user_on(struct user_on** head, int sd) {
    struct user_on* new_user = malloc(sizeof(struct user_on));
    if (new_user == NULL) {
        printf("Errore di allocazione della memoria\n");
        return NULL;
    }
    new_user->sd = sd;
    new_user->next = *head;
    *head = new_user;
    return new_user;
}

//funzione che permette di trovare un utente loggato
//Viene restituito un puntatore all'utente se viene trovato, altrimenti NULL
struct user_on* trova_utente(struct user_on* head, int sd) {
    struct user_on* current = head;
    while (current != NULL) {
        if (current->sd == sd) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//funzione che permette di rimuovere un utente loggato dalla lista
//Si gestisce il caso in cui l'utente da rimuovere è la testa della lista
//altrimenti si cerca l'utente da rimuovere grazie ad un puntatore temporaneo che tiene contiene l'elemento precedente
//Se l'utente non è presente nella lista non viene fatto nulla
//Se l'utente è presente nella lista viene rimosso e la memoria liberata

void rimuovi_utente(struct user_on **head, int sd) {
    struct user_on *temp = *head, *prev;

    if (temp != NULL && temp->sd == sd) {
        *head = temp->next; 
        free(temp); 
        return;
    }

    while (temp != NULL && temp->sd != sd) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) 
        return;

    prev->next = temp->next;

    free(temp); 
}


//funzione che inizializza i campi di una partita
void inizializza_partita(struct partita* partita) {
    partita->token_raccolti = 0;
    partita->fine_partita = time(NULL) + 30*60;
    partita->fine_guess = 0;
    partita->enigma_attivo = false;
    partita->pos_enigma_attivo = -1;
    partita->guess_usata = false;
    partita->guess_attivo = false;

    partita->oggetti[0].nome = "libro";
    partita->oggetti[0].descrizione_bloccato = "Il libro è aperto, ma al suo interno non c'è scritto nulla\n""Forse possiamo rivelare quello che c'è scritto";
    partita->oggetti[0].descrizione_libero = "Ricetta Pasta e Fagioli\n"
                                            "Ingredienti:\n"
                                            "- 300g di fagioli\n"
                                            "- 300g di pasta\n"
                                            "- 2 spicchi d'aglio\n"
                                            "- 400g di pomodori pelati\n"
                                            "-Sale, pepe, olio\n"
                                            "-Non scordare l'ingrediente segreto: LA CIPOLLA\n";
    partita->oggetti[0].bloccato = true;
    partita->oggetti[0].usato = true;
    partita->oggetti[0].pos_enigma = 0;
    partita->oggetti[0].prendibile = false;

    partita->oggetti[1].nome = "pianoforte";
    partita->oggetti[1].descrizione_bloccato = "Sembra un pianoforte molto pregiato, ma sulla tastiera manca un tasto nero.\n"
                                                "Sul leggio c'è uno **spartito**";
    partita->oggetti[1].descrizione_libero = "Sembra un pianoforte molto pregiato\n"
                                              "Sul leggio c'è uno **spartito**";
    partita->oggetti[1].bloccato = true;
    partita->oggetti[1].usato = false;
    partita->oggetti[1].pos_enigma = -1;
    partita->oggetti[1].prendibile = false;

    partita->oggetti[2].nome = "spartito";
    partita->oggetti[2].descrizione_bloccato = "";
    partita->oggetti[2].descrizione_libero = "Sullo spartito si legge:\n"
                                            "\"      Wolfang Amadeus Mozart\n"
                                                "Eine Kleine Nachtmusik, K. 525\n"
                                                "Una piccola musica notturna, k. 525\"\n"
                                            "Che composizione magnifica.";
    partita->oggetti[2].bloccato = false;
    partita->oggetti[2].usato = true;
    partita->oggetti[2].pos_enigma = -1;
    partita->oggetti[2].prendibile = false;

    partita->oggetti[3].nome = "scrivania";
    partita->oggetti[3].descrizione_bloccato = "Una scrivania con oggetti in disordine. Non sembra esserci niente di utile";
    partita->oggetti[3].descrizione_libero = "Una scrivania con oggetti in disordine\n." 
                                              "Sotto la scrivania c'è il cassetto aperto. Hai gia preso il pezzo della chiave";
    partita->oggetti[3].bloccato = true;
    partita->oggetti[3].usato = true;
    partita->oggetti[3].pos_enigma = -1;
    partita->oggetti[3].prendibile = false;

    partita->oggetti[4].nome = "televisore";
    partita->oggetti[4].descrizione_bloccato = "Il televisore è spento, serve il telecomando per accenderla, ma non sembra sia nella stanza";
    partita->oggetti[4].descrizione_libero = "Sul televisore c'è una televendita di cucina\n"
                                              "\nStanno provando a vendere un set di mestoli e coperchi a soli 19,99€\n";
    partita->oggetti[4].bloccato = true;
    partita->oggetti[4].usato = false;
    partita->oggetti[4].pos_enigma = -1;
    partita->oggetti[4].prendibile = false;

    partita->oggetti[5].nome = "girasole";
    partita->oggetti[5].descrizione_bloccato = "";
    partita->oggetti[5].descrizione_libero = "Sembrano i girasoli di Van Ghogh\n"
                                              "Il quadro è molto bello, ma non sembra esserci nulla di utile\n";
    partita->oggetti[5].bloccato = false;
    partita->oggetti[5].usato = true;
    partita->oggetti[5].pos_enigma = -1;
    partita->oggetti[5].prendibile = false;

    partita->oggetti[6].nome = "cavallo";
    partita->oggetti[6].descrizione_bloccato = "";
    partita->oggetti[6].descrizione_libero = "E' raffigurato un maestoso cavallo bianco, deve essere il famoso cavallo bianco di Napoleone\n"
                                              "Il quadro è molto bello, ma non sembra esserci nulla di utile\n";
    partita->oggetti[6].bloccato = false;
    partita->oggetti[6].usato = true;
    partita->oggetti[6].pos_enigma = -1;
    partita->oggetti[6].prendibile = false;

    partita->oggetti[7].nome = "paesaggio";
    partita->oggetti[7].descrizione_bloccato = "";
    partita->oggetti[7].descrizione_libero = "Non sembra un quadro prezioso e nemmeno molto bello\n"
                                              "E'leggermente spostato e intravedi una **cassaforte**";
    partita->oggetti[7].bloccato = false;
    partita->oggetti[7].usato = true;
    partita->oggetti[7].pos_enigma = -1;
    partita->oggetti[7].prendibile = false;

    partita->oggetti[8].nome = "cassaforte";
    partita->oggetti[8].descrizione_bloccato = "La cassaforte è bloccata. Serve una parola d'ordine per sbloccarla";
    partita->oggetti[8].descrizione_libero = "La cassaforte è aperta e vuota. Hai già ottenuto il pezzo di chiave che c'era dentro";
    partita->oggetti[8].bloccato = true;
    partita->oggetti[8].usato = true;
    partita->oggetti[8].pos_enigma = 1;
    partita->oggetti[8].prendibile = false;

    partita->oggetti[9].nome = "specchio";
    partita->oggetti[9].descrizione_bloccato = "";
    partita->oggetti[9].descrizione_libero = "Vedi la tua immagine riflessa";
    partita->oggetti[9].bloccato = false;
    partita->oggetti[9].usato = true;
    partita->oggetti[9].pos_enigma = -1;
    partita->oggetti[9].prendibile = false;

    partita->oggetti[10].nome = "water";
    partita->oggetti[10].descrizione_bloccato = "L'acqua arriva fino all'orlo a causa dello sciacquone rotto.\n"
                                                "Forse serve qualcosa per ripararlo";
    partita->oggetti[10].descrizione_libero = "L'acqua se ne è andata. Vedi sul fondo un **telecomando**, chissà se funziona sempre";
    partita->oggetti[10].bloccato = true;
    partita->oggetti[10].usato = true;
    partita->oggetti[10].pos_enigma = -1;
    partita->oggetti[10].prendibile = false;

    partita->oggetti[11].nome = "cassetta";
    partita->oggetti[11].descrizione_bloccato = "La cassetta è chiusa da un lucchetto. Serve una chiave per aprirla";
    partita->oggetti[11].descrizione_libero = "Dentro trovi una **chiave_inglese** insieme ad altri attrezzi";
    partita->oggetti[11].bloccato = true;
    partita->oggetti[11].usato = true;
    partita->oggetti[11].pos_enigma = 2;
    partita->oggetti[11].prendibile = false;

    partita->oggetti[12].nome = "teca";
    partita->oggetti[12].descrizione_bloccato = "La teca ha i vetri oscurati. Non riesci a vedere dentro. C'è un tastierino che potrebbe sbloccarla.";
    partita->oggetti[12].descrizione_libero = "Nella teca si trova un **tasto** nero.";
    partita->oggetti[12].bloccato = true;
    partita->oggetti[12].usato = true;
    partita->oggetti[12].pos_enigma = 3;
    partita->oggetti[12].prendibile = false;

    partita->oggetti[13].nome = "telecomando";
    partita->oggetti[13].descrizione_bloccato = "";
    partita->oggetti[13].descrizione_libero = "Un telecomando. Forse serve per accendere il televisore\n";
    partita->oggetti[13].bloccato = true;
    partita->oggetti[13].usato = false;
    partita->oggetti[13].pos_enigma = -1;
    partita->oggetti[13].prendibile = false;

    partita->oggetti[14].nome = "chiave_inglese";
    partita->oggetti[14].descrizione_bloccato = "";
    partita->oggetti[14].descrizione_libero = "Una chiave inglese. Potrebbe servire per aggiustare qualcosa\n";
    partita->oggetti[14].bloccato = true;
    partita->oggetti[14].usato = false;
    partita->oggetti[14].pos_enigma = -1;
    partita->oggetti[14].prendibile = false;

    partita->oggetti[15].nome = "tasto";
    partita->oggetti[15].descrizione_bloccato = "";
    partita->oggetti[15].descrizione_libero = "Un tasto nero. Potrebbe essere il pezzo mancante\n";
    partita->oggetti[15].bloccato = true;
    partita->oggetti[15].usato = false;
    partita->oggetti[15].pos_enigma = -1;
    partita->oggetti[15].prendibile = false;

    partita->oggetti_raccolti[0] = NULL;
    partita->oggetti_raccolti[1] = NULL;
    partita->oggetti_raccolti[2] = NULL;

}

//funzione che permette di creare una nuova partita
//viene allocata la memoria per la struttura partita e viene fatto un inserimento in testa chiamando anche la funzione inizializza_partita per inizalizzare gli altri campi
struct partita* new_partita(struct partita** head, int sd) {
    struct partita* new_partita = malloc(sizeof(struct partita));
    if (new_partita == NULL) {
        printf("Errore di allocazione della memoria\n");
        return NULL;
    }
    new_partita->sd = sd;
    inizializza_partita(new_partita);
    new_partita->next = *head;
    *head = new_partita;
    return new_partita;
}

//funzione che permette di trovare una partita
//Viene restituito un puntatore alla partita se viene trovata, altrimenti NULL
struct partita* trova_partita(struct partita* head, int sd) {
    struct partita* current = head;
    while (current != NULL) {
        if (current->sd == sd) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//funzione che permette di rimuovere una partita dalla lista
//Si gestisce il caso in cui la partita da rimuovere è la testa della lista
//altrimenti si cerca la partita da rimuovere grazie ad un puntatore temporaneo che tiene contiene l'elemento precedente
//Se la partita non è presente nella lista non viene fatto nulla
//Se la partita è presente nella lista viene rimossa e la memoria liberata
void rimuovi_partita(struct partita **head, int sd) {
    struct partita *temp = *head, *prev;

    if (temp != NULL && temp->sd == sd) {
        *head = temp->next; 
        free(temp); 
        return;
    }

    while (temp != NULL && temp->sd != sd) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) 
        return;

    prev->next = temp->next;

    free(temp);  
}


//funzione che permette di trovare una locazione all'intero dell'array locazioni e restituirne la descrizione
char* trova_locazione(char* nome) {
    int i;
    for ( i = 0; i < N_LOCAZIONI; i++) {
        if (strcmp(locazioni[i].nome, nome) == 0) {
            return locazioni[i].descrizione;
        }
    }
    return NULL;
}


//funzione che permette di trovare un oggetto all'interno della partita
//Viene restituito un puntatore all'oggetto se viene trovato, altrimenti NULL
struct oggetto* trova_oggetto(struct partita* head,int sd, char* nome) {
    int i;
    struct partita* partita = trova_partita(head, sd);
    for (i = 0; i < N_OGGETTI; i++) {
        if (strcmp(partita->oggetti[i].nome, nome) == 0) 
            return &partita->oggetti[i];   
    }
    return NULL;
}

//funzione che permette di trovare un enigma all'interno dell'array enigmi
//Non viene effettuato un controllo sull'indice passato come parametro perchè l'indice è inizializzato correttamente nella funzione inizializza_partita
struct enigma* trova_enigma(int pos) {
    return &enigmi[pos];
}


//funzione che permette di restituire lo stato della partita
//Viene restituita una stringa con il numero di token raccolti e il tempo rimasto
char* stato_partita(struct partita* partita) {
    static char stato[1024];
    static time_t tempo_rimasto;
    tempo_rimasto = partita->fine_partita - time(NULL);

    int minuti = tempo_rimasto / 60;
    int secondi = tempo_rimasto % 60;

    sprintf(stato, "\n\nHai raccolto %d/2 token\nTi rimangono ancora %d minuti e %d secondi\n\n", partita->token_raccolti, minuti, secondi);
    return stato;
}

//funzione che permette di verificare se un oggetto è stato raccolto
bool oggetto_raccolto(struct partita* partita, char* nome) {
    int i;
    for (i = 0; i < MAX_OGGETTI_RACCOLTI; i++) {
        if (partita->oggetti_raccolti[i] != NULL && strcmp(partita->oggetti_raccolti[i], nome) == 0) {
            return true;
        }
    }
    return false;
}



