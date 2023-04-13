#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <cstring>
#include <string>
#include <cstdlib>

using namespace std;

/* codul de eroare returnat de anumite apeluri */
extern int errno;
struct sockaddr_in server;
char comanda_primita[2000]; //mesajul primit de la server
char comanda_trimisa_1[100], comanda_trimisa_2[100];  // mesajul trimis către server
char username[30], itinerariu_tren[1000];
int lungime_sir, client_autentificat, comanda_ok;
int sd;			// descriptorul de socket
int port;   // portul de conectare la server

int logare(int sd);
int securizare_parola(char password[30]);
int client_logat(int sd, char username[30]);
int client_statie(int sd, char username[30]);
int client_statie_afisare_sosiri(int sd, char username[30], char nume_statie[30]);
int client_statie_afisare_plecari(int sd, char username[30], char nume_statie[30]);
int client_tren(int sd, char username[30]);
int informatii_tren(int sd, char username[30]);
int actualizare_orar_tren(int sd, char username[30]);
int client_calator(int sd, char username[30]);
int client_calator_afisare_sosiri_1h(int sd, char username[30], char nume_statie[30]);
int client_calator_afisare_plecari_1h(int sd, char username[30], char nume_statie[30]);
int client_calator_cauta_tren(int sd, char username[30]);
int write_to_server(int sd, char* comanda, int lungime);

int main(int argc, char* argv[])
{
    struct sockaddr_in server;	// structura folosita pentru conectare 
    // mesajul trimis
    char* buf[10];

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);  // stabilim portul 

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    server.sin_family = AF_INET;       // familia socket-ului 
    server.sin_addr.s_addr = inet_addr(argv[1]);    // adresa IP a serverului 
    server.sin_port = htons(port);     // portul de conectare 

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  // cream socketul 
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    printf("!! Pentru a părăsi aplicația poți tasta oricând comanda 'exit'!\n");
    logare(sd);

    /* inchidem conexiunea, am terminat */
    close(sd);
}

int logare(int sd)
{
    char comanda_trimisa_1[100];  // mesajul trimis către server
    char comanda_trimisa_2[100];  // mesajul trimis către server
    char comanda_primita[1000]; //mesajul primit de la server
    int lungime_sir, to_exit = 0, parola_criptata_int = 0;

    // -------- LOGAREA CLIENTULUI --------

    printf("\nLOGARE CERUTĂ! \n");
    client_autentificat = 0;
    while (!client_autentificat)
    {
        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Introduceți username-ul: ");
        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strcmp(comanda_trimisa_1, "exit"))  // comanda != `exit`
        {
            strcpy(username, comanda_trimisa_1);

            bzero(comanda_trimisa_2, 100);    // citirea mesajului 
            fflush(stdout);
            printf("Introduceți parola: ");
            fflush(stdout);
            read(0, comanda_trimisa_2, 100);
            comanda_trimisa_2[strlen(comanda_trimisa_2) - 1] = '\0';

            if (strcmp(comanda_trimisa_2, "") == 0)     // daca clientul apasa enter 
                strcpy(comanda_trimisa_2, ".");

            to_exit = 0;
            if (strcmp(comanda_trimisa_2, "exit") == 0)
                to_exit = 1;
            else
                parola_criptata_int = securizare_parola(comanda_trimisa_2);
        }
        else
        {
            to_exit = 1;
            strcpy(comanda_trimisa_2, "exit");
        }
        if (!to_exit)
        {
            sprintf(comanda_trimisa_2, "%d", parola_criptata_int);
        }

        if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
            strcpy(comanda_trimisa_1, ".");
        //trimitem mesajul la server
        write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

        write_to_server(sd, comanda_trimisa_2, strlen(comanda_trimisa_2));
        if (strcmp(comanda_trimisa_1, "exit") == 0 || strcmp(comanda_trimisa_2, "exit") == 0)
            exit(0);

        /* citirea raspunsului dat de server
            (apel blocant pina cind serverul raspunde) */
        if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
        {
            perror("Eroare la read() de la client. Linia 111\n");
            return errno;
        }

        if (read(sd, comanda_primita, lungime_sir) <= 0)
        {
            perror("Eroare la read() de la client. Linia 112\n");
            return errno;
        }
        comanda_primita[lungime_sir] = '\0';

        /* afisam mesajul primit */

        if (strcmp(comanda_primita, "utilizator conectat") == 0)   //daca utilizatorul exista
        {
            printf("Te-ai conectat cu succes!\n");
            client_autentificat = 1;

            client_logat(sd, username);
        }
        else
        {
            printf("Datele de logare nu sunt corecte! Mai incearca o data!\n");
            printf("\n");
        }

    }
    return 0;
}

