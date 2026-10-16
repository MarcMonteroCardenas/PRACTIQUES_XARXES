// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024
#define RESP_SIZE 4096
#define MAX_RUTAS 10
#define MAX_NOMBRE 128

typedef struct {
    char cim[MAX_NOMBRE];
    float km;
    int desnivell;
    float temps;
} Ruta;

static void enviar(int s, const char *msg)
{
    if (send(s, msg, strlen(msg), 0) < 0)
        perror("Error en enviar la resposta");
}

int main(int argc, char *argv[])
{
    int sock, new_socket;
    struct sockaddr_in address, client_addr;
    socklen_t addrlen;
    char buffer[BUFFER_SIZE];
    char resposta[RESP_SIZE];

    Ruta rutas[MAX_RUTAS];
    int num_rutas = 0;

    int port;
    char *endptr;

    if (argc == 1) {
        port = DEFAULT_PORT;
        printf("Cap port indicat. Utilitzant valor per defecte: %d\n", port);
    } else if (argc == 2) {
        errno = 0;
        port = strtol(argv[1], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
            fprintf(stderr, "Error: el port ha de ser un enter entre 1 i 65535.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Ús: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // socket(domini, tipus, protocol)
    //  - AF_INET: família d'adreces IPv4
    //  - SOCK_STREAM: socket orientat a connexió (TCP), flux de bytes fiable i ordenat
    //  - 0: protocol per defecte per a aquest tipus (TCP)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error en crear el socket");
        exit(EXIT_FAILURE);
    }

    // Permet reutilitzar el port immediatament després de tancar el servidor
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // sin_family: IPv4.
    // sin_addr.s_addr: INADDR_ANY = acceptar connexions per qualsevol interfície de xarxa.
    // sin_port: port on escoltem.
    // htonl()/htons() converteixen de l'ordre de bytes del host a l'ordre de xarxa
    // (big-endian), perquè cada màquina pot guardar els enters de manera diferent
    // i la xarxa necessita un format comú. htonl = 32 bits (adreça), htons = 16 bits (port).
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    // bind(socket, adreça, mida_adreça): associa el socket a l'adreça IP i port indicats.
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en fer el bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // listen(socket, backlog): posa el socket en mode escolta.
    // 3 = màxim de connexions pendents a la cua abans de ser acceptades.
    if (listen(sock, 3) < 0) {
        perror("Error en escoltar");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Servidor en funcionament, esperant connexions...\n");

    while (1) {
        // accept(socket_escolta, adreça_client, mida_adreça):
        // espera un client i retorna un NOU socket dedicat a aquesta connexió.
        // sock només serveix per escoltar i acceptar; new_socket serveix per parlar
        // amb aquest client concret. Així sock segueix lliure per acceptar-ne d'altres.
        addrlen = sizeof(client_addr);
        if ((new_socket = accept(sock, (struct sockaddr *)&client_addr, &addrlen)) < 0) {
            perror("Error en acceptar la connexió");
            continue;
        }
        printf("Client connectat\n");

        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            memset(resposta, 0, sizeof(resposta));

            int n = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0) {
                printf("Client desconnectat (PID: %d)\n", getpid());
                break;
            }
            buffer[n] = '\0';

            printf("Missatge rebut del client (PID: %d): %s\n", getpid(), buffer);

            if (strcmp(buffer, "EXIT") == 0) {
                printf("Tancant connexió amb el client (PID: %d)...\n", getpid());
                enviar(new_socket, "Connexió tancada\n");
                break;
            }
            else if (strncmp(buffer, "INSERTAR;", 9) == 0) {
                if (num_rutas >= MAX_RUTAS) {
                    enviar(new_socket, "ERROR: Array ple\n");
                    continue;
                }
                Ruta tmp;
                char operacio[MAX_NOMBRE];
                int r = sscanf(buffer, "%127[^;];%127[^;];%f;%d;%f",
                               operacio, tmp.cim, &tmp.km, &tmp.desnivell, &tmp.temps);
                if (r != 5) {
                    enviar(new_socket, "ERROR: Dades incorrectes\n");
                    continue;
                }
                rutas[num_rutas++] = tmp;
                snprintf(resposta, sizeof(resposta),
                         "Ruta inserida correctament. Total de rutes: %d\n", num_rutas);
                enviar(new_socket, resposta);
            }
            else if (strcmp(buffer, "LISTAR") == 0) {
                if (num_rutas == 0) {
                    enviar(new_socket, "No hi ha cap ruta guardada\n");
                    continue;
                }
                size_t len = 0;
                for (int i = 0; i < num_rutas; i++) {
                    len += snprintf(resposta + len, sizeof(resposta) - len,
                        "%d. %s - %.2f km, %d m desnivell, %.2f h\n",
                        i + 1, rutas[i].cim, rutas[i].km, rutas[i].desnivell, rutas[i].temps);
                }
                enviar(new_socket, resposta);
            }
            else if (strcmp(buffer, "ESTADISTIQUES") == 0) {
                if (num_rutas == 0) {
                    enviar(new_socket, "No hi ha cap ruta guardada\n");
                    continue;
                }
                float tkm = 0, ttemps = 0;
                int tdesn = 0, llarga = 0;
                for (int i = 0; i < num_rutas; i++) {
                    tkm += rutas[i].km;
                    tdesn += rutas[i].desnivell;
                    ttemps += rutas[i].temps;
                    if (rutas[i].km > rutas[llarga].km) llarga = i;
                }
                snprintf(resposta, sizeof(resposta),
                    "Rutes: %d | Km totals: %.2f | Desnivell total: %d m | Temps total: %.2f h\n"
                    "Km mitjans: %.2f | Ruta més llarga: %s (%.2f km)\n",
                    num_rutas, tkm, tdesn, ttemps, tkm / num_rutas,
                    rutas[llarga].cim, rutas[llarga].km);
                enviar(new_socket, resposta);
            }
            else if (strncmp(buffer, "ELIMINAR;", 9) == 0) {
                char nom[MAX_NOMBRE];
                if (sscanf(buffer + 9, "%127[^;]", nom) != 1) {
                    enviar(new_socket, "ERROR: Dades incorrectes\n");
                    continue;
                }
                int trobat = -1;
                for (int i = 0; i < num_rutas; i++)
                    if (strcmp(rutas[i].cim, nom) == 0) { trobat = i; break; }

                if (trobat < 0) {
                    enviar(new_socket, "ERROR: Ruta no trobada\n");
                } else {
                    for (int i = trobat; i < num_rutas - 1; i++)
                        rutas[i] = rutas[i + 1];
                    num_rutas--;
                    snprintf(resposta, sizeof(resposta),
                             "Ruta eliminada. Total de rutes: %d\n", num_rutas);
                    enviar(new_socket, resposta);
                }
            }
            else {
                snprintf(resposta, sizeof(resposta), "%s. Missatge rebut\n", buffer);
                enviar(new_socket, resposta);
            }
        }

        close(new_socket);   // Només tanquem el socket d'aquest client; el servidor segueix
    }

    close(sock);
    return 0;
}