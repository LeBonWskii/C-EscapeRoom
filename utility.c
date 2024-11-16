
#include "utility.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



void mostra_comandi_server(){
    printf("\n***************************** SERVER STARTED *********************************\n\nDigita un comando:\n1) start --> avvia il server di gioco\n2) stop --> termina il server\n\n******************************************************************************\n");
}

char* lista_comandi(){
    return "Comandi disponibili:\n1) register <username> <password> --> per registrare un nuovo utente\n2) login <username> <password> --> per effettuare il login\n3) start <NomeStanza> --> per avviare l'escape room\n4) look <stanza | oggetto> --> per visualizzare la descrizione di una stanza o un oggetto\n5) take <oggetto> --> per raccogliere un oggetto o avviare un enigma\n6) use <oggetto1> [oggetto2] --> per utilizzare un oggetto\n7) objs --> per visualizzare gli oggetti raccolti\n8) end--> per terminare la partita\n9) guess--> Contatta l'aiutante e attiva la funzione speciale\n";
}

int ricevi(int sd, char* buf){
    //funzione che permette la recezione di un messagio. 
    //Si ha una doppia recv nella quale prima si riceve la lunghezza del messaggio e poi il messaggio stesso
    int ret, len; 
    uint16_t lmsg; 


    ret=recv(sd, (void*) &lmsg, sizeof(uint16_t), 0); //ricevo la lunghezza del messaggio
    if(ret<0){ //se ret è <0 allora c'è sato un errore nella recv
        perror("Errore nella ricezione della lunghezza del messaggio");
        return ret;
    }
    if(ret==0) //se ret è 0 allora c'è stata la chiusura della connessione
        return ret;

    len=ntohs(lmsg); //converto la lunghezza del messaggio in formato host

    ret=recv(sd, (void*) buf, len, 0); //ricevo il messaggio
    if(ret<0) //se ret è <0 allora c'è sato un errore nella recv
        perror("Errore nella ricezione del messaggio");

    return ret;
}

int invia(int sd, char* buf){
    //funzione che permette l'invio di un messagio. 
    //Si ha una doppia send nella quale prima si invia la lunghezza del messaggio e poi il messaggio stesso
    int ret;
    int len = strlen(buf)+1; //calcolo la lunghezza del messaggio
    uint16_t lmsg = htons(len); //converto la lunghezza del messaggio

    ret=send(sd, (void*) &lmsg, sizeof(uint16_t), 0); //invio la lunghezza del messaggio

    if(ret<0){ //se ret è <0 allora c'è sato un errore nella send
        perror("Errore nell'invio della lunghezza del messaggio");
        return ret;
    }

    if(ret==0) //se ret è 0 allora c'è stata la chiusura della connessione
        return ret;
    
    ret=send(sd, (void*) buf, len, 0); //invio il messaggio

    if(ret<0) //se ret è <0 allora c'è sato un errore nella send
        perror("Errore nell'invio del messaggio");
    
    return ret;
}

void gestisci_comandi_client(int giocatore_sd, int aiutante_sd, char* buf){
    //funzione che permette di gestire i comandi ricevuti dal client.
    
    
    

    char* comando=NULL;
    char* attr1=NULL;
    char* attr2=NULL;
    char* attr3=NULL;
    //Viene controllato se il giocatore ha esaurito il tempo perchè se così fosse andrebbe terminata la partita
    if(tempo_esaurito(giocatore_sd)){
        invia(giocatore_sd,gestore_fine_tempo(giocatore_sd));
        return;
    }

    //il client non può semplicemente premere invio
    if(buf[0]=='\0'){
        invia(giocatore_sd,"Devi scrivere qualcosa");
        return;
    }


    // se c'è un enigma attivo quello che riceviamo non è un comando ma la risposta all'enigma
    if(enigma_attivo(giocatore_sd)){
        invia(giocatore_sd,gestore_risposta(giocatore_sd,buf));
        return;
    }
    

    comando=strtok(buf," "); //estraggo il comando dato che potrei avere un comando con un parametro

    //se la funzione speciale è attiva allora quella che riceviamo è la parola scritta dal client
    if(guess_attivo(giocatore_sd)){
        invia(giocatore_sd,gestore_guess(giocatore_sd,aiutante_sd,comando));
        return;
    }


    attr1=strtok(NULL," "); //estraggo il primo parametro
    attr2=strtok(NULL," "); //estraggo il secondo parametro
    attr3=strtok(NULL," "); //estraggo il terzo parametro

    //le funzioni a seconda della loro implementazione prendono un numero di parametri diversi
    //Ad esempio per le funzioni che non richiedono parametri atttr1 rappresenta l'errore nell'aver immesso troppi parametri
    //lo stesso viene fatto per attr2 con i comandi a singolo parametro e attr3 per quelli a doppio parametro

    

    
    if(strcmp(comando,"register")==0)
        invia(giocatore_sd,gestore_register(attr1,attr2,attr3));

    else if(strcmp(comando,"login")==0)
        invia(giocatore_sd,gestore_login(giocatore_sd,attr1,attr2,attr3));

    else if(strcmp(comando,"start")==0)
        invia(giocatore_sd,gestore_start_room(giocatore_sd,attr1,attr2));

    else if(strcmp(comando,"look")==0)
        invia(giocatore_sd,gestore_look(giocatore_sd,attr1,attr2));

    else if(strcmp(comando,"take")==0)
        invia(giocatore_sd,gestore_take(giocatore_sd,attr1,attr2));

    else if(strcmp(comando,"use")==0)
        invia(giocatore_sd,gestore_use(giocatore_sd,attr1,attr2,attr3));

    else if(strcmp(comando,"objs")==0)
        invia(giocatore_sd,gestore_objs(giocatore_sd,attr1));

    else if(strcmp(comando,"end")==0)
        invia(giocatore_sd,gestore_end(giocatore_sd,attr1));
    else if(strcmp(comando,"guess")==0)
        invia(giocatore_sd,attiva_guess(giocatore_sd,aiutante_sd,attr1));
    else{
        strcpy(buf,"Comando non riconosciuto\n");
        invia(giocatore_sd,strcat(buf,lista_comandi()));
    }
}