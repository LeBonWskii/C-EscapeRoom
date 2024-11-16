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
#include <time.h>
#include "cmd_client.h"
#include "utility.h"


//ricontrollare tutto il progetto aggiungendo i commenti e testarlo nella sua interezza
//fare la documentazione
struct user_on* user_on_testa=NULL; //testa della lista per gli uteti loggati
struct partita* partita_testa=NULL; //testa della lista per le partite in corso
char buf_risposta[1024]; //buffer per la risposta da inviare al client

//funzione che controlla se è attivo un enigma al momento della gestione del comando
//dato che se ci fosse un enigma attivo quello che stiamo ricevendo non è un comando ma la risposta dell'enigma
bool enigma_attivo(int sd){
    struct partita* partita=trova_partita(partita_testa,sd);
    if(partita==NULL)
        return false;
    return partita->enigma_attivo;
}

//funzione che controlla se è attiva la guess al momento della gestione del comando
//dato che se ci fosse una guess attiva quello che stiamo ricevendo non è un comando ma la parola del client 
bool guess_attivo(int sd){
    struct partita* partita=trova_partita(partita_testa,sd);
    if(partita==NULL)
        return false;
    return partita->guess_attivo;
}


//funzione che gestisce la risposta ad un enigma
//ogni enigma ha la sua posizione nella lista degli enigmi e la risposta corretta
//tramite la posizione dell'enigma attivo si controlla la risposta data e si gestisce i vari casi
//se la risposta data è sbagliata viene disattivato l'enigma in modo che i messaggi ricevuti dal client siano interpretati nuovamente come comandi
char* gestore_risposta(int sd,char* risposta){
    struct enigma* enigma;
    struct partita* partita=trova_partita(partita_testa,sd);
    enigma = trova_enigma(partita->pos_enigma_attivo);

    if(strcmp(risposta,enigma->risposta)==0){

        if(partita->pos_enigma_attivo==0){
            partita->oggetti[0].bloccato=false;
            partita->enigma_attivo=false;
            partita->pos_enigma_attivo=-1;
            return "Hai risolto l'enigma. Sul libro è apparsa la ricetta";
        }

        if(partita->pos_enigma_attivo==1){
            partita->oggetti[8].bloccato=false;
            partita->enigma_attivo=false;
            partita->pos_enigma_attivo=-1;
            partita->token_raccolti++;
            if(partita->token_raccolti==N_TOKEN){
                rimuovi_partita(&partita_testa,sd);
                return "Complimenti. Hai ricostruito la chiave e sei riuscito a fuggire dalle grinfie di Pedro. Ora ti aspetta una vita in libertà.\n\nPer iniziare una nuova partita digita nuovamente start Villa, altrimenti digita end";
            }
                
            return "Hai risolto l'enigma. La cassaforte si è sbloccata e hai preso una pezzo della chiave";
        }

        if(partita->pos_enigma_attivo==2){
            partita->oggetti[11].bloccato=false;
            partita->oggetti[14].bloccato=false;
            partita->enigma_attivo=false;
            partita->pos_enigma_attivo=-1;
            partita->oggetti[14].prendibile=true;
            return "Hai risolto l'enigma. Il lucchetto della cassetta si è aperto e ora puoi vedere cosa c'è nella cassetta";
        }

        if(partita->pos_enigma_attivo==3){
            partita->oggetti[12].bloccato=false;
            partita->enigma_attivo=false;
            partita->pos_enigma_attivo=-1;
            partita->oggetti[15].prendibile=true;
            return "Hai risolto l'enigma. La teca si è aperta e puoi vedere cosa c'è dentro";
        }


    }

    partita->enigma_attivo=false;
    partita->pos_enigma_attivo=-1;
    return "Risposta errata";
}