int client_logat(int sd, char username[30])
{
    if (strstr(username, "statia"))
        client_statie(sd, username);
    else if (strstr(username, "tren"))
        client_tren(sd, username);
    else
        client_calator(sd, username);

    return 0;
}

int client_statie(int sd, char username[30])
{
    comanda_ok = 0;
    while (!comanda_ok)
    {
     printf("\n            MENIU - `STAȚIE`:\n");
        printf("Alegeți ce informații doriți să primiți:\n");
        printf("1. Panoul cu sosori.\n");
        printf("2. Panoul pentru plecări.\n");
        printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
        printf("4. Dacă doriți să părăsiți aplicația!\n");

        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strcmp(comanda_trimisa_1, "3") == 0)
            strcpy(comanda_trimisa_1, "logout");
        else if (strcmp(comanda_trimisa_1, "4") == 0)
            strcpy(comanda_trimisa_1, "exit");

        if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
            strcpy(comanda_trimisa_1, ".");

        write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

        if (strcmp(comanda_trimisa_1, "1") == 0)
        {
            comanda_ok = 1;

            char name_statie[30], username_[30];
            strcpy(username_, username);
            char* p = strtok(username_, "-");
            while (p)
            {
                strcpy(name_statie, p);     // salvaz doar numele statiei
                p = strtok(NULL, "-");
            }

            client_statie_afisare_sosiri(sd, username, name_statie);
        }
        else if (strcmp(comanda_trimisa_1, "2") == 0)
        {
            comanda_ok = 1;

            char name_statie[30], username_[30];
            strcpy(username_, username);
            char* p = strtok(username_, "-");
            while (p)
            {
                strcpy(name_statie, p);     // salvaz doar numele statiei
                p = strtok(NULL, "-");
            }

            client_statie_afisare_plecari(sd, username, name_statie);
        }
        else if (strcmp(comanda_trimisa_1, "exit") == 0)
        {
            comanda_ok = 1;
            exit(3);
        }
        else if (strcmp(comanda_trimisa_1, "logout") == 0)
        {
            comanda_ok = 1;
            logare(sd);
        }
        else
        {
            printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
            printf("\n");
        }
    }
    return 0;
}

