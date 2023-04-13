#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <cstdlib>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <time.h>
#include <cstring>

/* portul folosit */
#define PORT 3407

/* codul de eroare returnat de anumite apeluri */
extern int errno;
int utilizatorul_exista, client_autentificat, lungime_sir, comanda_ok, prim, semn, gasit_statie_start, gasit_1_tren, tren, gasit_statie, gasit_ora, gasit_delay, linia, gasit_statie_s, gasit_statie_p, gasit_ora_p, prima_data;
char numar_tren[10], statie_p[15], statie_p1[15], statie_s[15], ora_s[10], ora_p[10], delay[10], ora_sosire[10], char_ora[5], char_minute[5], ora_anterioara[10];
char comanda_primita_1[100];		//mesajul primit de la client 
char comanda_primita_2[100];		//mesajul primit de la client 
char comanda_trimisa[2000];		//mesajul trimis la client 
char username[30], parola[30], nume_statie_c[30];


typedef struct thData {
    int idThread; //id-ul thread-ului tinut in evidenta de acest program
    int cl; //descriptorul intors de accept
}thData;

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;    // variabila mutex ce va fi partajata de threaduri

static void* treat(void*); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void* logare(void* arg);
int autentificare_client(xmlNode* radacina, char username[30], char parola[30]); //functie ce verifica daca numele utilizatorului
                                                                                // primit de la client exista in fisierul 'clienti.xml'
void* client_logat(void* arg, char username[30]);
void* client_statie(void* arg, char username[30]);
void* client_statie_afisare_sosiri(void* arg, char username[30], char nume_statie[30]);
const char* afisare_sosiri(xmlNode* radacina, char nume_statie[30], int prim);
void* client_statie_afisare_plecari(void* arg, char username[30], char nume_statie[30]);
const char* afisare_plecari(xmlNode* radacina, char nume_statie[30], int prim);
void* client_tren(void* arg, char username[30]);
void* client_tren_afisare_inf_tren(void* arg, char username[30]);
const char* informatii_tren(xmlNode* radacina, char nume_tren[30], int prim);
void* actualizare_orar_tren(void* arg, char username[30]);
const char* verif_statie_ok_tren(xmlNode* radacina, char nume_tren[30], char nume_statie[30], int prim);
const char* actualizare_sosiri_tren(xmlNode* radacina_2, xmlDoc* document, char nume_tren[30], char nume_statie[40], char numar_minute[10], int semn);
void* client_calator(void* arg, char username[30]);
const char* verif_statie(xmlNode* radacina, char nume_statie[30], int prim);
void* calator_afisare_sosiri(void* arg, char username[30]);
void* calator_afisare_plecari(void* arg, char username[30]);
void* client_calator_afisare_sosiri_1h(void* arg, char username[30], char nume_statie[30]);
const char* afisare_sosiri_1h(xmlNode* radacina, char nume_statie[30], struct tm* timeinfo, int prim);
void* client_calator_afisare_plecari_1h(void* arg, char username[30], char nume_statie[30]);
const char* afisare_plecari_1h(xmlNode* radacina, char nume_statie[30], struct tm* timeinfo, int prim);
void* client_calator_cauta_tren(void* arg, char nume_statie[30], char nume_statii[50]);
const char* cauta_trenuri(xmlNode* radacina, char statie_1[30], char statie_2[30], int prim);
int write_info(void* arg, char comanda_trimisa[1000], int linia);
const char* read_info(void* arg, int linia);

int main()
{
    struct sockaddr_in server;	// structura folosita de server
    struct sockaddr_in from;
    int sd;		//descriptorul de socket 
    pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
    int i = 0;


    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    /* utilizarea optiunii SO_REUSEADDR */
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }
    /* servim in mod concurent clientii...folosind thread-uri */
    while (1)
    {
        int client;
        thData* td; //parametru functia executata de thread     
        unsigned int length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        // client= malloc(sizeof(int));
        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        if ((client = accept(sd, (struct sockaddr*)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        /* s-a realizat conexiunea, se astepta mesajul */

          // int idThread; //id-ul threadului
          // int cl; //descriptorul intors de accept

        td = (struct thData*)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);

    }   
};

static void* treat(void* arg)
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    logare((struct thData*)arg);

    /* am terminat cu acest client, inchidem conexiunea */
    close((intptr_t)arg);
    return(NULL);
};

