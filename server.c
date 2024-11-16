#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdbool.h>
#include "utility.h"


int main (int argc, char* argv []){
    int fdmax, ret, i, porta, listener_sck, newfd, giocatore_sck=-1, aiutante_sck=-1;
    
    bool start = false; 

    fd_set master; 
    fd_set read_fds;
    

    socklen_t addrlen; 

    struct sockaddr_in server_addr; 
    struct sockaddr_in client_addr; 
    char buffer[1024]; 

    porta = htons(4242); 
    // viene creato il socket del server. 
    listener_sck = socket (AF_INET, SOCK_STREAM, 0); 
    if(listener_sck < 0){
        perror("Errore nella creazione del socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = porta;

    server_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(listener_sck, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(ret < 0){
        perror("Errore nella bind");
        exit(1);
    }

    ret = listen(listener_sck, 2);
    if(ret < 0){
        perror("Errore nella listen");
        exit(1);
    }

    //viene settato l'i/o multiplexing
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(STDIN_FILENO, &master);
    FD_SET(listener_sck, &master);

    fdmax = listener_sck;

    mostra_comandi_server();

    while(1){
        //il server inizialmente attende ciclicamente l'arrivo del comando dallo standard input.
        //solo una volta ricevuto il comando start, il server si mette in ascolto dei client.
        memset(buffer, 0, sizeof(buffer));
        read_fds = master;
        ret=select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if(ret < 0){
            perror("Errore nella select");
            exit(1);
        }
        
        for(i=0; i<=fdmax;i++){
            if(FD_ISSET(i, &read_fds)){
                if(i==STDIN_FILENO){
                    scanf("%s", buffer);
                    if(strcmp(buffer, "start") == 0){
                            if(!start){
                                start = true;
                                printf("Server avviato\n");
                    }
                    else
                        printf("Server gia' avviato\n");
                }
                else if(strcmp(buffer, "stop") == 0){
                    if(start){
                        start = false;
                        printf("Server terminato\n");
                        for(i = 0; i <= fdmax; i++) {
                            if(FD_ISSET(i, &master)) {
                            close(i);
                            FD_CLR(i, &master);
                            }
                        }
                        exit(0);
                    }
                }
                else
                    printf("Comando non valido\n");
            }
            else if(start && i==listener_sck){
                // una volta che il server è avviato e arriva una richiesta di connessione da parte del client
                //viene distinto il tipo di client connesso per distinguere i brevi handshake iniziali
                //se si connette un giocatore deve essere inviato il messaggio di benvenuto aspettando la risposta
                //se si connette un aiutante deve essere inviato un messaggio diverso in modo che si possa mettere in attesa 
                //che il il giocatore attivi la funzionalità speciale 
                addrlen = sizeof(client_addr);
                memset(&client_addr, 0, sizeof(client_addr));
                newfd = accept(listener_sck, (struct sockaddr *) &client_addr, &addrlen);
                if(newfd < 0){
                    perror("Errore nella accept");
                    exit(1);
                }
                
                printf("Nuovo client con socket %d connesso\n", newfd);
                memset(buffer, 0, sizeof(buffer));
                ret=ricevi(newfd, buffer);
                if(ret < 0)
                    exit(1);
                if(strcmp(buffer, "giocatore") == 0){
                printf("Si è connesso un %s\n", buffer);
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Benvenuto nel server di gioco\nPrima di iniziare a giocare devi effettuare il login\n");
                strcat(buffer, lista_comandi());
                ret = invia(newfd, buffer);
                if(ret < 0)
                    exit(1);
                memset(buffer, 0, sizeof(buffer));
                ret = ricevi(newfd, buffer);
                if(ret < 0)
                    exit(1);
                
                if(strcmp(buffer, "ricevuti") == 0)
                    printf("Comandi di accesso inviati e ricevuti con successo\n");
                memset(buffer, 0, sizeof(buffer));
                giocatore_sck = newfd;
            }
            else if(strcmp(buffer, "aiutante") == 0){
                printf("Si è connesso un %s\n", buffer);
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "attendi");
                ret = invia(newfd, buffer);
                if(ret < 0)
                    exit(1);
                memset(buffer, 0, sizeof(buffer));
                ret = ricevi(newfd, buffer);
                if(ret < 0)
                    exit(1);
                if(strcmp(buffer, "ok") == 0)
                    printf("L'aiutante si è messo in attesa correttamente\n");
                memset(buffer, 0, sizeof(buffer));
                aiutante_sck = newfd;
            }

                FD_SET(newfd, &master);
                if(newfd > fdmax)
                    fdmax = newfd; 
            }
            else{
                if(start){
                    //Se ci sono client connessi il server aspetta di ricevere comandi e gestirli evntualmente con una apposita funzione
                    ret = ricevi(i, buffer);
                    if(ret < 0)
                        exit(1);
                    else if(ret == 0){
                        printf("Chiusura del socket %d\n", i);
                        close(i);
                        printf("Socket %d chiuso\n", i);
                        FD_CLR(i, &master);
                        printf("Socket %d rimosso dal master\n", i);
                    }
                    else{
                        printf("socket %d comando %s\n", i, buffer);
                        gestisci_comandi_client(i, aiutante_sck, buffer);
                    }
                }
            }
        }
    }

}
}