int client_statie_afisare_sosiri(int sd, char username[30], char nume_statie[30])
{
    char copie_nume_statie[30];
    strcpy(copie_nume_statie, nume_statie);
    
    // VREA PANOUL PT SOSIRI 
    bzero(comanda_primita, sizeof(comanda_primita));
    comanda_primita[0] = '\0';
    if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        perror("Eroare la read() de la client. Linia 239\n");
        return errno;
    }

    if (read(sd, comanda_primita, lungime_sir) <= 0)
    {
        perror("Eroare la read() de la client. Linia 248\n");
        return errno;
    }
    comanda_primita[lungime_sir] = '\0';

    if (strcmp(comanda_primita, "statie fara trenuri") == 0) // daca statia nu este tranzitata de trenuri la sosiri
        printf("Nu există niciun tren care va sosi în %s!\n", username);
    else
    {
        for (int i = 0; i < strlen(nume_statie); i++)
            nume_statie[i] = toupper(nume_statie[i]);

        while (strlen(nume_statie) < 14)
            strcat(nume_statie, "*");

        printf("\n***************************PANOU SOSIRI - %s**************************\n", nume_statie);
        printf("Sosește la     Delay       De la            Trenul Nr.        Direcția\n");
        printf("**********************************************************************************\n");
        printf("%s\n", comanda_primita);
    }

    int comanda_ok = 0;
    if (strstr(username, "-"))    // daca este client statie
    {
        while (!comanda_ok)
        {
            printf("\n            MENIU - `SORIRI-STAȚIE`:\n");
            printf("Alegeți următoarea comandă:\n");
            printf("1. Pentru a reveni la meniul comenzilor unei stații\n");
            printf("2. Pentru a reactualiza panoul pentru sosiri\n");
            printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
            printf("4. Dacă doriți să părăsiți aplicația!\n");
            bzero(comanda_trimisa_1, 100);
            fflush(stdout);
            printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

            fflush(stdout);
            read(0, comanda_trimisa_1, 100);
            comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

            if (strcmp(comanda_trimisa_1, "3") == 0)
                strcpy(comanda_trimisa_1, "logout");
            else if (strcmp(comanda_trimisa_1, "4") == 0)
                strcpy(comanda_trimisa_1, "exit");

            if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
                strcpy(comanda_trimisa_1, ".");

            write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

            if (strcmp(comanda_trimisa_1, "1") == 0)
            {
                comanda_ok = 1;
                client_statie(sd, username);
            }
            else if (strcmp(comanda_trimisa_1, "2") == 0)
            {
                comanda_ok = 1;
                client_statie_afisare_sosiri(sd, username, copie_nume_statie);
            }
            else if (strcmp(comanda_trimisa_1, "exit") == 0)
            {
                exit(3);
            }
            else if (strcmp(comanda_trimisa_1, "logout") == 0)
            {
                comanda_ok = 1;
                logare(sd);
            }
            else
            {
                printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
                printf("\n");
            }
        }
    }
    else    // daca este calator si vrea sosirile dintr-o statie
    {
        while (!comanda_ok)
        {
            printf("\n            MENIU - `CĂLĂTORI - AFIȘARE SOSIRI STAȚIE`:\n");
            printf("Alegeți următoarea comandă:\n");
            printf("1. Pentru a reveni la meniul comenzilor unui călător\n");
            printf("2. Pentru a reactualiza panoul pentru sosiri\n");
            printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
            printf("4. Dacă doriți să părăsiți aplicația!\n");
            bzero(comanda_trimisa_1, 100);
            fflush(stdout);
            printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

            fflush(stdout);
            read(0, comanda_trimisa_1, 100);
            comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

            if (strcmp(comanda_trimisa_1, "3") == 0)
                strcpy(comanda_trimisa_1, "logout");
            else if (strcmp(comanda_trimisa_1, "4") == 0)
                strcpy(comanda_trimisa_1, "exit");

            if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
                strcpy(comanda_trimisa_1, ".");

            write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

            if (strcmp(comanda_trimisa_1, "1") == 0)
            {
                comanda_ok = 1;
                client_calator(sd, username);
            }
            else if (strcmp(comanda_trimisa_1, "2") == 0)
            {
                comanda_ok = 1;
                client_statie_afisare_sosiri(sd, username, copie_nume_statie);
            }
            else if (strcmp(comanda_trimisa_1, "exit") == 0)
            {
                exit(3);
            }
            else if (strcmp(comanda_trimisa_1, "logout") == 0)
            {
                comanda_ok = 1;
                logare(sd);
            }
            else
            {
                printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
                printf("\n");
            }
        }
    }
    return 0;
}