void* logare(void* arg)
{
    char comanda_primita_1[100];		//comanda primita de la client
    char comanda_primita_2[100];		//comanda primita de la client 
    char comanda_trimisa[1000] = "";		//mesajul trimis la client 
    char username[30] = "";   //sirul in care se stocheaza username-ul calatorului, numarul trenului sau numele statiei
    struct thData tdL;
    tdL = *((struct thData*)arg);

    printf("\n[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());

    // Logam clientul
    comanda_ok = 0;
    while (!comanda_ok)
    {
        // Citim USERNAME-UL si PAROLA de la client
        //--Citirea username-ului
        bzero(comanda_primita_1, 100);
        fflush(stdout);
        strcpy(comanda_primita_1, read_info((struct thData*)arg, 168));
        strcpy(username, comanda_primita_1);

        printf("[Thread %d]Mesajul a fost receptionat: %s\n", tdL.idThread, comanda_primita_1);

        //--Citirea parolei
        bzero(comanda_primita_2, 100);
        fflush(stdout);
        strcpy(comanda_primita_2, read_info((struct thData*)arg,184));

        printf("[Thread %d]Mesajul a fost receptionat: %s\n", tdL.idThread, comanda_primita_2);
        if (strcmp(comanda_primita_1, "exit") == 0 || strcmp(comanda_primita_2, "exit") == 0)
        {
            printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            /* am terminat cu acest client, inchidem conexiunea */
            close((intptr_t)arg);
            return(NULL);
        }
        strcpy(parola, comanda_primita_2);

        xmlDoc* document_clienti;
        xmlNode* radacina_1;
        char* fisier_1;
        fisier_1 = (char*)"./clienti.xml";

        document_clienti = xmlReadFile(fisier_1, NULL, 0);
        radacina_1 = xmlDocGetRootElement(document_clienti);
        strcpy(comanda_trimisa, "");

        utilizatorul_exista = 0;
        client_autentificat = 0;
        if (autentificare_client(radacina_1, username, parola)) //daca datele de conectare furnizate de client exista in fisierul 'clienti.xml'
        {
            strcpy(comanda_trimisa, "utilizator conectat");
            client_autentificat = 1;
            comanda_ok = 1;
            printf("\nClientul %s s-a autentificat!\n", username);
        }
        else
            strcpy(comanda_trimisa, "utilizator neconectat");

        write_info((struct thData*)arg, comanda_trimisa, 217);
        if(client_autentificat)
            client_logat((struct thData*)arg, username);
    }
    return(NULL);
}

void* client_logat(void* arg, char username[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    
    if (strstr(username, "statia"))
        client_statie((struct thData*)arg, username);
    else if (strstr(username, "tren"))
        client_tren((struct thData*)arg, username);
    else
        client_calator((struct thData*)arg, username);

    return(NULL);
}

void* client_statie(void* arg, char username[30])       // COMENZI CLIENT STATIE
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
  
    char nume_statie[30], copie_username[30];
    strcpy(copie_username, username);
    char* p = strtok(copie_username, "-");
    while (p)
    {
        strcpy(nume_statie, p); //salvam in 'nume_statie' numele statiei
        p = strtok(NULL, "-");
    }

    comanda_ok = 0;
    while (!comanda_ok)
    {
        bzero(comanda_primita_1, 100);
        fflush(stdout);
        strcpy(comanda_primita_1, read_info((struct thData*)arg,256));

        if (strcmp(comanda_primita_1, "1") == 0)
        {
            comanda_ok = 1;
            client_statie_afisare_sosiri((struct thData*)arg, username, nume_statie);
        }
        else if (strcmp(comanda_primita_1, "2") == 0)
        {
            comanda_ok = 1;
            client_statie_afisare_plecari((struct thData*)arg, username, nume_statie);
        }
        else if (strcmp(comanda_primita_1, "exit") == 0)
        {
            printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            /* am terminat cu acest client, inchidem conexiunea */
            close((intptr_t)arg);
            return(NULL);
        }
        else if (strcmp(comanda_primita_1, "logout") == 0)
        {
            printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            logare((struct thData*)arg);
        }
    }
    return(NULL);
}

void* client_statie_afisare_sosiri(void* arg, char username[30], char nume_statie[30])       // CLIENT STATIE - AFISARE SOSIRI
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    xmlDoc* document_trenuri;
    xmlNode* radacina_2;
    char* fisier_2;
    fisier_2 = (char*)"./trenuri.xml";

    document_trenuri = xmlReadFile(fisier_2, NULL, 0);
    radacina_2 = xmlDocGetRootElement(document_trenuri);

    char panou_sosiri[2000] = "";
    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_1_tren = 0; prima_data = 0;

    strcpy(panou_sosiri, afisare_sosiri(radacina_2, nume_statie, 0));
    if (strcmp(panou_sosiri, "") == 0)
        strcpy(panou_sosiri, "statie fara trenuri");
    write_info((struct thData*)arg, panou_sosiri, 303);

    int comanda_okk = 0;
    if (strstr(username, "statia"))     // daca clientul este o statie
    {
        while (!comanda_okk)
        {
            bzero(comanda_primita_1, 100);
            fflush(stdout);
            strcpy(comanda_primita_1, read_info((struct thData*)arg, 310));

            if (strcmp(comanda_primita_1, "1") == 0)
            {
                comanda_okk = 1;
                client_statie((struct thData*)arg, username);
            }
            else if (strcmp(comanda_primita_1, "2") == 0)
            {
                comanda_okk = 1;
                client_statie_afisare_sosiri((struct thData*)arg, username, nume_statie);
            }
            else if (strcmp(comanda_primita_1, "exit") == 0)
            {
                printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                /* am terminat cu acest client, inchidem conexiunea */
                close((intptr_t)arg);
                return(NULL);
            }
            else if (strcmp(comanda_primita_1, "logout") == 0)
            {
                printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                logare((struct thData*)arg);
            }
        }
    }
    else    // daca clientul este un calator
    {
        while (!comanda_okk)
        {
            bzero(comanda_primita_1, 100);
            fflush(stdout);
            strcpy(comanda_primita_1, read_info((struct thData*)arg, 310));

            if (strcmp(comanda_primita_1, "1") == 0)
            {
                comanda_okk = 1;
                client_calator((struct thData*)arg, username);
            }
            else if (strcmp(comanda_primita_1, "2") == 0)
            {
                comanda_okk = 1;
                client_statie_afisare_sosiri((struct thData*)arg, username, nume_statie);
            }
            else if (strcmp(comanda_primita_1, "exit") == 0)
            {
                printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                /* am terminat cu acest client, inchidem conexiunea */
                close((intptr_t)arg);
                return(NULL);
            }
            else if (strcmp(comanda_primita_1, "logout") == 0)
            {
                printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                logare((struct thData*)arg);
            }
        }
    }
    return(NULL);
}

void* client_statie_afisare_plecari(void* arg, char username[30], char nume_statie[30])  
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    xmlDoc* document_trenuri;
    xmlNode* radacina_2;
    char* fisier_2;
    fisier_2 = (char*)"./trenuri.xml";

    document_trenuri = xmlReadFile(fisier_2, NULL, 0);
    radacina_2 = xmlDocGetRootElement(document_trenuri);

    char panou_plecari[2000] = "";
    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_1_tren = 0;
    strcpy(panou_plecari, afisare_plecari(radacina_2, nume_statie, 0));
    if (strcmp(panou_plecari, "") == 0)
        strcpy(panou_plecari, "statie fara trenuri");
    write_info((struct thData*)arg, panou_plecari, 357);

    int comanda_okk = 0;
    if (strstr(username, "statia"))     // daca clientul este o statie
    {
        while (!comanda_okk)
        {
            bzero(comanda_primita_1, 100);
            fflush(stdout);
            strcpy(comanda_primita_1, read_info((struct thData*)arg, 364));

            if (strcmp(comanda_primita_1, "1") == 0)
            {
                comanda_okk = 1;
                client_statie((struct thData*)arg, username);
            }
            else if (strcmp(comanda_primita_1, "2") == 0)
            {
                comanda_okk = 1;
                client_statie_afisare_plecari((struct thData*)arg, username, nume_statie);
            }
            else if (strcmp(comanda_primita_1, "exit") == 0)
            {
                printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                /* am terminat cu acest client, inchidem conexiunea */
                close((intptr_t)arg);
                return(NULL);
            }
            else if (strcmp(comanda_primita_1, "logout") == 0)
            {
                printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                logare((struct thData*)arg);
            }
        }
    }
    else     // daca clientul este un calator
    {
        while (!comanda_okk)
        {
            bzero(comanda_primita_1, 100);
            fflush(stdout);
            strcpy(comanda_primita_1, read_info((struct thData*)arg, 310));

            if (strcmp(comanda_primita_1, "1") == 0)
            {
                comanda_okk = 1;
                client_calator((struct thData*)arg, username);
            }
            else if (strcmp(comanda_primita_1, "2") == 0)
            {
                comanda_okk = 1;
                client_statie_afisare_plecari((struct thData*)arg, username, nume_statie);
            }
            else if (strcmp(comanda_primita_1, "exit") == 0)
            {
                printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                /* am terminat cu acest client, inchidem conexiunea */
                close((intptr_t)arg);
                return(NULL);
            }
            else if (strcmp(comanda_primita_1, "logout") == 0)
            {
                printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
                comanda_okk = 1;
                logare((struct thData*)arg);
            }
        }
    }
    return(NULL);
}

