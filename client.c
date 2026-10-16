// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define DEFAULT_DOMAIN "localhost"
#define BUFFER_SIZE 1024
#define RESP_SIZE 4096

// Envia un missatge i imprimeix la resposta. Retorna 0 si tot va bé, -1 si hi ha error.
static int enviar_i_rebre(int sock, const char *msg)
{
    char resp[RESP_SIZE];

    // send(socket, dades, nombre_de_bytes, flags): envia 'msg' pel socket (flags 0 = cap opció especial)
    if (send(sock, msg, strlen(msg), 0) < 0) {
        perror("Error en enviar");
        return -1;
    }

    // memset posa tot el buffer a 0 perquè no hi quedi brossa d'una resposta anterior
    // i el text acabi sempre en '\0'.
    memset(resp, 0, sizeof(resp));

    // recv(socket, buffer, mida_màxima, flags): espera dades del servidor.
    // Llegim com a molt sizeof-1 bytes per reservar lloc al '\0'.
    int n = recv(sock, resp, sizeof(resp) - 1, 0);
    if (n < 0) {
        perror("Error en rebre");
        return -1;
    }
    if (n == 0) {
        printf("El servidor ha tancat la connexió.\n");
        return -1;
    }
    printf("Resposta del servidor:\n%s\n", resp);
    return 0;
}

int main(int argc, char *argv[])
{
    int sock;
    char buffer[BUFFER_SIZE];
    char cadena[BUFFER_SIZE];

    char *domini;
    int port;
    char *endptr;

    if (argc == 1) {
        domini = DEFAULT_DOMAIN;
        port = DEFAULT_PORT;
        printf("Cap argument indicat. Utilitzant valors per defecte: %s %d\n", domini, port);
    } else if (argc == 3) {
        domini = argv[1];
        errno = 0;
        port = strtol(argv[2], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
            fprintf(stderr, "Error: el port ha de ser un enter entre 1 i 65535.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Ús: %s [nom_de_domini port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // socket(domini, tipus, protocol)
    //  - AF_INET: IPv4
    //  - SOCK_STREAM: TCP (flux fiable). Alternativa: SOCK_DGRAM (UDP, datagrames
    //    sense connexió ni garanties), o SOCK_RAW (accés directe a nivell IP).
    //  - 0: protocol per defecte
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error en crear el socket");
        return -1;
    }

    // getaddrinfo resol tant noms ("localhost") com IPs ("127.0.0.1"),
    // cosa que inet_pton NO fa. Omple l'estructura sockaddr amb:
    //  - família AF_INET, adreça IP resolta i port (ja en ordre de xarxa, equivalent a htons).
    // htons() és necessari perquè el port ha d'anar en big-endian (ordre de xarxa).
    struct addrinfo hints, *res;
    char port_str[8];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port_str, sizeof(port_str), "%d", port);

    int err = getaddrinfo(domini, port_str, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Error resolent '%s': %s\n", domini, gai_strerror(err));
        close(sock);
        return -1;
    }

    // connect(socket, adreça_servidor, mida_adreça): inicia la connexió TCP amb el servidor
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connexió fallida");
        freeaddrinfo(res);
        close(sock);
        return -1;
    }
    freeaddrinfo(res);

    printf("Connectat al servidor. Podeu començar a enviar missatges.\n");

    int option;

    while (1) {
        char cim[128];
        float km, temps;
        int desnivell;

        printf("\nMenú principal:\n");
        printf("1. Enviar missatge\n");
        printf("2. Inserir ruta\n");
        printf("3. Llistar rutes\n");
        printf("4. Estadístiques\n");
        printf("5. Eliminar ruta\n");
        printf("6. Sortir\n");
        printf("Opció: ");

        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n');
            printf("Opció invàlida\n");
            continue;
        }
        while (getchar() != '\n');

        switch (option)
        {
        case 1:
        case 6:
            if (option == 1) {
                printf("Introdueix el missatge a enviar ('EXIT' per tancar la connexió i sortir): ");
                if (fgets(cadena, BUFFER_SIZE, stdin) == NULL) strcpy(cadena, "EXIT");
                cadena[strcspn(cadena, "\n")] = '\0';
            } else {
                strcpy(cadena, "EXIT");
            }

            enviar_i_rebre(sock, cadena);

            if (strcmp(cadena, "EXIT") == 0) {
                close(sock);
                return 0;
            }
            break;

        case 2:
            printf("Posa el nom del cim (sense espais): ");
            if (scanf("%127s", cim) != 1) { while (getchar() != '\n'); break; }
            printf("Posa els quilometres: ");
            if (scanf("%f", &km) != 1) { while (getchar() != '\n'); printf("Dada incorrecta\n"); break; }
            printf("Posa el desnivell total: ");
            if (scanf("%d", &desnivell) != 1) { while (getchar() != '\n'); printf("Dada incorrecta\n"); break; }
            printf("Posa el temps que has trigat (hores): ");
            if (scanf("%f", &temps) != 1) { while (getchar() != '\n'); printf("Dada incorrecta\n"); break; }
            while (getchar() != '\n');

            snprintf(buffer, sizeof(buffer), "INSERTAR;%s;%f;%d;%f", cim, km, desnivell, temps);
            enviar_i_rebre(sock, buffer);
            break;

        case 3:
            enviar_i_rebre(sock, "LISTAR");
            break;

        case 4:
            enviar_i_rebre(sock, "ESTADISTIQUES");
            break;

        case 5:
            printf("Nom del cim a eliminar: ");
            if (scanf("%127s", cim) != 1) { while (getchar() != '\n'); break; }
            while (getchar() != '\n');
            snprintf(buffer, sizeof(buffer), "ELIMINAR;%s", cim);
            enviar_i_rebre(sock, buffer);
            break;

        default:
            printf("Opció invàlida\n");
            break;
        }
    }

    close(sock);
    return 0;
}