int client_statie_afisare_plecari(int sd, char username[30], char nume_statie[30])
{
    char copie_nume_statie[30];
    strcpy(copie_nume_statie, nume_statie);
    
    //  VREA PANOUL PT PLECARI 
    bzero(comanda_primita, sizeof(comanda_primita));
    comanda_primita[0] = '\0';
    if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        perror("Eroare la read() de la client. Linia 239\n");
        return errno;
    }

    if (read(sd, comanda_primita, lungime_sir) <= 0)
    {
        perror("Eroare la read() de la client. Linia 248\n");
        return errno;
    }
    comanda_primita[lungime_sir] = '\0';

    if (strcmp(comanda_primita, "statie fara trenuri") == 0) // daca statia nu este tranzitata de trenuri la sosiri
        printf("Nu există niciun tren care va pleca din %s!\n", username);
    else
    {
        for (int i = 0; i < strlen(copie_nume_statie); i++)
            copie_nume_statie[i] = toupper(copie_nume_statie[i]);

        while (strlen(copie_nume_statie) < 14)
            strcat(copie_nume_statie, "*");
                                                   
        printf("\n***************************PANOU PLECARI - %s**************************\n", copie_nume_statie);
        printf("Pleacă la      Delay        Spre            Trenul Nr.        Direcția\n");
        printf("***********************************************************************************\n");
        printf("%s\n", comanda_primita);
    }


    int comanda_ok = 0;
    if (strstr(username, "-"))    // daca este client statie
    {
        while (!comanda_ok)
        {
            printf("\n            MENIU - `PLECĂRI-STAȚIE`:\n");
            printf("Alegeți următoarea comandă:\n");
            printf("1. Pentru a reveni la meniul comenzilor unei stații.\n");
            printf("2. Pentru a reactualiza panoul pentru plecări.\n");
            printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
            printf("4. Dacă doriți să părăsiți aplicația!\n");
            bzero(comanda_trimisa_1, 100);
            fflush(stdout);
            printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

            fflush(stdout);
            read(0, comanda_trimisa_1, 100);
            comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

            if (strcmp(comanda_trimisa_1, "3") == 0)
                strcpy(comanda_trimisa_1, "logout");
            else if (strcmp(comanda_trimisa_1, "4") == 0)
                strcpy(comanda_trimisa_1, "exit");

            if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
                strcpy(comanda_trimisa_1, ".");

            write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

            if (strcmp(comanda_trimisa_1, "1") == 0)
            {
                comanda_ok = 1;
                client_statie(sd, username);
            }
            else if (strcmp(comanda_trimisa_1, "2") == 0)
            {
                comanda_ok = 1;
                client_statie_afisare_plecari(sd, username, nume_statie);
            }
            else if (strcmp(comanda_trimisa_1, "exit") == 0)
            {
                exit(3);
            }
            else if (strcmp(comanda_trimisa_1, "logout") == 0)
            {
                comanda_ok = 1;
                logare(sd);
            }
            else
            {
                printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
                printf("\n");
            }
        }
    }
    else     // clientul este un calator ce vrea plecarile dintr-o statie
    {
        while (!comanda_ok)
        {
            printf("\n            MENIU - `CĂLĂTORI - AFIȘARE PLECĂRI STAȚIE`:\n");
            printf("Alegeți următoarea comandă:\n");
            printf("1. Pentru a reveni la meniul comenzilor unui călător.\n");
            printf("2. Pentru a reactualiza panoul pentru plecări.\n");
            printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
            printf("4. Dacă doriți să părăsiți aplicația!\n");
            bzero(comanda_trimisa_1, 100);
            fflush(stdout);
            printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

            fflush(stdout);
            read(0, comanda_trimisa_1, 100);
            comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

            if (strcmp(comanda_trimisa_1, "3") == 0)
                strcpy(comanda_trimisa_1, "logout");
            else if (strcmp(comanda_trimisa_1, "4") == 0)
                strcpy(comanda_trimisa_1, "exit");

            if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
                strcpy(comanda_trimisa_1, ".");

            write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

            if (strcmp(comanda_trimisa_1, "1") == 0)
            {
                comanda_ok = 1;
                client_calator(sd, username);
            }
            else if (strcmp(comanda_trimisa_1, "2") == 0)
            {
                comanda_ok = 1;
                client_statie_afisare_plecari(sd, username, nume_statie);
            }
            else if (strcmp(comanda_trimisa_1, "exit") == 0)
            {
                exit(3);
            }
            else if (strcmp(comanda_trimisa_1, "logout") == 0)
            {
                comanda_ok = 1;
                logare(sd);
            }
            else
            {
                printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
                printf("\n");
            }
        }
    }
    return 0;
}

int client_tren(int sd, char username[30])
{
    int comanda_ok = 0;
    while (!comanda_ok)
    {
     printf("\n            MENIU - `TREN`:\n");
        printf("Alegeți ce informații doriți să primiți:\n");
        printf("1. Informatiile despre itinerariul trenului dumneavoastră!\n");
        printf("2. Dacă doriți să actualizați orarul rutei dumneavoastră!\n");
        printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
        printf("4. Dacă doriți să părăsiți aplicația!\n");
        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strcmp(comanda_trimisa_1, "3") == 0)
            strcpy(comanda_trimisa_1, "logout");
        else if (strcmp(comanda_trimisa_1, "4") == 0)
            strcpy(comanda_trimisa_1, "exit");

        if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
            strcpy(comanda_trimisa_1, ".");

        write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

        if (strcmp(comanda_trimisa_1, "1") == 0)
        {
            comanda_ok = 1;
            informatii_tren(sd, username);
        }
        else if (strcmp(comanda_trimisa_1, "2") == 0)
        {
            comanda_ok = 1;
            actualizare_orar_tren(sd, username);
        }
        else if (strcmp(comanda_trimisa_1, "exit") == 0)
        {
            comanda_ok = 1;
            exit(3);
        }
        else if (strcmp(comanda_trimisa_1, "logout") == 0)
        {
            comanda_ok = 1;
            logare(sd);
        }
        else
        {
            printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
            printf("\n");
        }
    }

    return 0;
}

int informatii_tren(int sd, char username[30])
{
    printf("\nAcestea sunt informațiile despre ruta dumneavoastră!\n");

    bzero(comanda_primita, sizeof(comanda_primita));
    comanda_primita[0] = '\0';
    if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        perror("eroare la read() de la client. linia 459\n");
        return errno;
    }

    if (read(sd, comanda_primita, lungime_sir) <= 0)
    {
        perror("eroare la read() de la client. linia 465\n");
        return errno;
    }
    comanda_primita[lungime_sir] = '\0';
    
    printf("\n%s", comanda_primita);

    client_tren(sd, username);

    return 0;
}