void* client_tren(void* arg, char username[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    comanda_ok = 0;
    while (!comanda_ok)
    {
        bzero(comanda_primita_1, 100);
        fflush(stdout);
        strcpy(comanda_primita_1, read_info((struct thData*)arg,403));

        if (strcmp(comanda_primita_1, "1") == 0)
        {
            comanda_ok = 1;
            client_tren_afisare_inf_tren((struct thData*)arg, username);
        }
        else if (strcmp(comanda_primita_1, "2") == 0)
        {
            comanda_ok = 1;
            actualizare_orar_tren((struct thData*)arg, username);
        }
        else if (strcmp(comanda_primita_1, "exit") == 0)
        {
            printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            /* am terminat cu acest client, inchidem conexiunea */
            close((intptr_t)arg);
            return(NULL);
        }
        else if (strcmp(comanda_primita_1, "logout") == 0)
        {
            printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            logare((struct thData*)arg);
        }
    }
    return(NULL);    
}

void* client_tren_afisare_inf_tren(void* arg, char username[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    xmlDoc* document_trenuri;
    xmlNode* radacina_2;
    char* fisier_2;
    fisier_2 = (char*)"./trenuri.xml";

    document_trenuri = xmlReadFile(fisier_2, NULL, 0);
    radacina_2 = xmlDocGetRootElement(document_trenuri);

    char traseu_tren[2000] = "";
    tren = 0;
    strcpy(traseu_tren, informatii_tren(radacina_2, username, 0));
    
    write_info((struct thdata*)arg, traseu_tren, 449);       // trimitere inf despre traseul trenului
    
    client_tren((struct thData*)arg, username);

    return(NULL);
}

void* actualizare_orar_tren(void* arg, char username[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    char copie_comanda[40], nume_statie[30], numar_minute[10];
    int semn;   // semn = 0 -> Clientul a ajuns mai devreme in statie (-)
                // semn = 1 -> Clientul a ajuns mai tarziu in statie  (+)

    bzero(comanda_primita_1, 100);
    fflush(stdout);

    strcpy(comanda_primita_1, read_info((struct thData*)arg,468)); // primesc comanda "nume_statie+i" de la client pentru a actualiza orele de sosire
    strcpy(copie_comanda, comanda_primita_1);

    if (strstr(copie_comanda, "+") != 0)
    {
        semn = 1; // Clientul a ajuns mai tarziu in statie  (+)
        char* p = strtok(copie_comanda, "+");
        while (p)
        {
            if (!gasit_statie)
            {
                strcpy(nume_statie, p);     //salvez numele statiei
                gasit_statie = 1;
            }
            else
            {
                strcpy(numar_minute, p);    //salven nr de minute
            }
            p = strtok(NULL, "+");
        }
    }
    else if (strstr(copie_comanda, "-") != 0)
    {
        semn = 0; // Clientul a ajuns mai devreme in statie(-)
        char* p = strtok(copie_comanda, "-");
        while (p)
        {
            if (!gasit_statie)
            {
                strcpy(nume_statie, p);     //salvez numele statiei
                gasit_statie = 1;
            }
            else
                strcpy(numar_minute, p);       //salven nr de minute
            p = strtok(NULL, "-");
        }
    }
    else if (strcmp(copie_comanda, "exit") == 0)
    {
        printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
        /* am terminat cu acest client, inchidem conexiunea */
        close((intptr_t)arg);
        return(NULL);
    }
    else if (strcmp(copie_comanda, "logout") == 0)
    {
        printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
        comanda_ok = 1;
        logare((struct thData*)arg);
    }
    else if (strcmp(copie_comanda, "back") == 0)
    {
        comanda_ok = 1;
        client_tren((struct thData*)arg, username);
    }

    xmlDoc* document_trenuri;
    xmlNode* radacina_2;
    char* fisier_2;
    fisier_2 = (char*)"./trenuri.xml";

    document_trenuri = xmlReadFile(fisier_2, NULL, 0);
    radacina_2 = xmlDocGetRootElement(document_trenuri);

    char statie[1000] = "";
    tren = 0;
    strcpy(statie, verif_statie_ok_tren(radacina_2, username, nume_statie, 0));
    write_info((struct thdata*)arg, statie, 534);       // trimit raspunsul daca statia la care clientul doreste sa actualizeze
                                                   // ora de sosire exista pe itinerariul său

    if (strcmp(statie, "statie negasita") == 0) // daca statia nu a fost gasita
        actualizare_orar_tren((struct thData*)arg, username);

    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_delay = 0;
    actualizare_sosiri_tren(radacina_2, document_trenuri, username, nume_statie, numar_minute, semn); // actualizez ora pt statie trimisa de client si pentru cele urmatoare de pe itinerariul trenului

    client_tren((struct thData*)arg, username);
    return(NULL);
}

void* client_calator(void* arg, char username[30])
{    
    struct thData tdL;
    tdL = *((struct thData*)arg);

    comanda_ok = 0;
    while (!comanda_ok)
    {
        bzero(comanda_primita_1, 100);
        fflush(stdout);
        strcpy(comanda_primita_1, read_info((struct thData*)arg, 639));
        if (strstr(comanda_primita_1, "1"))
        {
            comanda_ok = 1;
            calator_afisare_sosiri((struct thData*)arg, username);
        }
        else if (strstr(comanda_primita_1, "2"))
        {
            comanda_ok = 1;
            calator_afisare_plecari((struct thData*)arg, username);
        }
        else if (strstr(comanda_primita_1, "3"))
        {
            comanda_ok = 1;

            char name_statie[30], copie_comanda[30];
            strcpy(copie_comanda, comanda_primita_1);
            char* p = strtok(copie_comanda, ".");
            while (p)
            {
                strcpy(name_statie, p);     // salvaz doar numele statiei
                p = strtok(NULL, "-");
            }
            xmlDoc* document_clienti;
            xmlNode* radacina_1;
            char* fisier_1;
            fisier_1 = (char*)"./clienti.xml";

            document_clienti = xmlReadFile(fisier_1, NULL, 0);
            radacina_1 = xmlDocGetRootElement(document_clienti);

            char raspuns[30];
            strcpy(raspuns, verif_statie(radacina_1, name_statie, 0));

            write_info((struct thdata*)arg, raspuns, 677);       // trimit raspunsul daca statia exista sau nu

            if (strcmp(raspuns, "statie negasita") == 0) // daca statia nu exista
                client_calator((struct thData*)arg, username);

            client_calator_afisare_sosiri_1h((struct thData*)arg, username, name_statie);
        }
        else if (strstr(comanda_primita_1, "4"))
        {
            comanda_ok = 1;

            char name_statie[30], copie_comanda[30];
            strcpy(copie_comanda, comanda_primita_1);
            char* p = strtok(copie_comanda, ".");
            while (p)
            {
                strcpy(name_statie, p);     // salvaz doar numele statiei
                p = strtok(NULL, "-");
            }
            xmlDoc* document_clienti;
            xmlNode* radacina_1;
            char* fisier_1;
            fisier_1 = (char*)"./clienti.xml";

            document_clienti = xmlReadFile(fisier_1, NULL, 0);
            radacina_1 = xmlDocGetRootElement(document_clienti);

            char raspuns[30];
            strcpy(raspuns, verif_statie(radacina_1, name_statie, 0));

            write_info((struct thdata*)arg, raspuns, 677);       // trimit raspunsul daca statia exista sau nu

            if (strcmp(raspuns, "statie negasita") == 0) // daca statia nu exista
                client_calator((struct thData*)arg, username);

            client_calator_afisare_plecari_1h((struct thData*)arg, username, name_statie);
        }        
        else if (strstr(comanda_primita_1, "5"))
        {
            comanda_ok = 1;
            client_calator_cauta_tren((struct thData*)arg, username, comanda_primita_1);
        }
        else if (strcmp(comanda_primita_1, "exit") == 0)
        {
            printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            /* am terminat cu acest client, inchidem conexiunea */
            close((intptr_t)arg);
            return(NULL);
        }
        else if (strcmp(comanda_primita_1, "logout") == 0)
        {
            printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
            comanda_ok = 1;
            logare((struct thData*)arg);
        }
    }
    return(NULL);
}

void* calator_afisare_sosiri(void* arg, char username[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    char name_statie[30], copie_comanda[30];
    strcpy(copie_comanda, comanda_primita_1);
    char* p = strtok(copie_comanda, ".");
    while (p)
    {
        strcpy(name_statie, p);     // salvaz doar numele statiei
        p = strtok(NULL, "-");
    }

    xmlDoc* document_clienti;
    xmlNode* radacina_1;
    char* fisier_1;
    fisier_1 = (char*)"./clienti.xml";

    document_clienti = xmlReadFile(fisier_1, NULL, 0);
    radacina_1 = xmlDocGetRootElement(document_clienti);

    char raspuns[30];
    strcpy(raspuns, verif_statie(radacina_1, name_statie, 0));

    write_info((struct thdata*)arg, raspuns, 677);       // trimit raspunsul daca statia exista sau nu

    if (strcmp(raspuns, "statie negasita") == 0) // daca statia nu a fost gasita
        client_calator((struct thData*)arg, username);

    client_statie_afisare_sosiri((struct thData*)arg, username, name_statie);

    return NULL;
}

void* calator_afisare_plecari(void* arg, char username[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    char name_statie[30], copie_comanda[30];
    strcpy(copie_comanda, comanda_primita_1);
    char* p = strtok(copie_comanda, ".");
    while (p)
    {
        strcpy(name_statie, p);     // salvaz doar numele statiei
        p = strtok(NULL, "-");
    }

    xmlDoc* document_clienti;
    xmlNode* radacina_1;
    char* fisier_1;
    fisier_1 = (char*)"./clienti.xml";

    document_clienti = xmlReadFile(fisier_1, NULL, 0);
    radacina_1 = xmlDocGetRootElement(document_clienti);

    char raspuns[30];
    strcpy(raspuns, verif_statie(radacina_1, name_statie, 0));

    write_info((struct thdata*)arg, raspuns, 677);       // trimit raspunsul daca statia exista sau nu

    if (strcmp(raspuns, "statie negasita") == 0) // daca statia nu a fost gasita
        client_calator((struct thData*)arg, username);

    client_statie_afisare_plecari((struct thData*)arg, username, name_statie);

    return NULL;
}

void* client_calator_afisare_sosiri_1h(void* arg, char username[30], char nume_statie[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    // daca statia exista atunci caut sosirile din urmatoarea ora

    // Salvez ora curenta
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    //timeinfo->tm_hour = 12;
    //timeinfo->tm_min = 0;
    rawtime = mktime(timeinfo);  // actualizează numărul de secunde de la epoca Unix

    xmlDoc* document_trenuri;
    xmlNode* radacina_3;
    char* fisier_3;
    fisier_3 = (char*)"./trenuri.xml";

    document_trenuri = xmlReadFile(fisier_3, NULL, 0);
    radacina_3 = xmlDocGetRootElement(document_trenuri);

    char panou_sosiri[2000] = "";
    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_1_tren = 0;

    strcpy(panou_sosiri, afisare_sosiri_1h(radacina_3, nume_statie, localtime(&rawtime), 0));
    if (strcmp(panou_sosiri, "") == 0)
        strcpy(panou_sosiri, "statie fara trenuri");
    write_info((struct thData*)arg, panou_sosiri, 303);

    int comanda_okk = 0;
    while (!comanda_okk)
    {
        bzero(comanda_primita_1, 100);
        fflush(stdout);
        strcpy(comanda_primita_1, read_info((struct thData*)arg, 310));

        if (strcmp(comanda_primita_1, "1") == 0)
        {
            comanda_okk = 1;
            client_calator((struct thData*)arg, username);
        }
        else if (strcmp(comanda_primita_1, "2") == 0)
        {
            comanda_okk = 1;
            client_calator_afisare_sosiri_1h((struct thData*)arg, username, nume_statie);
        }
        else if (strcmp(comanda_primita_1, "exit") == 0)
        {
            printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
            comanda_okk = 1;
            /* am terminat cu acest client, inchidem conexiunea */
            close((intptr_t)arg);
            return(NULL);
        }
        else if (strcmp(comanda_primita_1, "logout") == 0)
        {
            printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
            comanda_okk = 1;
            logare((struct thData*)arg);
        }
    }
    return NULL;
}

void* client_calator_afisare_plecari_1h(void* arg, char username[30], char nume_statie[30])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    // daca statia exista atunci caut sosirile din urmatoarea ora
    // Salvez ora curenta
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    //timeinfo->tm_hour = 6;
    //timeinfo->tm_min = 15;
    rawtime = mktime(timeinfo);  // actualizează numărul de secunde de la epoca Unix

    xmlDoc* document_trenuri;
    xmlNode* radacina_3;
    char* fisier_3;
    fisier_3 = (char*)"./trenuri.xml";

    document_trenuri = xmlReadFile(fisier_3, NULL, 0);
    radacina_3 = xmlDocGetRootElement(document_trenuri);

    char panou_sosiri[2000] = "";
    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_1_tren = 0;

    strcpy(panou_sosiri, afisare_plecari_1h(radacina_3, nume_statie, localtime(&rawtime), 0));
    if (strcmp(panou_sosiri, "") == 0)
        strcpy(panou_sosiri, "statie fara trenuri");
    write_info((struct thData*)arg, panou_sosiri, 303);

    int comanda_okk = 0;
    while (!comanda_okk)
    {
        bzero(comanda_primita_1, 100);
        fflush(stdout);
        strcpy(comanda_primita_1, read_info((struct thData*)arg, 310));

        if (strcmp(comanda_primita_1, "1") == 0)
        {
            comanda_okk = 1;
            client_calator((struct thData*)arg, username);
        }
        else if (strcmp(comanda_primita_1, "2") == 0)
        {
            comanda_okk = 1;
            client_calator_afisare_plecari_1h((struct thData*)arg, username, nume_statie);
        }
        else if (strcmp(comanda_primita_1, "exit") == 0)
        {
            printf("[Thread %d] Clientul %d s-a deconectat!\n", tdL.idThread, tdL.idThread);
            comanda_okk = 1;
            /* am terminat cu acest client, inchidem conexiunea */
            close((intptr_t)arg);
            return(NULL);
        }
        else if (strcmp(comanda_primita_1, "logout") == 0)
        {
            printf("[Thread %d] Clientul %d s-a delogat!\n", tdL.idThread, tdL.idThread);
            comanda_okk = 1;
            logare((struct thData*)arg);
        }
    }
    return NULL;
}

void* client_calator_cauta_tren(void* arg, char username[30], char nume_statii[50])
{
    struct thData tdL;
    tdL = *((struct thData*)arg);

    char statie_1[30], statie_2[30], copie_nume_statii[50], raspuns_1[30], raspuns_2[30], raspuns_final[30];
    int statie_1_gasita = 0;
    strcpy(copie_nume_statii, nume_statii + 2);

    char* p = strtok(copie_nume_statii, "-");
    while (p)
    {
        if (statie_1_gasita == 0)
        {
            strcpy(statie_1, p);     // salvaz doar numele statiei 1
            statie_1_gasita = 1;
        }
        else 
            strcpy(statie_2, p);
        p = strtok(NULL, "-");
    }

    xmlDoc* document_clienti;
    xmlNode* radacina_1;
    char* fisier_1;
    fisier_1 = (char*)"./clienti.xml";

    document_clienti = xmlReadFile(fisier_1, NULL, 0);
    radacina_1 = xmlDocGetRootElement(document_clienti);

    strcpy(raspuns_1, verif_statie(radacina_1, statie_1, 0));

    strcpy(raspuns_2, verif_statie(radacina_1, statie_2, 0));

    if (strcmp(raspuns_1, "statie gasita") == 0 && strcmp(raspuns_2, "statie gasita") == 0 && strcmp(statie_1, statie_2) != 0)
        strcpy(raspuns_final, "statii ok");
    else if (strcmp(raspuns_1, "statie negasita") == 0 && strcmp(raspuns_2, "statie gasita") == 0)
        strcpy(raspuns_final, "prima statie nu e ok");
    else if (strcmp(raspuns_1, "statie gasita") == 0 && strcmp(raspuns_2, "statie negasita") == 0)
        strcpy(raspuns_final, "a doua statie nu e ok");
    else if (strcmp(raspuns_1, "statie negasita") == 0 && strcmp(raspuns_2, "statie negasita") == 0)
        strcpy(raspuns_final, "nicio statie nu e ok");
    else if (strcmp(statie_1, statie_2) == 0)     // daca statia de start == statia de sosire
        strcpy(raspuns_final, "statii identice");

    write_info((struct thdata*)arg, raspuns_final, 677);       // trimit raspunsul daca statia exista sau nu

    if(strcmp(raspuns_final, "statii ok"))      // daca statiile introduse nu sunt corecte
        client_calator((struct thData*)arg, username);
    else     // daca statiile introduse sunt corecte
    {
        xmlDoc* document_trenuri;
        xmlNode* radacina_2;
        char* fisier_2;
        fisier_2 = (char*)"./trenuri.xml";

        document_trenuri = xmlReadFile(fisier_2, NULL, 0);
        radacina_2 = xmlDocGetRootElement(document_trenuri);

        char trenuri_gasite[2000];
        tren = 0; gasit_statie_p = 0; gasit_statie_s = 0; gasit_ora_p = 0;
        strcpy(trenuri_gasite, cauta_trenuri(radacina_2, statie_1, statie_2, 0));

        if (strcmp(trenuri_gasite, "") == 0)
            strcpy(trenuri_gasite, "niciun tren gasit");
        {
            char copie_trenuri_gasite[2000];
            strcpy(copie_trenuri_gasite, trenuri_gasite);

            strcpy(trenuri_gasite, "**************************** TRENURI ");
            strcat(trenuri_gasite, statie_1);
            strcat(trenuri_gasite, " -> ");
            strcat(trenuri_gasite, statie_2);
            strcat(trenuri_gasite, " ****************************\n");
            strcat(trenuri_gasite, "  Nr Tren     Stație plecare     Oră plecare     Stație sosire     Oră Sosire\n");
            strcat(trenuri_gasite, "******************************************************************************\n");
            strcat(trenuri_gasite, copie_trenuri_gasite);
        }

        write_info((struct thdata*)arg, trenuri_gasite, 1022);
    }
    client_calator((struct thData*)arg, username);
    return NULL;
}

const char* read_info(void* arg, int linia)
{
    bzero(comanda_primita_1, 100);
    fflush(stdout);

    struct thData tdL;
    tdL = *((struct thData*)arg);

    if (read(tdL.cl, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
        printf("    La linia %d\n", linia);
    }

    if (read(tdL.cl, comanda_primita_1, lungime_sir) <= 0)
    {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
        printf("    La linia %d\n", linia);
    }
    comanda_primita_1[lungime_sir] = '\0';
    return comanda_primita_1;
}

int write_info(void* arg, char comanda_trimisa[1000], int linia)
{
    int lungime_sir = strlen(comanda_trimisa);
    bzero(comanda_primita_1, 100);
    fflush(stdout);

    struct thData tdL;
    tdL = *((struct thData*)arg);

    //trimite prima data lungimea mesajului la client si apoi mesajul propriu zis
    if (write(tdL.cl, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        printf("    La linia %d\n", linia);
    }

    // trimitem mesajul la client
    if (write(tdL.cl, comanda_trimisa, lungime_sir) <= 0)
    {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        printf("    La linia %d\n", linia);
        return 0;
    }
    else
    {
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
        return 1;
    }
}

int autentificare_client(xmlNode* radacina, char username[30], char parola[30])
{
    xmlNode* nod = NULL;
    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "username") == 0 && strcmp((const char*)xmlNodeGetContent(nod), username) == 0)
                    utilizatorul_exista = 1;
                if (strcmp((const char*)nod->name, "parola") == 0 && strcmp((const char*)xmlNodeGetContent(nod), parola) == 0 && utilizatorul_exista == 1)
                    client_autentificat = 1;
            }
        }
        autentificare_client(nod->children, username, parola);
    }
    return client_autentificat;
}

const char* afisare_sosiri(xmlNode* radacina, char nume_statie[30], int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
        strcpy(comanda_trimisa, "");

    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0)
                {
                    strcpy(numar_tren, (const char*)xmlNodeGetContent(nod));
                    tren = 1;
                }
                else if (strcmp((const char*)nod->name, "statie_plecare") == 0 && tren == 1)
                {
                    strcpy(statie_p, (const char*)xmlNodeGetContent(nod));
                    strcpy(statie_p1, (const char*)xmlNodeGetContent(nod));
                    while (strlen(statie_p) < 14)
                    {
                        strcat(statie_p, " ");
                    }
                }
                else if (strcmp((const char*)nod->name, "statie_sosire") == 0 && tren == 1)
                {
                    strcpy(statie_s, (const char*)xmlNodeGetContent(nod));
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_statie) == 0 &&
                    tren == 1)
                {
                    if (strcmp(statie_p1, (const char*)xmlNodeGetContent(nod)) != 0)
                        gasit_statie = 1;
                }
                else if (strcmp((const char*)nod->name, "soseste_la") == 0 &&
                    tren == 1 && gasit_statie == 1)
                {
                    strcpy(ora_s, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 1;
                }
                else if (strcmp((const char*)nod->name, "intarziere_sosire") == 0 &&
                    tren == 1 && gasit_statie == 1 && gasit_ora == 1)
                {
                    strcpy(delay, (const char*)xmlNodeGetContent(nod));
                    gasit_delay = 1;
                }

                if (gasit_1_tren == 0 && gasit_statie == 1)
                    gasit_1_tren = 1;

                if (tren == 1 && gasit_statie == 1 && gasit_ora == 1 && gasit_delay == 1)
                {
                    strcat(comanda_trimisa, "  ");
                    strcat(comanda_trimisa, ora_s);
                    strcat(comanda_trimisa, "       ");

                    if (strcmp(delay, "00:00") == 0 || strcmp(delay, ora_s) == 0) // cand nu exista delay
                        strcat(comanda_trimisa, "LA TIMP");
                    else
                    {
                        strcat(comanda_trimisa, " ");
                        strcat(comanda_trimisa, delay);
                        strcat(comanda_trimisa, " ");
                    }

                    strcat(comanda_trimisa, "       ");
                    strcat(comanda_trimisa, statie_p);
                    strcat(comanda_trimisa, "   ");
                    strcat(comanda_trimisa, numar_tren);
                    strcat(comanda_trimisa, "      ");
                    strcat(comanda_trimisa, statie_p1);
                    strcat(comanda_trimisa, "->");
                    strcat(comanda_trimisa, statie_s);
                    strcat(comanda_trimisa, "\n");
                    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_delay = 0;

                    lungime_sir = strlen(comanda_trimisa);
                    prima_data = 1;
                    strcpy(ora_anterioara, ora_s);
                    
                }
            }
        }
        afisare_sosiri(nod->children, nume_statie, 1);
    }
    return comanda_trimisa;
}