//funzione che gestisce la registrazione di un utente.
//username e password non possono superare i 20 caratteri e non si possono avere due username uguali
//Se tutto va a buon fine, l'account viene salvato nel file utenti.txt
//Uno stesso client può creare piu account
char* gestore_register(char* username,char* password, char* errore){

    FILE* file=NULL;
    char* line=NULL;
    size_t len=0;


    if(username==NULL || password==NULL)
        return "La sintassi del comando prevede sia username che password";
    if(errore!=NULL)
        return "Sono stati immessi troppo parametri";
    if(strlen(username)>20)
        return "Username troppo lungo: dimensione massimo 20 caratteri";
    if(strlen(password)>20)
        return "Password troppo lunga: dimensione massimo 20 caratteri";
    
    file=fopen("utenti.txt","a+");
    if(file==NULL)
        return "Errore nell'apertura del file utenti.txt";
    
    while(getline(&line,&len,file)!=-1){
        if(strcmp(strtok(line," "),username)==0){
            free(line);
            fclose(file);
            return "Username già esistente";
        }
    }
    fprintf(file,"%s %s\n",username,password);
    free(line);
    fclose(file);
    return "Utente registrato con successo con successo";   
}

//funzione che gestisce il login di un utente
//Vengono confrontati username e password forniti con quelli presenti nel file.
//Se il login avviene con successo viene creato un nuovo elemento usern_on e inserito nella lista degli account online
//e al client vengono dati gli scenari disponibili in modo che possa avviare una partita
char* gestore_login(int sd,char* username,char* password, char* errore){
    FILE* file=NULL;
    char* line=NULL;
    size_t len=0;
    char* user=NULL;
    char* pass=NULL;
    struct user_on* new_user=NULL;

    if(username==NULL || password==NULL)
        return "La sintassi del comando prevede sia username che password";

    if(errore!=NULL)
        return "Sono stati immessi troppo parametri";

    if(strlen(username)>20)
        return "Username troppo lungo: dimensione massimo 20 caratteri";

    if(strlen(password)>20)
        return "Password troppo lunga: dimensione massimo 20 caratteri";

    if(trova_utente(user_on_testa,sd)!=NULL)
        return "Utente già loggato";
    
    file=fopen("utenti.txt","a+");
    if(file==NULL)
        return "Errore nell'apertura del file utenti.txt";
    
    while(getline(&line,&len,file)!=-1){
        line[strcspn(line,"\n")]=0; //rimuovo il newline dalla stringa letta con getline
        user=strtok(line," ");
        pass=strtok(NULL," ");
        if(strcmp(user,username)==0 && strcmp(pass,password)==0){
            free(line);
            fclose(file);
            new_user=new_user_on(&user_on_testa,sd);
            if(new_user==NULL)
                return "Impossibile effettuare il login: errore di allocazione della memoria";

            return "Login effettuato con successo\nScenari Presenti:\n1)Villa";
        }
    }
    free(line);
    fclose(file);
    return "Username o password errati";
}

//funxione che gestisce la creazione di una nuova partita
//Se l'uente avvia la partita viene creato un elemento partita e inserito nella lista delle partite in corso
char* gestore_start_room(int sd,char* stanza,char* errore){
    struct partita* nuova_partita=NULL;
    if(stanza==NULL)
        return "La sintassi del comando prevede la stanza";

    if(errore!=NULL)
        return "Sono stati immessi troppo parametri";
    
    if(trova_utente(user_on_testa,sd)==NULL)
        return "Devi effettuare il login per avviare una partita";

    if(strcmp(stanza,"Villa")!=0)
        return "Stanza non esistente";
    
    if(trova_partita(partita_testa,sd)!=NULL)
        return "Hai già avviato una partita";

    nuova_partita=new_partita(&partita_testa,sd);
    if(nuova_partita==NULL)
        return "Impossibile avviare la partita: errore di allocazione della memoria";

    return "Partita avviata con successo\nSei stato rapito dal capo del villaggio Pedro e sei chiuso nella sua villa.\nAdesso è fuori e sei in casa da solo. Riuscirai a fuggire prima che torni?\nHai 30 minuti di tempo a disposizione e 2 pezzi della chiave da raccogliere. Buona fortuna!";
}