int actualizare_orar_tren(int sd, char username[30])
{
    int comanda_ok = 0, gasit_statie = 0, statie_ok = 1, numar_ok = 1, i;
    char copie_comanda[30], nume_statie[20], numar_minute[10];


    while (!comanda_ok)
    {
        gasit_statie = 0;
        statie_ok = 1;
        numar_ok = 1;
        int comanda_actualizare = 0;
        strcpy(comanda_trimisa_1, "");

        printf("\n            MENIU - `ACTUALIZARE INFORMAȚII TREN`:");
        printf("\nPentru a actualiza orarul de sosire/plecare într-o/dintr-o stație respectați următoarea scriere:\n");
        printf("1. Pentru a raporta că ați ajuns mai devreme într-o stație tastați comanda:\n");
        printf("        <nume_statie>-i, unde i resprezintă numărul de minute\n");
        printf("     EX: Brasov-2\n");
        printf("2. Pentru a raporta că ați ajuns mai târziu într-o stație tastați comanda:\n");
        printf("        <nume_statie>+i, unde i resprezintă numărul de minute\n");
        printf("     EX: Bucuresti Nord+3\n");
        printf("3. Tastați ”back” dacă doriți să reveniți la meniul principal al trenurilor!\n");
        printf("4. Tastați ”logout” dacă doriți să vă delogați din contul dumneavoastră!\n");
        printf("5. Tastați ”exit” dacă doriți să părăsiți aplicația!\n");
        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Tastați comanda: ");
        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strstr(comanda_trimisa_1, "+") != 0)
        {
            comanda_actualizare = 1;
            strcpy(copie_comanda, comanda_trimisa_1);
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
        else if (strstr(comanda_trimisa_1, "-") != 0)
        {
            comanda_actualizare = 1;
            strcpy(copie_comanda, comanda_trimisa_1);
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
        } else if (strcmp(comanda_trimisa_1, "logout") == 0 || strcmp(comanda_trimisa_1, "exit") == 0 || strcmp(comanda_trimisa_1, "back") == 0)
            comanda_ok = 1;

        for (i = 0; i < strlen(nume_statie); i++)
        {
            if (nume_statie[i] < 'A' || nume_statie[i]> 'z')
            {
                if(nume_statie[i] != ' ')
                    statie_ok = 0;
            }
        }

        for (i = 0; i < strlen(numar_minute); i++)
        {
            if (numar_minute[i] < '0' || numar_minute[i]> '9')
                numar_ok = 0;
        }

        if (statie_ok == 1 && numar_ok == 1)
            comanda_ok = 1;

        if (comanda_ok == 0)
        {
            printf("Comanda nu a fost recunoscuta! Încercați altă comandă! Linia 505\n");
            printf("\n");
            actualizare_orar_tren(sd, username);
        }
        if (comanda_actualizare)
        {
            // verific daca statia introdusa de client se afla pe itinerariul trenului
            // trimit comanda tastata la server 

            if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
                strcpy(comanda_trimisa_1, ".");

            write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

            // primesc raspunsul daca statie se afla pe itinerariul trenului
            bzero(comanda_primita, sizeof(comanda_primita));
            comanda_primita[0] = '\0';
            if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
            {
                perror("Eroare la read() de la client. Linia 239\n");
                return errno;
            }

            if (read(sd, comanda_primita, lungime_sir) <= 0)
            {
                perror("Eroare la read() de la client. Linia 248\n");
                return errno;
            }
            comanda_primita[lungime_sir] = '\0';

            if (strcmp(comanda_primita, "statie negasita") == 0) // daca statia nu a fost gasita
            {
                printf("Statia introdusa nu se afla pe itinerariul trenului introdus! Încercați din nou!\n");
                comanda_ok = 0;
            }
            else if (strcmp(comanda_primita, "statie gasita") == 0) // daca statia a fost gasita
                comanda_ok = 1;
        }
    }

    if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
        strcpy(comanda_trimisa_1, ".");

    write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

    if (strcmp(comanda_trimisa_1, "exit") == 0)
        exit(3);
    else if (strcmp(comanda_trimisa_1, "logout") == 0)
    {
        comanda_ok = 1;
        logare(sd);
    }
    else if (strcmp(comanda_trimisa_1, "back") == 0)
    {
        comanda_ok = 1;
        client_tren(sd, username);
    }

    printf("Actualizarea datelor s-a realizat cu succes!\n");
    client_tren(sd, username);
    printf("a");
    return 0;

}