const char* afisare_sosiri_1h(xmlNode* radacina, char nume_statie[30], struct tm* timeinfo, int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
    {
        strcpy(comanda_trimisa, "");
        printf("Ora curentă este: %02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min);
    }

    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0)
                {
                    strcpy(numar_tren, (const char*)xmlNodeGetContent(nod));
                    tren = 1;
                }
                else if (strcmp((const char*)nod->name, "statie_plecare") == 0 && tren == 1)
                {
                    strcpy(statie_p, (const char*)xmlNodeGetContent(nod));
                    strcpy(statie_p1, (const char*)xmlNodeGetContent(nod));
                    while (strlen(statie_p) < 14)
                        strcat(statie_p, " ");
                }
                else if (strcmp((const char*)nod->name, "statie_sosire") == 0 && tren == 1)
                    strcpy(statie_s, (const char*)xmlNodeGetContent(nod));
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_statie) == 0 &&
                    tren == 1)
                {
                    if (strcmp(statie_p1, (const char*)xmlNodeGetContent(nod)) != 0)
                        gasit_statie = 1;
                }
                else if (strcmp((const char*)nod->name, "soseste_la") == 0 &&
                    tren == 1 && gasit_statie == 1)
                {
                    strcpy(ora_s, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 1;
                }
                else if (strcmp((const char*)nod->name, "intarziere_sosire") == 0 &&
                    tren == 1 && gasit_statie == 1 && gasit_ora == 1)
                {
                    strcpy(delay, (const char*)xmlNodeGetContent(nod));

                    struct tm timeinfo_1;

                    if (strcmp(delay, "00:00") == 0 || strcmp(delay, ora_s) == 0)    // nu exista delay
                        strptime(ora_s, "%H:%M", &timeinfo_1);
                    else
                        strptime(delay, "%H:%M", &timeinfo_1);

                    if (timeinfo->tm_hour - timeinfo_1.tm_hour <= 1)
                    {
                        if (timeinfo_1.tm_hour - timeinfo->tm_hour == 0 && timeinfo_1.tm_min - timeinfo->tm_min >= 0)
                            gasit_delay = 1;
                        else if (timeinfo_1.tm_hour - timeinfo->tm_hour == 1 && timeinfo_1.tm_min - timeinfo->tm_min <= 0 && timeinfo_1.tm_min - timeinfo->tm_min >= -59)
                            gasit_delay = 1;
                        else
                        {
                            tren = 0;
                            gasit_statie = 0;
                            gasit_ora = 0;
                            gasit_delay = 0;
                        }
                    } 
                    else
                    {
                        tren = 0;
                        gasit_statie = 0;
                        gasit_ora = 0;
                        gasit_delay = 0;
                    }
                }

                if (gasit_1_tren == 0 && gasit_statie == 1)
                    gasit_1_tren = 1;

                if (tren == 1 && gasit_statie == 1 && gasit_ora == 1 && gasit_delay == 1)
                {
                    strcat(comanda_trimisa, "  ");
                    strcat(comanda_trimisa, ora_s);
                    strcat(comanda_trimisa, "       ");

                    if (strcmp(delay, "00:00") == 0 || strcmp(delay, ora_s) == 0) // cand nu exista delay
                        strcat(comanda_trimisa, "LA TIMP");
                    else
                    {
                        strcat(comanda_trimisa, " ");
                        strcat(comanda_trimisa, delay);
                        strcat(comanda_trimisa, " ");
                    }

                    strcat(comanda_trimisa, "       ");
                    strcat(comanda_trimisa, statie_p);
                    strcat(comanda_trimisa, "   ");
                    strcat(comanda_trimisa, numar_tren);
                    strcat(comanda_trimisa, "      ");
                    strcat(comanda_trimisa, statie_p1);
                    strcat(comanda_trimisa, "->");
                    strcat(comanda_trimisa, statie_s);
                    strcat(comanda_trimisa, "\n");
                    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_delay = 0;

                    lungime_sir = strlen(comanda_trimisa);
                }
            }
        }
        afisare_sosiri_1h(nod->children, nume_statie, timeinfo, 1);
    }
    return comanda_trimisa;
}

