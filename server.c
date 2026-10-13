// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

#define MAX_RUTAS 10
#define MAX_NOMBRE 128

typedef struct {
    char cim[MAX_NOMBRE];
    float km;
    int desnivell;
    float temps;
} Ruta;

int main(int argc, char *argv[]) {
	int sock, new_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buffer[BUFFER_SIZE] = {0};
	//Socket nou 
	char resposta[2048] = {0}; 
	char recv_buffer[2048] = {0};

	//Per poder guardar les dades de les rutes que ens enviaran els clients
	Ruta rutas[MAX_RUTAS];
	int num_rutas = 0;
	char operacio[128] = "";

	int port;
	char *endptr;

	if (argc == 1) {
		// Cap argument -> port per defecte
		port = DEFAULT_PORT;
		printf("Cap port indicat. Utilitzant valor per defecte: %d\n", port);
	}
	else if (argc == 2) {
		errno = 0;
		port = strtol(argv[1], &endptr, 10);

		// Comprovacions
		if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
			fprintf(stderr, "Error: el port ha de ser un enter entre 1 i 65535.\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		fprintf(stderr, "Ús: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	// Crear el socket
	// Afegiu comentari explicant els arguments
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Error en crear el socket");
		exit(EXIT_FAILURE);
	}


	// Afegiu comentari explicant què són aquests 3 paràmetres,
	// per què s'utilitzen htons() i htonl(),
	// i per què s'utilitzen els valors que hi ha

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);	// Per a INADDR_ANY no faria falta htonl(), però li posem per coherència
	address.sin_port = htons(port);

	// Enllaçar el socket al port especificat
	// Afegiu comentari explicant els arguments
	if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) 
	{
		perror("Error en fer el bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Escoltar connexions entrants
	// Afegiu comentari explicant els arguments
	
	if (listen(sock, 3) < 0) 
	{
		perror("Error en escoltar");
		close(sock);
		exit(EXIT_FAILURE);
	}

	printf("Servidor en funcionament, esperant connexions...\n");

	while (1) {
		// Acceptar connexions de clients
		// Afegiu comentari explicant els arguments i per què hi ha i cal new_socket si ja tenim sock
		
		if ((new_socket = accept(sock, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) 
		{
			perror("Error en acceptar la connexió");
			close(sock);
			exit(EXIT_FAILURE);
		}

		 while (1) {
			memset(buffer, 0, BUFFER_SIZE);

			memset(resposta, 0, sizeof(resposta));
			// Llegir missatge del client
			if (recv(new_socket, buffer, BUFFER_SIZE, 0) <= 0) 
			{
				printf("Client desconnectat (PID: %d)\n", getpid());
				break;
			}

			// Aquí haureu d'implementar l'anàlisi de la cadena rebuda per saber l'operació,
			// i si n'hi ha els arguments, executar-la i tornar el(s) resultat(s)



			// A continuació hi ha el codi corresponent a l'opció d'Enviar missatge, amb el retorn d'una cadena fixa,
			// i la de Sortida de la connexió/bucle si és el cas.
			// Modifiqueu la cadena de retorn per tal que sigui la que diu l'enunciat.

			printf("Missatge rebut del client (PID: %d): %s\n", getpid(), buffer);

			// Comprovar si el client vol tancar la connexió
			if (strcmp(buffer, "EXIT") == 0) 
			{
				printf("Tancant connexió amb el client (PID: %d)...\n", getpid());

				send(new_socket, "Connexió tancada\n", strlen("Connexió tancada\n"), 0);
				break;	// Sortir del bucle
			}

			int resultat = sscanf(buffer,"%127[^;];%127[^;];%f;%d;%f",operacio,
						(num_rutas < MAX_RUTAS) ? rutas[num_rutas].cim : (char[MAX_NOMBRE]){0},
						(num_rutas < MAX_RUTAS) ? &rutas[num_rutas].km : NULL,
						(num_rutas < MAX_RUTAS) ? &rutas[num_rutas].desnivell : NULL,
						(num_rutas < MAX_RUTAS) ? &rutas[num_rutas].temps : NULL);

			if (strcmp(operacio, "INSERTAR") == 0)
			{
				if (num_rutas >= MAX_RUTAS)
				{
					send(new_socket, "ERROR. No es poden guardar mes rutes\n", strlen("ERROR. No es poden guardar mes rutes\n"),0);	
				}
				
				if (resultat != 5)
				{
					send(new_socket, "ERROR. Dades incorrectes\n", strlen("ERROR. Dades incorrectes\n"),0);	
				}
				else if (strcmp(operacio, "INSERTAR") == 0)
				{
					printf("Ruta %d guardada: %s | %.2f km | %d m | %.2f h\n",
							num_rutas + 1,
							rutas[num_rutas].cim,
							rutas[num_rutas].km,
							rutas[num_rutas].desnivell,
							rutas[num_rutas].temps);

					num_rutas++;

					send(new_socket, "CIMS ENTRATS\n", strlen("CIMS ENTRATS\n"), 0);
				}	
			} 
			else 
			{
				snprintf(resposta, sizeof(resposta), "%s. Missatge rebut\n", buffer); // Crear la resposta amb el missatge rebut
				send(new_socket, resposta, strlen(resposta), 0);
			}	
		}

		close(new_socket); // Tancar la connexió amb el client
	}

	close(sock);
	return 0;
}