int client_calator(int sd, char username[30])
{
    comanda_ok = 0;
    while (!comanda_ok)
    {
        printf("\n            MENIU - `CĂLĂTOR`:\n");
        printf("Alegeți ce informații doriți să primiți:\n");
        printf("1. Panoul cu sosori pentru o stație introdusă. (EX: `1.Iasi`) \n");
        printf("2. Panoul cu plecări pentru o stație introdusă. (EX: `2.Cluj Napoca`)\n");
        printf("3. Panoul cu sosirile din următoarea oră pentru o stație introdusă. (EX: `3.Iasi`)\n");
        printf("4. Panoul cu plecările din următoarea oră pentru o stație introdusă. (EX: `4.Iasi`)\n");
        printf("5. Informațiile trenului / trenurilor ce îl pot duce pe călător din stația A în stația B. (EX: `5.Iasi-Bucuresti Nord`)\n");
        printf("6. Dacă doriți să vă delogați din contul dumneavoastră!\n");
        printf("7. Dacă doriți să părăsiți aplicația!\n");
        printf("\nIntroduceți numărul opțiunii urmat de numele stației/stațiilor/trenului!\n");

        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Introduceți opțiunea (1 / 2 / 3 / 4 / 5 / 6 / 7): ");

        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strcmp(comanda_trimisa_1, "6") == 0)
            strcpy(comanda_trimisa_1, "logout");
        else if (strcmp(comanda_trimisa_1, "7") == 0)
            strcpy(comanda_trimisa_1, "exit");

        if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
            strcpy(comanda_trimisa_1, ".");

        write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));
        if (strstr(comanda_trimisa_1, "1.") != 0 || strstr(comanda_trimisa_1, "2.") != 0 ||
            strstr(comanda_trimisa_1, "3.") != 0 || strstr(comanda_trimisa_1, "4.") != 0)
        {
            char name_statie[30], copie_comanda[30];
            strcpy(copie_comanda, comanda_trimisa_1);
            char* p = strtok(copie_comanda, ".");
            while (p)
            {
                strcpy(name_statie, p);     // salvaz doar numele statiei
                p = strtok(NULL, "-");
            }

            // primesc raspunsul daca statia exista 
            bzero(comanda_primita, sizeof(comanda_primita));
            comanda_primita[0] = '\0';
            if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
            {
                perror("Eroare la read() de la client. Linia 239\n");
                return errno;
            }

            if (read(sd, comanda_primita, lungime_sir) <= 0)
            {
                perror("Eroare la read() de la client. Linia 248\n");
                return errno;
            }
            comanda_primita[lungime_sir] = '\0';

            if (strcmp(comanda_primita, "statie negasita") == 0) // daca statia nu a fost gasita
                printf("\nStatia introdusa nu există! Încercați din nou!\n");
            else if (strcmp(comanda_primita, "statie gasita") == 0) // daca statia a fost gasita
            {
                comanda_ok = 1;

                if (strstr(comanda_trimisa_1, "1."))
                    client_statie_afisare_sosiri(sd, username, name_statie);
                else if (strstr(comanda_trimisa_1, "2."))
                    client_statie_afisare_plecari(sd, username, name_statie);
                else if (strstr(comanda_trimisa_1, "3."))
                    client_calator_afisare_sosiri_1h(sd, username, name_statie);
                else if (strstr(comanda_trimisa_1, "4."))
                    client_calator_afisare_plecari_1h(sd, username, name_statie);
            }
        }
        else if (strstr(comanda_trimisa_1, "5."))
        {
            comanda_ok = 1;

            client_calator_cauta_tren(sd, username);

        }
        else if (strcmp(comanda_trimisa_1, "exit") == 0)
        {
            comanda_ok = 1;
            exit(3);
        }
        else if (strcmp(comanda_trimisa_1, "logout") == 0)
        {
            comanda_ok = 1;
            logare(sd);
        }
        else
        {
            printf("\nComanda nu a fost recunoscuta! Încercați altă comandă!\n");
            printf("\n");
        }
    }
    return 0;
}