const char* afisare_plecari_1h(xmlNode* radacina, char nume_statie[30], struct tm* timeinfo, int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
    {
        strcpy(comanda_trimisa, "");
        printf("Ora curentă este: %02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min);
    }

    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0)
                {
                    strcpy(numar_tren, (const char*)xmlNodeGetContent(nod));
                    tren = 1;
                }
                else if (strcmp((const char*)nod->name, "statie_plecare") == 0 && tren == 1)
                    strcpy(statie_p, (const char*)xmlNodeGetContent(nod));
                else if (strcmp((const char*)nod->name, "statie_sosire") == 0 && tren == 1)
                    strcpy(statie_s, (const char*)xmlNodeGetContent(nod));
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_statie) == 0 &&
                    tren == 1)
                {
                    if (strcmp(statie_s, (const char*)xmlNodeGetContent(nod)) != 0)
                        gasit_statie = 1;
                }
                else if (strcmp((const char*)nod->name, "pleaca_la") == 0 &&
                    tren == 1 && gasit_statie == 1)
                {
                    strcpy(ora_s, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 1;
                }
                else if (strcmp((const char*)nod->name, "intarziere_plecare") == 0 &&
                    tren == 1 && gasit_statie == 1 && gasit_ora == 1)
                {
                    strcpy(delay, (const char*)xmlNodeGetContent(nod));
                    struct tm timeinfo_1;

                    if (strcmp(delay, "00:00") == 0 || strcmp(delay, ora_s) == 0)    // nu exista delay
                        strptime(ora_s, "%H:%M", &timeinfo_1);
                    else
                        strptime(delay, "%H:%M", &timeinfo_1);

                    if (timeinfo->tm_hour - timeinfo_1.tm_hour <= 1)
                    {
                        if (timeinfo_1.tm_hour - timeinfo->tm_hour == 0 && timeinfo_1.tm_min - timeinfo->tm_min >= 0)
                            gasit_delay = 1;
                        else if (timeinfo_1.tm_hour - timeinfo->tm_hour == 1 && timeinfo_1.tm_min - timeinfo->tm_min <= 0 && timeinfo_1.tm_min - timeinfo->tm_min >= -59)
                            gasit_delay = 1;
                        else
                        {
                            tren = 0;
                            gasit_statie = 0;
                            gasit_ora = 0;
                            gasit_delay = 0;
                        }
                    }
                    else
                    {
                        tren = 0;
                        gasit_statie = 0;
                        gasit_ora = 0;
                        gasit_delay = 0;
                    }
                }

                if (gasit_1_tren == 0 && gasit_statie == 1)
                    gasit_1_tren = 1;

                if (tren == 1 && gasit_statie == 1 && gasit_ora == 1 && gasit_delay == 1)
                {
                    strcat(comanda_trimisa, "  ");
                    strcat(comanda_trimisa, ora_s);
                    strcat(comanda_trimisa, "       ");

                    if (strcmp(delay, "00:00") == 0 || strcmp(delay, ora_s) == 0) // cand nu exista delay
                        strcat(comanda_trimisa, "LA TIMP");
                    else
                    {
                        strcat(comanda_trimisa, " ");
                        strcat(comanda_trimisa, delay);
                        strcat(comanda_trimisa, " ");
                    }

                    char statie_s1[15];
                    strcpy(statie_s1, statie_s);
                    while (strlen(statie_s1) < 14)
                        strcat(statie_s1, " ");

                    strcat(comanda_trimisa, "       ");
                    strcat(comanda_trimisa, statie_s1);
                    strcat(comanda_trimisa, "   ");
                    strcat(comanda_trimisa, numar_tren);
                    strcat(comanda_trimisa, "      ");
                    strcat(comanda_trimisa, statie_p);
                    strcat(comanda_trimisa, "->");
                    strcat(comanda_trimisa, statie_s);
                    strcat(comanda_trimisa, "\n");
                    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_delay = 0;

                    lungime_sir = strlen(comanda_trimisa);
                }
            }
        }
        afisare_plecari_1h(nod->children, nume_statie, timeinfo, 1);
    }
    return comanda_trimisa;
}

