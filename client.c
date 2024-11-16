#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <stdbool.h>
#include "utility.h"


int main(int argc, char *argv[]) {
    int ret, i, client_sck, porta;
    fd_set master, read_fds;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char buffer_guess[1024];
    char* token1=NULL;
    char* token2=NULL;
    bool guess_attiva=false;

    porta=htons(4242);

    client_sck=socket(AF_INET, SOCK_STREAM, 0); // Creazione del socket del client
    if(client_sck<0){
        perror("Errore nella creazione del socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=porta;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    ret=connect(client_sck, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret<0){
        perror("Errore nella connect");
        exit(1);
    }

    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "giocatore");
    ret=invia(client_sck, buffer);//il client giocatore deve far capire al server che è un giocatore
    if(ret<0)
        exit(1);
    memset(buffer, 0, sizeof(buffer));
    ret=ricevi(client_sck, buffer);//quindi il client giocatore riceverà un messaggio di benvenuto ed i comandi disponibili
    if(ret<0)
        exit(1);
    printf("%s\n", buffer);
    printf("____________________________________________________________________________\n\n");
    memset(buffer, 0, sizeof(buffer));

    strcpy(buffer, "ricevuti");
    ret=invia(client_sck, buffer);// viene chiuso il breve handshake iniziale col server
    if(ret<0)
        exit(1);
    memset(buffer, 0, sizeof(buffer));

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(STDIN_FILENO, &master);
    FD_SET(client_sck, &master);

    while(1){
        memset(buffer, 0, sizeof(buffer));
        read_fds=master;
        ret=select(client_sck+1, &read_fds, NULL, NULL, NULL);
        if(ret<0){
            perror("Errore nella select");
            exit(1);
        }

        for(i=0; i<=client_sck; i++){
            if(FD_ISSET(i, &read_fds)){
                if(i==STDIN_FILENO){
                    memset(buffer, 0, sizeof(buffer));
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strcspn(buffer, "\n")] = 0; // Rimuove il newline alla fine della stringa in modo che il confronto che avverrà successivamente non fallisca
                    //si invia al server il comando (o la risposta all'enigma o la parola della funzione speciale)
                    ret=invia(client_sck, buffer);
                    if(ret<0)
                        exit(1);
                    memset(buffer, 0, sizeof(buffer));
                    if(guess_attiva){
                        printf("Attendi che l'aiutante inserisca la sua parola\n");
                    }
                    //aspettiamo di ricevere dal server la risposta la comando inviato
                    ret=ricevi(client_sck, buffer); 
                    if(ret<0)
                        exit(1);
                    if(ret==0){
                        printf("Il server ha terminato la connessione\n");
                        close(client_sck);
                        exit(0);
                    }
                    //serve a pulire lo stdin quando il giocatore è in attesa che il l'aiutante scriva una parola
                    //questo perchè se il giocatore scrivesse qualcosa durante l'attessa ci possono essere problemi successivamente
                    tcflush(STDIN_FILENO, TCIFLUSH);

                    if(ret<0)
                        exit(1);
                    //se il comando speciale è attivo il client deve comportarsi diversamente e controllare le informazioni che gli ha inviato il server
                    if(guess_attiva){
                        strcpy(buffer_guess, buffer);
                        token1=strtok(buffer_guess, " ");
                        //va controllato se la funzione speciale è terminata in modo da cambiare la variabile guess_attiva
                        if(strcmp(token1,"TERMINA")==0){
                            guess_attiva=false;
                            token2=strtok(NULL, " ");
                            //viene stampato un messaggio diverso a seconda se la funzione speciale 
                            //è terminata per l'esaurimento del tempo o perchè è stata scritta la stessa parola
                            if(strcmp(token2,"TEMPO")==0){
                                memset(buffer, 0, sizeof(buffer));
                                strcpy(buffer, "Avete esaurito il tempo a disposizione. Non guadagni tempo extra");
                            }
                            else if(strcmp(token2,"INDOVINATA")==0){
                                memset(buffer, 0, sizeof(buffer));
                                strcpy(buffer, "Siete riusciti a capirvi. Hai guadagnato tempo extra");
                            }
                        }
                        memset(buffer_guess, 0, sizeof(buffer_guess));
                    }
                    //se il server ci ha inviato il comando per attivare la funzione speciale allora settiamo la variabile e il giocatore potrà scrivere la parola
                    if(strcmp(buffer,"GUESS ATTIVA")==0){
                        guess_attiva=true;
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, "Scrivi la tua parola");
                    }
                    //se il client giocatore ha digitato il comando end allora il server invia una stringa che ci permette di terminare la connessione tra client e server
                    if(strcmp(buffer,"END")==0){
                        printf("Grazie per aver giocato! Ci auguriamo che tu ti sia divertito\n");
                        close(client_sck);
                        exit(0);
                    }
                    printf("%s\n", buffer);
                    if(!guess_attiva)
                        printf("____________________________________________________________________________\n\n");                    
                }

                else if(i==client_sck){ 
                    memset(buffer, 0, sizeof(buffer));
                    ret=ricevi(client_sck, buffer);
                    if(ret<0) 
                        exit(1);
                
                    if(ret==0){
                        printf("Il server ha terminato la connessione\n");
                        close(client_sck);
                        exit(0);
                    }
                    printf("%s\n", buffer);
                    printf("____________________________________________________________________________\n\n");     
                }
            }
        }
    }
}