int client_calator_afisare_sosiri_1h(int sd, char username[30], char nume_statie[30])
{

    // CALATORUL VREA PANOUL PT SOSIRI PENTRU URMATOAREA ORA
    bzero(comanda_primita, sizeof(comanda_primita));
    comanda_primita[0] = '\0';
    if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        perror("Eroare la read() de la client. Linia 898\n");
        return errno;
    }

    if (read(sd, comanda_primita, lungime_sir) <= 0)
    {
        perror("Eroare la read() de la client. Linia 904\n");
        return errno;
    }
    comanda_primita[lungime_sir] = '\0';

    if (strcmp(comanda_primita, "statie fara trenuri") == 0) // daca statia nu este tranzitata de trenuri la sosiri
        printf("\nNu există niciun tren care va sosi în următoare oră în statia %s!\n", nume_statie);
    else
    {
        char copie_nume_statie[30];
        strcpy(copie_nume_statie, nume_statie);
        for (int i = 0; i < strlen(copie_nume_statie); i++)
            copie_nume_statie[i] = toupper(copie_nume_statie[i]);

        while (strlen(copie_nume_statie) < 14)
            strcat(copie_nume_statie, "*");

        printf("\n********************PANOU SOSIRI URMĂTOAREA ORĂ - %s*******************\n", copie_nume_statie);
        printf("Sosește la     Delay       De la            Trenul Nr.        Direcția\n");
        printf("**********************************************************************************\n");
        printf("%s\n", comanda_primita);
    }
    comanda_ok = 0;
    while (!comanda_ok)
    {
        printf("\n            MENIU - `CĂLĂTORI - AFIȘARE SOSIRI STAȚIE 1h`:\n");
        printf("Alegeți următoarea comandă:\n");
        printf("1. Pentru a reveni la meniul comenzilor unui călător.\n");
        printf("2. Pentru a reactualiza panoul pentru sosiri.\n");
        printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
        printf("4. Dacă doriți să părăsiți aplicația!\n");
        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strcmp(comanda_trimisa_1, "3") == 0)
            strcpy(comanda_trimisa_1, "logout");
        else if (strcmp(comanda_trimisa_1, "4") == 0)
            strcpy(comanda_trimisa_1, "exit");

        if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
            strcpy(comanda_trimisa_1, ".");

        write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

        if (strcmp(comanda_trimisa_1, "1") == 0)
        {
            comanda_ok = 1;
            client_calator(sd, username);
        }
        else if (strcmp(comanda_trimisa_1, "2") == 0)
        {
            comanda_ok = 1;
            client_calator_afisare_sosiri_1h(sd, username, nume_statie);
        }
        else if (strcmp(comanda_trimisa_1, "exit") == 0)
        {
            comanda_ok = 1;
            exit(3);
        }
        else if (strcmp(comanda_trimisa_1, "logout") == 0)
        {
            comanda_ok = 1;
            logare(sd);
        }
        else
        {
            printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
            printf("\n");
        }
    }
    return 0;
}

int client_calator_afisare_plecari_1h(int sd, char username[30], char nume_statie[30])
{
    char copie_nume_statie[30];
    strcpy(copie_nume_statie, nume_statie);

    // CALATORUL VREA PANOUL PT PLECĂRI PENTRU URMATOAREA ORA
    bzero(comanda_primita, sizeof(comanda_primita));
    comanda_primita[0] = '\0';
    if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        perror("Eroare la read() de la client. Linia 898\n");
        return errno;
    }

    if (read(sd, comanda_primita, lungime_sir) <= 0)
    {
        perror("Eroare la read() de la client. Linia 904\n");
        return errno;
    }
    comanda_primita[lungime_sir] = '\0';

    if (strcmp(comanda_primita, "statie fara trenuri") == 0) // daca statia nu este tranzitata de trenuri la sosiri
        printf("Nu există niciun tren care va pleca din %s în următoarea oră!\n", nume_statie);
    else
    {
        for (int i = 0; i < strlen(nume_statie); i++)
            nume_statie[i] = toupper(nume_statie[i]);

        while (strlen(nume_statie) < 14)
            strcat(nume_statie, "*");

        printf("\n********************PANOU PLECĂRI URMĂTOAREA ORĂ - %s*******************\n", nume_statie);
        printf("Pleacă la     Delay       Spre            Trenul Nr.        Direcția\n");
        printf("**********************************************************************************\n");
        printf("%s\n", comanda_primita);
    }

    comanda_ok = 0;
    while (!comanda_ok)
    {
        printf("\n            MENIU - `CĂLĂTORI - AFIȘARE PLECĂRI STAȚIE 1h`:\n");
        printf("Alegeți următoarea comandă:\n");
        printf("1. Pentru a reveni la meniul comenzilor unui călător.\n");
        printf("2. Pentru a reactualiza panoul pentru plecări.\n");
        printf("3. Dacă doriți să vă delogați din contul dumneavoastră!\n");
        printf("4. Dacă doriți să părăsiți aplicația!\n");
        bzero(comanda_trimisa_1, 100);
        fflush(stdout);
        printf("Introduceți opțiunea (1 / 2 / 3 / 4): ");

        fflush(stdout);
        read(0, comanda_trimisa_1, 100);
        comanda_trimisa_1[strlen(comanda_trimisa_1) - 1] = '\0';

        if (strcmp(comanda_trimisa_1, "3") == 0)
            strcpy(comanda_trimisa_1, "logout");
        else if (strcmp(comanda_trimisa_1, "4") == 0)
            strcpy(comanda_trimisa_1, "exit");

        if (strcmp(comanda_trimisa_1, "") == 0)     // daca clientul apasa enter 
            strcpy(comanda_trimisa_1, ".");

        write_to_server(sd, comanda_trimisa_1, strlen(comanda_trimisa_1));

        if (strcmp(comanda_trimisa_1, "1") == 0)
        {
            comanda_ok = 1;
            client_calator(sd, username);
        }
        else if (strcmp(comanda_trimisa_1, "2") == 0)
        {
            comanda_ok = 1;
            client_calator_afisare_plecari_1h(sd, username, nume_statie);
        }
        else if (strcmp(comanda_trimisa_1, "exit") == 0)
        {
            exit(3);
        }
        else if (strcmp(comanda_trimisa_1, "logout") == 0)
        {
            comanda_ok = 1;
            logare(sd);
        }
        else
        {
            printf("Comanda nu a fost recunoscuta! Încercați altă comandă!\n");
            printf("\n");
        }
    }

    return 0;
}