const char* afisare_plecari(xmlNode* radacina, char nume_statie[30], int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
        strcpy(comanda_trimisa, "");
    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0)
                {
                    strcpy(numar_tren, (const char*)xmlNodeGetContent(nod));
                    tren = 1;
                }
                else if (strcmp((const char*)nod->name, "statie_plecare") == 0 && tren == 1)
                {
                    strcpy(statie_p, (const char*)xmlNodeGetContent(nod));
                    strcpy(statie_p1, (const char*)xmlNodeGetContent(nod));
                    while (strlen(statie_p) < 14)
                    {
                        strcat(statie_p, " ");
                    }
                }
                else if (strcmp((const char*)nod->name, "statie_sosire") == 0 && tren == 1)
                {
                    strcpy(statie_s, (const char*)xmlNodeGetContent(nod));
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_statie) == 0 &&
                    tren == 1)
                {
                    if (strcmp(statie_s, (const char*)xmlNodeGetContent(nod)) != 0)
                        gasit_statie = 1;
                }
                else if (strcmp((const char*)nod->name, "pleaca_la") == 0 &&
                    tren == 1 && gasit_statie == 1)
                {
                    strcpy(ora_s, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 1;
                }
                else if (strcmp((const char*)nod->name, "intarziere_plecare") == 0 &&
                    tren == 1 && gasit_statie == 1 && gasit_ora == 1)
                {
                    strcpy(delay, (const char*)xmlNodeGetContent(nod));
                    gasit_delay = 1;
                }

                if (gasit_1_tren == 0 && gasit_statie == 1)
                    gasit_1_tren = 1;

                if (tren == 1 && gasit_statie == 1 && gasit_ora == 1 && gasit_delay == 1)
                {
                    strcat(comanda_trimisa, "  ");
                    strcat(comanda_trimisa, ora_s);
                    strcat(comanda_trimisa, "       ");

                    if (strcmp(delay, "00:00") == 0 || strcmp(delay, ora_s) == 0) // cand nu exista delay
                        strcat(comanda_trimisa, "LA TIMP");
                    else
                    {
                        strcat(comanda_trimisa, " ");
                        strcat(comanda_trimisa, delay);
                        strcat(comanda_trimisa, " ");
                    }

                    strcat(comanda_trimisa, "       ");

                    char copie_statie_s[30];
                    strcpy(copie_statie_s, statie_s);
                    while (strlen(copie_statie_s) < 14)
                    {
                        strcat(copie_statie_s, " ");
                    }
                    strcat(comanda_trimisa, copie_statie_s);
                    strcat(comanda_trimisa, "   ");
                    strcat(comanda_trimisa, numar_tren);
                    strcat(comanda_trimisa, "      ");
                    strcat(comanda_trimisa, statie_p1);
                    strcat(comanda_trimisa, "->");
                    strcat(comanda_trimisa, statie_s);
                    strcat(comanda_trimisa, "\n");
                    tren = 0; gasit_statie = 0; gasit_ora = 0; gasit_delay = 0;

                    lungime_sir = strlen(comanda_trimisa);
                }
            }
        }
        afisare_plecari(nod->children, nume_statie, 1);
    }
    return comanda_trimisa;
}

