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
#include "utility.h"


int main(int argc, char *argv[]) {
    //il client aiutnate viene creato in modo analogo al client giocatore con delle differenze nell'handshake iniziale
    int ret, i, other_sck, porta;
    fd_set master, read_fds;
    struct sockaddr_in server_addr;
    char buffer[1024];

    porta=htons(4242);

    other_sck=socket(AF_INET, SOCK_STREAM, 0);
    if(other_sck<0){
        perror("Errore nella creazione del socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=porta;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    ret=connect(other_sck, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret<0){
        perror("Errore nella connect");
        exit(1);
    }

    //il client aiutante deve far capire al server che è un aiutante
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "aiutante");
    ret=invia(other_sck, buffer);
    if(ret<0)
        exit(1);
    memset(buffer, 0, sizeof(buffer));
    ret=ricevi(other_sck, buffer);
    if(ret<0){
        exit(1);
    }

    if(ret==0){
        printf("Il server ha chiuso la connessione\n");
        close(other_sck);
        exit(0);
    }
    //dal server si riceve il comando di attesa e il client aiutante risponde al server chiudendo l'handshake iniziale
    if(strcmp(buffer, "attendi")==0){
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "ok");
        ret=invia(other_sck, buffer);
        
    }

    
    while(1){
        //ciclo che serve a poter riusare la guess nelle nuove partite dopo che è stata usata nelle vecchie. Una volta terminata la funzionalità speciale guess il client aiutante si rimette in attesa aspettando di essere richiamato

        memset(buffer, 0, sizeof(buffer));
        printf("Attendi che il giocatore richieda il tuo aiuto\n");
        //il client aiutante rimane in attesa di ricevere il messaggio che il giocatore ha richiesto il suo aiuto
        ret=ricevi(other_sck, buffer);
        if(ret<0)
            exit(1);

        if(ret==0){
            printf("Il server ha chiuso la connessione\n");
            close(other_sck);
            exit(0);
        }   
        //viene inviata al server la risposta di essere pronti
        ret=invia(other_sck, "ok");
        if(ret<0)
            exit(1);
        printf("Il giocatore ha richiesto il tuo aiuto\n");


        while(1){
            //l'aiutante attenderà che il giocatore scriva la sua parola
            printf("Attendi che il giocatore inserisca la sua parola, intanto pensa alla tua\n");
            memset(buffer, 0, sizeof(buffer));
            ret=ricevi(other_sck, buffer);
            if(ret<0)
                exit(1);
            if(ret==0){
                printf("Il server ha chiuso la connessione\n");
                close(other_sck);
                exit(0);
            } 
            //se dal server viene ricevuta la seguente stringa allora vuol dire che è stato esaurito il tempo e il client aiutante si può rimettere in attesa di essere nuovamente chiamato
            if(strcmp(buffer, "Tempo")==0){
                printf("\nPurtroppo ci avete messo troppo tempo. Il giocatore non è riuscito a guadagnare tempo extra\n");
                break;
            }
            while(1){
                //ciclo che serve a controllare a svuotare lo STDIN mentre l'aiutante era in attesa di ricevere la parola dal giocatore
                //serve anche a controllare che l'aiutante inserisca una parola
                tcflush(STDIN_FILENO, TCIFLUSH);
                printf("Scrivi la tua parola:");
                memset(buffer, 0, sizeof(buffer));
                fgets(buffer, sizeof(buffer), stdin);
                if(buffer[0]!='\n' && buffer[0]!='\0')
                    break;
                printf("Formato della parola non valido\n");
            }
            //la parola verrà inviata al server che la invierà a sua volta al giocatore
            ret=invia(other_sck, buffer);
            if(ret<0)
                exit(1);
            memset(buffer, 0, sizeof(buffer));
            //il server ci invia la parola scritta dal giocatore
            ret=ricevi(other_sck, buffer);
            if(ret<0)
                exit(1);

            if(ret==0){
            printf("Il server ha chiuso la connessione\n");
            close(other_sck);
            exit(0);
            } 
            //se dal server viene ricevuta la seguente stringa allora vuol dire che la parola è stata indovinata e il client aiutante si può rimettere in attesa di essere nuovamente chiamato
            if(strcmp(buffer, "Chiudi")==0){
                printf("\nGrazie a te il giocatore ha guadagnato tempo extra\n");
                break;
            }
            printf("La parola scritta dal giocatore è: %s\n", buffer);
            
        }
    }

}