//funzione che implementa il comando look
//Viene prima cercata la descrizione da restituire nel vettore delle locazioni.
//Le locazioni non cambiano nel corso della partita quindi per  cercare la lcazioni abbiamo bisogno solo del nome
//Se non viene trovata la locazione, viene cercato l'oggetto

char* gestore_look(int sd,char* param,char* errore){
    char* locazione=NULL;
    memset(buf_risposta,0,sizeof(buf_risposta));
    
    struct partita* partita=trova_partita(partita_testa,sd);

    if(partita==NULL)
        return "Non hai ancora avviato una partita";


    if(errore!=NULL){
        strcpy(buf_risposta,"Sono stati immessi troppo parametri");
        strcat(buf_risposta, stato_partita(partita));
        return buf_risposta;
    }
    
    if(param==NULL)
        param="principale";

    locazione=trova_locazione(param);

    if(locazione!=NULL){
        strcpy(buf_risposta,locazione);
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    struct oggetto* oggetto=trova_oggetto(partita_testa,sd,param);
    if(oggetto!=NULL){
        if(oggetto->bloccato){
            strcpy(buf_risposta,oggetto->descrizione_bloccato);
            strcat(buf_risposta,stato_partita(partita));
            return buf_risposta;
        }
        else{
            strcpy(buf_risposta,oggetto->descrizione_libero);
            strcat(buf_risposta,stato_partita(partita));
            return buf_risposta;
        }
    }


    strcpy(buf_risposta,"Nome locazione o oggetto non valido");
    strcat(buf_risposta,stato_partita(partita));
    return buf_risposta;
}

//funzione di implementazione del comando take
//Se l'oggetto è bloccato e vie è associato un enigma allora viene attivato l'enigma e viene restituita l'enigma
//Se l'oggetto può essere raccolto e l'inventario non è pieno, viene raccolto l'oggetto
char* gestore_take(int sd,char* obj,char* errore){
    int i;
    struct enigma* enigma;
    struct partita* partita=trova_partita(partita_testa,sd);

    if(partita==NULL)
        return "Non hai ancora avviato una partita";
    
    memset(buf_risposta,0,sizeof(buf_risposta));

    if(errore!=NULL){
        strcpy(buf_risposta,"Sono stati immessi troppo parametri");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    struct oggetto* oggetto=trova_oggetto(partita_testa,sd,obj);

    if(oggetto==NULL){
        strcpy(buf_risposta,"Nome oggetto non valido");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    if(oggetto->bloccato && oggetto->pos_enigma!=-1){
        partita->enigma_attivo=true;
        partita->pos_enigma_attivo=oggetto->pos_enigma;
        enigma=trova_enigma(oggetto->pos_enigma);
        return enigma->domanda;
    }

    if(!oggetto->prendibile){
        strcpy(buf_risposta,"Non puoi prendere questo oggetto");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    for(i=0;i<MAX_OGGETTI_RACCOLTI;i++){
        if(partita->oggetti_raccolti[i]==NULL)
            break;
    }

    if(i==MAX_OGGETTI_RACCOLTI){
        strcpy(buf_risposta,"Il tuo inventario è pieno");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    
    partita->oggetti_raccolti[i]=oggetto->nome;
    oggetto->prendibile=false;
    oggetto->descrizione_libero="L'oggetto è nel tuo inventario";

    if(strcmp(oggetto->nome,"telecomando")==0)
        partita->oggetti[10].descrizione_libero="L'acqua se ne è andata. Il water è vuoto";
    if(strcmp(oggetto->nome,"chiave_inglese")==0)
        partita->oggetti[11].descrizione_libero="La cassetta contiene varia attrezzi che non ti servono";
    if(strcmp(oggetto->nome,"tasto")==0)
        partita->oggetti[12].descrizione_libero="La teca è vuota";

    sprintf(buf_risposta,"Hai raccolto %s",oggetto->nome);
    strcat(buf_risposta,stato_partita(partita));
    return buf_risposta;
}

//funzione di implementazione del comando use
//Viene controllato se il nome degli oggetti è valido.
//Nel comando use devono essere presenti sempre due oggetti per come è stato implementato il gioco
//Un oggetto per poter essere usato deve essere stato raccolto e non deve essere già stato usato
//L'utilizzo di un oggetto non comporta la rimozione dell'oggetto dagli oggetti raccolti 
char* gestore_use(int sd,char* obj1,char* obj2, char* errore){
    struct partita* partita=trova_partita(partita_testa,sd);
    struct oggetto* oggetto1;
    struct oggetto* oggetto2;
    if(partita==NULL)
        return "Non hai ancora avviato una partita";

    memset(buf_risposta,0,sizeof(buf_risposta));

    if(errore!=NULL){
        strcpy(buf_risposta,"Sono stati immessi troppo parametri");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    if(obj1==NULL || obj2==NULL){
        strcpy(buf_risposta,"La sintassi del comando prevede due oggetti");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    oggetto1=trova_oggetto(partita_testa,sd,obj1);
    oggetto2=trova_oggetto(partita_testa,sd,obj2);

    if(oggetto1==NULL || oggetto2==NULL){
        strcpy(buf_risposta,"Nome oggetto non valido");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
        
    if(strcmp(oggetto1->nome,"telecomando")==0 && strcmp(oggetto2->nome,"televisore")==0){
        if(oggetto1->usato){
            strcpy(buf_risposta,"Hai già usato il telecomando");
            strcat(buf_risposta,stato_partita(partita));
            return buf_risposta;
        }
        if(!oggetto_raccolto(partita,oggetto1->nome)){
            strcpy(buf_risposta,"Devi prima raccogliere il telecomando per poterlo usare");
            strcat(buf_risposta,stato_partita(partita));
            return buf_risposta;
            }

        oggetto1->usato=true;
        oggetto2->usato=true;
        partita->oggetti[4].bloccato=false;
        strcpy(buf_risposta,"Il televisore si è acceso");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    if(strcmp(oggetto1->nome,"chiave_inglese")==0 && strcmp(oggetto2->nome,"water")==0){
        if(oggetto1->usato){
            strcpy(buf_risposta,"Hai già usato la chiave inglese");
            strcat(buf_risposta,stato_partita(partita));
            return buf_risposta;
        }

        if(!oggetto_raccolto(partita,oggetto1->nome)){
                strcpy(buf_risposta,"Devi prima raccogliere la chiave inglese per poterla usare");
                strcat(buf_risposta,stato_partita(partita));
                return buf_risposta;
            }
        oggetto1->usato=true;
        oggetto2->usato=true;
        partita->oggetti[10].bloccato=false;
        partita->oggetti[13].bloccato=false;
        partita->oggetti[13].prendibile=true;

        strcpy(buf_risposta,"Hai aperto il water. L'acqua sta scendendo");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    if(strcmp(oggetto1->nome,"tasto")==0 && strcmp(oggetto2->nome,"pianoforte")==0){
        if(oggetto1->usato){
            strcpy(buf_risposta,"Hai già usato il tasto");
            strcat(buf_risposta,stato_partita(partita));
            return buf_risposta;
        }

        if(!oggetto_raccolto(partita,oggetto1->nome)){
                strcpy(buf_risposta,"Devi prima raccogliere il tasto per poterlo usare");
                strcat(buf_risposta,stato_partita(partita));
                return buf_risposta;
            }

        oggetto1->usato=true;
        oggetto2->usato=true;
        partita->oggetti[1].bloccato=false;
        partita->oggetti[3].bloccato=false;
        partita->token_raccolti++;
            if(partita->token_raccolti==N_TOKEN){
                rimuovi_partita(&partita_testa,sd);
                return "Hai completato il pianoforte. Sotto la scrivania si è aperto un cassetto. Hai trovato un pezzo di chiave.\n\nComplimenti. Hai ricostruito la chiave e sei riuscito a fuggire dalle grinfie di Pedro. Ora ti aspetta una vita in libertà.\n\nPer iniziare una nuova partita digita nuovamente start Villa, altrimenti digita end";
            }
        strcpy(buf_risposta,"Hai completato il pianoforte. Sotto la scrivania si è aperto un cassetto. Hai trovato un pezzo di chiave.");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    strcpy(buf_risposta,"Non puoi utilizzare il comando use con questi oggetti");
    strcat(buf_risposta,stato_partita(partita));
    return buf_risposta;
}

//funzione che gestisce il comando objs
//Si scorre l'array degli oggetti raccolti fino a che non si trova una posizione vuota o fino a che non si raggiunge la fine dell'array
//Vengono restituiti gli oggetti che sono stati raccolti. Se non ce ne sono viene restituito un apposito messaggio
char* gestore_objs(int sd,char* errore){

    int i;
    struct partita* partita=trova_partita(partita_testa,sd);

    
    if(partita==NULL)
        return "Non hai ancora avviato una partita";

    memset(buf_risposta,0,sizeof(buf_risposta));



    if(errore!=NULL){
        strcpy(buf_risposta,"Sono stati immessi troppo parametri");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    strcpy(buf_risposta,"I tuoi oggetti raccolti sono:\n");

    for(i=0;i<MAX_OGGETTI_RACCOLTI;i++){
        if(partita->oggetti_raccolti[i]==NULL)
            break;
            
        strcat(buf_risposta,partita->oggetti_raccolti[i]);
        strcat(buf_risposta,"\n");
    }
    if(i==0){
        memset(buf_risposta,0,sizeof(buf_risposta));
        strcpy(buf_risposta,"Non hai ancora raccolto nessun oggetto");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    strcat(buf_risposta,stato_partita(partita));
    return buf_risposta;

}

//funzione che gestisce l'attivazione della funzionalità speciale contattando il client aiutante
//Si cerca di contattare il client aiutante inviandogli una stringa di attivazione, dato che fino ad adesso era rimasto in attesa di ricevere qualcosa
//Se si riceve correttamente la risposta dal client aiutante, viene attivata la guess con il tempo limite e viene restituito un messaggio di conferma
char* attiva_guess(int giocatore_sd,int aiutante_sd,char* errore){
    int ret;
    struct partita* partita=trova_partita(partita_testa,giocatore_sd);

    memset(buf_risposta,0,sizeof(buf_risposta));

    if(errore!=NULL){
        strcpy(buf_risposta,"Sono stati immessi troppo parametri");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    
    if(aiutante_sd==-1){
        strcpy(buf_risposta,"L'aiutante non si è ancora connesso");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    if(partita==NULL)
        return "Non hai ancora avviato una partita";
    
    if(partita->fine_partita<=time(NULL)+60*5){
        strcpy(buf_risposta,"Non hai abbastanza tempo per usare la funzionalità speciale");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    if(partita->guess_usata){
        strcpy(buf_risposta,"Hai già chiamato l'aiutante");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    
    ret=invia(aiutante_sd,"guess");

    if(ret<0){
        strcpy(buf_risposta,"Non è stato possibile contattare l'aiutante");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }

    ret=ricevi(aiutante_sd,buf_risposta);

    if(ret<0){
        strcpy(buf_risposta,"Errore nella ricezione della risposta dall'aiutante");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    
    printf("L'aiutante è stato chiamato correttamente\n");

    partita->guess_attivo=true;
    partita->fine_guess=time(NULL)+60*5;

    
    return "GUESS ATTIVA";  
}

//funzione che gestisce la funzionalità speciale.
//La funionalità speciale consiste nel client giocatore e nel client aiutante che devono cercare
//di scrivere la stessa parola scrivendo ad ogni turno una parola ciascuno.
//Se la funzione guess è attiva allora il comando scritto dal client giocatore viene interpretato come la parola scritta
//Viene controllato che il tempo per la funzione speciale non sia scaduto, se è cosi viene disattivata la funzionalità speciale e si restituisce alla stringa ai client aiutante e giocatore che gli faccia gestire la fine della funzionalità speciale
//Se il tempo non è scaduto, viene inviata al client aiutante una stringa che gli permetta di scrivere la parola e poi si aspetta la risposta
//Viene presa solo la prima parola data dal client aiutante e viene confrontata con quella data dal client giocatore
//Se le parole sono uguali, la funzionalità speciale viene disattivata e viene aggiunto del tempo alla partita, e infine inviata una stringa per la gestione della fine della funzionalità speciale
//Se le parole non sono uguali, viene inviata al client aiutante la parola data dal client giocatore e viceversa.

char* gestore_guess(int giocatore_sd,int aiutante_sd,char* parola_giocatore){
    static char risposta_aiutante[1024];
    char* parola_aiutante;
    int ret;
    struct partita* partita=trova_partita(partita_testa,giocatore_sd);

    memset(risposta_aiutante,0,sizeof(risposta_aiutante));  
    if(partita->fine_guess<=time(NULL)){
        partita->guess_attivo=false;
        partita->guess_usata=true;
        ret=invia(aiutante_sd,"Tempo");
        return "TERMINA TEMPO";
    }
    ret=invia(aiutante_sd,"Scrivi");

    if(ret<0)
        return "Errore nel contattare l'aiutante";
    
    ret=ricevi(aiutante_sd,risposta_aiutante);

    if(ret<0)
        return "Errore nella ricezione della risposta dall'aiutante";

    parola_aiutante=strtok(risposta_aiutante," ");
    parola_aiutante[strcspn(parola_aiutante, "\n")] = 0;
    if(strcmp(parola_aiutante,parola_giocatore)==0){
        partita->guess_attivo=false;
        partita->guess_usata=true;
        partita->fine_partita+=60*10;
        ret=invia(aiutante_sd,"Chiudi");
        return "TERMINA INDOVINATA";
    }
    ret = invia(aiutante_sd,parola_giocatore);
    if(ret<0)
        return "Errore nell'invio della risposta al giocatore";
    sprintf(parola_giocatore,"La parola scritta dall'aiutante è: %s\nScrivi la tua parola ",parola_aiutante);

    return parola_giocatore;
    
}

//funzione che gestisce il comando end
//Se il client è loggato, viene rimosso dalla lista degli utenti loggati
//Se il client ha avviato una partita, viene rimossa la partita dalla lista delle partite in corso
//Viene restituita la stringa "END" per gestire la chiusura della connessione
char* gestore_end(int sd,char* errore){
    struct user_on* user;
    struct partita* partita;
    partita=trova_partita(partita_testa,sd);

    if(errore!=NULL){
        strcpy(buf_risposta,"Sono stati immessi troppo parametri");
        strcat(buf_risposta,stato_partita(partita));
        return buf_risposta;
    }
    
    user=trova_utente(user_on_testa,sd);
    if(user!=NULL)
        rimuovi_utente(&user_on_testa,sd);
        
    if(partita!=NULL)
        rimuovi_partita(&partita_testa,sd);
    

    return "END";
}

//funzione di utilita che restituisce contralla se è stato esaurito il tgempo a disposizione per la partita
bool tempo_esaurito(int sd){
    struct partita* partita=trova_partita(partita_testa,sd);
    if(partita==NULL)
        return false;
    if(partita->fine_partita<=time(NULL))
        return true;

    return false;
}

//funzione che gestisce la fine del tempo a disposizione per la partita
//Se il tempo è finito, viene rimossa la partita dalla lista delle partite in corso e viene restituita una stringa di fine partita
char* gestore_fine_tempo(int sd){
    rimuovi_partita(&partita_testa,sd);
    return "Hai esaurito il tempo a dispozione. Pedro è tornato a casa e questa volta non te la farà passare liscia.\n\nPer iniziare una nuova partita digita nuovamente start_room Villa, altrimenti digita end";
}