const char* verif_statie(xmlNode* radacina, char nume_statie[30], int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
        strcpy(comanda_trimisa, "statie negasita");

    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "username") == 0 &&
                    strstr((const char*)xmlNodeGetContent(nod), "statia") != 0)
                {
                    char nod_content[30], name_statie[30];
                    strcpy(nod_content, (const char*)xmlNodeGetContent(nod));

                    char* p = strtok(nod_content, "-");
                    while (p)
                    {
                        strcpy(name_statie, p); //salvam in 'nume_statie' numele statiei
                        p = strtok(NULL, "-");
                    }
                    
                    if (strcmp(name_statie, nume_statie) == 0)
                        strcpy(comanda_trimisa, "statie gasita");
                }
            }
        }
        verif_statie(nod->children, nume_statie, 1);
    }
    return comanda_trimisa;
}

const char* verif_statie_ok_tren(xmlNode* radacina, char nume_tren[30], char nume_statie[30], int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
        strcpy(comanda_trimisa, "statie negasita");

    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_tren) == 0)
                {
                    tren = 1;
                }
                else if (strcmp((const char*)nod->name, "nume_tren") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_tren) != 0)
                {
                    tren = 0;
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 && 
                        strcmp((const char*)xmlNodeGetContent(nod), nume_statie) == 0 &&
                        tren == 1)
                {
                    strcpy(comanda_trimisa, "statie gasita");
                    tren = 0;
                }
            }
        }
        verif_statie_ok_tren(nod->children, nume_tren, nume_statie, 1);
    }
    return comanda_trimisa;
}

const char* actualizare_sosiri_tren(xmlNode* radacina_2, xmlDoc* document, char nume_tren[30], char nume_statie[40], char numar_minute[10], int semn)
{
    xmlNode* nod = NULL;

    for (nod = radacina_2; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_tren) == 0)
                {
                    tren = 1;
                }
                else if (strcmp((const char*)nod->name, "nume_tren") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_tren) != 0)
                {
                    tren = 0; gasit_statie = 0;
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_statie) == 0 &&
                    tren == 1)
                {
                    gasit_statie = 1;
                }
                else if (strcmp((const char*)nod->name, "soseste_la") == 0 && tren == 1 && gasit_statie == 1)
                {   // MODIFIC ORA DE SOSIRE
                    strcpy(ora_sosire, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 1;
                }
                else if (strcmp((const char*)nod->name, "intarziere_sosire") == 0 && tren == 1 && gasit_statie == 1 && gasit_ora == 1)
                {   // MODIFIC ORA DE SOSIRE
                    if (strcmp((const char*)xmlNodeGetContent(nod), "00:00") != 0)  //daca a mai avut loc deja o actualizare
                        strcpy(ora_sosire, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 0;

                    int gasit_ora_1 = 0; 

                    char* p = strtok(ora_sosire, ":");
                    while (p)
                    {
                        if (!gasit_ora_1)
                        {
                            strcpy(char_ora, p);     //salvez ora
                            gasit_ora_1 = 1;
                        }
                        else
                            strcpy(char_minute, p);    //salven nr de minute
                        p = strtok(NULL, ":");
                    }

                    int hour = atoi(char_ora);
                    int minute = atoi(char_minute);
                    int min_intarziere= atoi(numar_minute);

                    // Cream o structura tm
                    struct tm timeinfo = { 0 };
                    timeinfo.tm_hour = hour;
                    timeinfo.tm_min = minute;

                    if (semn == 0) // Trenul a ajuns mai devreme in statie(-)
                        timeinfo.tm_min -= min_intarziere;
                    else    // Trenul a ajuns mai tarziu in statie(+)
                        timeinfo.tm_min += min_intarziere;

                    // Convertim structura tm in formatul timestamp Unix
                    //time_t rawtime = mktime(&timeinfo);

                    // Afisam data de timp in formatul "HH:MM"
                    char ora_finala[10];
                    strftime(ora_finala, 10, "%H:%M", &timeinfo);

                    strcpy(comanda_trimisa, ora_finala);

                    pthread_mutex_lock(&mlock);  // **

                    xmlNodeSetContent(nod, (const xmlChar*)comanda_trimisa);
                    xmlSaveFile("./trenuri.xml", document);

                    pthread_mutex_unlock(&mlock);  // **
                }
                else if (strcmp((const char*)nod->name, "pleaca_la") == 0 && tren == 1 && gasit_statie == 1)
                {   // MODIFIC ORA DE PLECARE
                    strcpy(ora_sosire, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 1;
                }
                else if (strcmp((const char*)nod->name, "intarziere_plecare") == 0 && tren == 1 && gasit_statie == 1 && gasit_ora == 1)
                {   // MODIFIC ORA DE PLECARE
                    if (strcmp((const char*)xmlNodeGetContent(nod), "00:00") != 0)  //daca a mai avut loc deja o actualizare
                        strcpy(ora_sosire, (const char*)xmlNodeGetContent(nod));
                    gasit_ora = 0;

                    int gasit_ora_1 = 0;

                    char* p = strtok(ora_sosire, ":");
                    while (p)
                    {
                        if (!gasit_ora_1)
                        {
                            strcpy(char_ora, p);     //salvez ora
                            gasit_ora_1 = 1;
                        }
                        else
                            strcpy(char_minute, p);    //salven nr de minute
                        p = strtok(NULL, ":");
                    }

                    int hour = atoi(char_ora);
                    int minute = atoi(char_minute);
                    int min_intarziere = atoi(numar_minute);

                    // Cream o structura tm
                    struct tm timeinfo = { 0 };
                    timeinfo.tm_hour = hour;
                    timeinfo.tm_min = minute;

                    if (semn == 0) // Clientul a ajuns mai devreme in statie(-)
                        timeinfo.tm_min -= min_intarziere;
                    else    // Clientul a ajuns mai tarziu in statie(+)
                        timeinfo.tm_min += min_intarziere;

                    // Convertim structura tm in formatul timestamp Unix
                    //time_t rawtime = mktime(&timeinfo);

                    // Afisam data de timp in formatul "HH:MM"
                    char ora_finala[10];
                    strftime(ora_finala, 10, "%H:%M", &timeinfo);

                    strcpy(comanda_trimisa, ora_finala);

                    pthread_mutex_lock(&mlock);  // **

                    xmlNodeSetContent(nod, (const xmlChar*)comanda_trimisa);
                    xmlSaveFile("./trenuri.xml", document);

                    pthread_mutex_unlock(&mlock);  // **
                }
            }
        }
        actualizare_sosiri_tren(nod->children, document, nume_tren, nume_statie, numar_minute, semn);
    }
    return 0;
}