int client_calator_cauta_tren(int sd, char username[30])
{
    // primesc raspunsul daca statiile exista 
    bzero(comanda_primita, sizeof(comanda_primita));
    comanda_primita[0] = '\0';
    if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
    {
        perror("Eroare la read() de la client. Linia 239\n");
        return errno;
    }

    if (read(sd, comanda_primita, lungime_sir) <= 0)
    {
        perror("Eroare la read() de la client. Linia 248\n");
        return errno;
    }
    comanda_primita[lungime_sir] = '\0';

    if (strcmp(comanda_primita, "prima statie nu e ok") == 0) // daca prima statie nu exista
    {
        printf("\nPrima stație introdusă nu există! Încercați din nou!\n");
        client_calator(sd, username);
    }
    else if (strcmp(comanda_primita, "a doua statie nu e ok") == 0)  // daca a doua statie nu exista
    {
        printf("\nA doua stație introdusă nu există! Încercați din nou!\n");
        client_calator(sd, username);
    }
    else if (strcmp(comanda_primita, "nicio statie nu e ok") == 0) // daca ambele statii nu exista
    {
        printf("\nNicio stație introdusă nu există! Încercați din nou!\n");
        client_calator(sd, username);
    }
    else if (strcmp(comanda_primita, "statii identice") == 0) // daca statii sunt identice
    {
        printf("\nStația de plecare este identică cu stația de sosire! Încercați din nou!\n");
        client_calator(sd, username);
    }
    else if (strcmp(comanda_primita, "statii ok") == 0)  // daca statiile exista si nu sunt identice
    {
        printf("\nAcestea sunt informațiile despre trenurile găsite!\n");

        // primesc informatiile despre trenurile gasite 
        bzero(comanda_primita, sizeof(comanda_primita));
        comanda_primita[0] = '\0';
        if (read(sd, &lungime_sir, sizeof(lungime_sir)) <= 0)
        {
            perror("Eroare la read() de la client. Linia 239\n");
            return errno;
        }

        if (read(sd, comanda_primita, lungime_sir) <= 0)
        {
            perror("Eroare la read() de la client. Linia 248\n");
            return errno;
        }
        comanda_primita[lungime_sir] = '\0';

        if (strcmp(comanda_primita, "niciun tren gasit") == 0) // daca nu a fost niciun tren pe ruta respectiva
            printf("\nNu exista niciun tren pe ruta introdusă!\n");
        else
            printf("\n%s\n", comanda_primita);
        client_calator(sd, username);
    }

    return 0;
}

int write_to_server(int sd, char* comanda, int lungime)
{
    //trimite prima data lungimea mesajului la server si apoi mesajul propriu zis
    if (write(sd, &lungime, sizeof(lungime)) <= 0)
    {
        perror("[client]Eroare la write() spre server.\n");
        return errno;
    }

    // trimiterea mesajului la server
    if (write(sd, comanda, strlen(comanda)) <= 0)
    {
        perror("[client]Eroare la write() spre server.\n");
        return errno;
    }
    return 0;
}

int securizare_parola(char password[30])
{
    int Magic = 2795072;
    int Hash = 0;
    for (int i = 0; i < strlen(password); i++)
    {
        Hash = Hash ^ password[i];
        Hash = Hash * Magic;
    }
    return Hash;
}