const char* informatii_tren(xmlNode* radacina, char nume_tren[30], int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
    {
        strcpy(comanda_trimisa, "");
        prim = 1;
    }
    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_tren) == 0)
                {
                    char numar_tren[10], copie_nume_tren[20];
                    strcpy(copie_nume_tren, nume_tren);
                    char* p = strtok(copie_nume_tren, "-");
                    while (p)
                    {
                        strcpy(numar_tren, p); //salvam in 'nume_numar' numarul trenului
                        p = strtok(NULL, "-");
                    }
                    tren = 1;
                    strcat(comanda_trimisa, "Număr tren: ");
                    strcat(comanda_trimisa, numar_tren);
                }
                else if (strcmp((const char*)nod->name, "statie_plecare") == 0 && tren == 1)
                {
                    
                    strcat(comanda_trimisa, "\nDirecția: ");
                    strcat(comanda_trimisa, (const char*)xmlNodeGetContent(nod));
                    strcat(comanda_trimisa, "->");
                }
                else if (strcmp((const char*)nod->name, "statie_sosire") == 0 && tren == 1)
                {
                    strcat(comanda_trimisa, (const char*)xmlNodeGetContent(nod));
                    strcat(comanda_trimisa, "\nItinerariu:   **************************************************");
                    strcat(comanda_trimisa, "\n              *  Ora Sosire      Stația           Ora Plecare  *");
                    strcat(comanda_trimisa, "\n              **************************************************");
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 && tren == 1)
                {
                    strcpy(nume_statie_c, (const char*)xmlNodeGetContent(nod));
                    while (strlen(nume_statie_c) < 21)
                        strcat(nume_statie_c, " ");
                }
                else if (strcmp((const char*)nod->name, "soseste_la") == 0 && tren == 1)
                {
                    strcat(comanda_trimisa, "\n              *    ");
                    strcat(comanda_trimisa, (const char*)xmlNodeGetContent(nod));
                    strcat(comanda_trimisa, "         ");
                    strcat(comanda_trimisa, nume_statie_c);
                }
                else if (strcmp((const char*)nod->name, "pleaca_la") == 0 && tren == 1)
                {
                    //strcat(comanda_trimisa, "       ");
                    strcat(comanda_trimisa, (const char*)xmlNodeGetContent(nod));
                    strcat(comanda_trimisa, "    *");
                }
                if (tren == 1 && strcmp((const char*)nod->name, "nume_tren") == 0 &&
                    strcmp((const char*)xmlNodeGetContent(nod), nume_tren) != 0)
                {
                    tren = 0; 
                    strcat(comanda_trimisa, "\n              **************************************************");
                    strcat(comanda_trimisa, "\n");
                }
            }
        }
        informatii_tren(nod->children, nume_tren, 1);
    }
    return comanda_trimisa;
}

const char* cauta_trenuri(xmlNode* radacina, char statie_1[30], char statie_2[30], int prim)
{
    xmlNode* nod = NULL;
    if (prim == 0)
    {
        strcpy(comanda_trimisa, "");
        prim = 1;
    }
    for (nod = radacina; nod; nod = nod->next)
    {
        if (nod->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(nod) == 0)
            {
                if (strcmp((const char*)nod->name, "nume_tren") == 0)
                {
                    strcpy(numar_tren, (const char*)xmlNodeGetContent(nod));
                    tren = 1; gasit_statie_p = 0; gasit_ora_p = 0; gasit_statie_s = 0;
                    strcpy(ora_p, "");
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 && 
                    strcmp((const char*)xmlNodeGetContent(nod), statie_1) == 0 && tren == 1)
                {
                    gasit_statie_p = 1;
                    tren = 0;
                }
                else if (strcmp((const char*)nod->name, "pleaca_la") == 0 && gasit_statie_p == 1)
                {
                    if (strcmp((const char*)xmlNodeGetContent(nod), "") != 0)
                    {
                        strcpy(ora_p, (const char*)xmlNodeGetContent(nod));
                        gasit_ora_p = 1;
                        gasit_statie_p = 0;
                    }
                    else
                        gasit_statie_p = 0;
                }
                else if (strcmp((const char*)nod->name, "nume_statie") == 0 && 
                    strcmp((const char*)xmlNodeGetContent(nod), statie_2) == 0 && gasit_ora_p == 1)
                {
                    gasit_statie_s = 1;
                    gasit_ora_p = 0;
                }
                else if (strcmp((const char*)nod->name, "soseste_la") == 0 && gasit_statie_s == 1 && strcmp(ora_p,"") != 0)
                {
                    gasit_statie_s = 0;
                    char copie_statie_1[50], copie_statie_2[50];
                    strcpy(copie_statie_1, statie_1);
                    strcpy(copie_statie_2, statie_2);

                    while (strlen(copie_statie_1) < 15)
                        strcat(copie_statie_1, " ");
                    while (strlen(copie_statie_2) < 15)
                        strcat(copie_statie_2, " ");

                    strcat(comanda_trimisa, " ");
                    strcat(comanda_trimisa, numar_tren);
                    strcat(comanda_trimisa, "        ");
                    strcat(comanda_trimisa, copie_statie_1);
                    strcat(comanda_trimisa, "   ");
                    strcat(comanda_trimisa, ora_p);
                    strcat(comanda_trimisa, "          ");
                    strcat(comanda_trimisa, copie_statie_2);
                    strcat(comanda_trimisa, "   ");
                    strcat(comanda_trimisa, (const char*)xmlNodeGetContent(nod));
                    strcat(comanda_trimisa, "\n");

                }
            }
        }
        cauta_trenuri(nod->children, statie_1, statie_2, 1);
    }
    return comanda_trimisa;
}

