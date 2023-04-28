#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include "./sobel.c"

#define CHUNK_SIZE 1024

sem_t sem;

void receive_image(int sockfd) {
    FILE *fp;
    char buffer[CHUNK_SIZE];
    int newsockfd, n, counter = 0;
    struct sockaddr_in cli_addr;
    socklen_t clilen;

    // Acepta conexiones entrantes
   
        sem_wait(&sem); // Decrementa el semáforo

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            printf("Error al aceptar la conexión entrante\n");
            exit(1);
        }

        // Crea un archivo para guardar la imagen recibida
        char image_name[50];
        sprintf(image_name, "imagenrecibida%d.jpg", counter++);
        fp = fopen(image_name, "wb");
        if (fp == NULL) {
            printf("Error al crear el archivo de imagen\n");
            exit(1);
        }

        // Recibe los chunks de la imagen y escribe en el archivo de imagen
        while ((n = recv(newsockfd, buffer, CHUNK_SIZE, 0)) > 0) {
            if (fwrite(buffer, 1, n, fp) != n) {
                printf("Error al escribir los datos en el archivo de imagen\n");
                exit(1);
            }
        }


        // Espera a que todos los datos sean enviados
        if (shutdown(newsockfd, SHUT_WR) < 0) {
            printf("Error al cerrar el socket\n");
            exit(1);
        }

        // Recibe cualquier dato adicional del socket
        while (recv(newsockfd, buffer, CHUNK_SIZE, 0) > 0);

        // Cierra el archivo
        fclose(fp);
        sobel("imagenrecibida0.jpg");
        // Cierra el socket para esta conexión
        close(newsockfd);

        sem_post(&sem); // Incrementa el semáforo
    
}

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;

    // Inicializa el semáforo con valor 1 (semaforo binario)
    sem_init(&sem, 0, 1);

    // Crea el socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error al crear el socket\n");
        return 1;
    }

    // Configura la dirección del servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Asocia el socket con la dirección del servidor
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error al asociar el socket con la dirección del servidor\n");
        return 1;
    }

    // Acepta conexiones entrantes
    while(1){

        listen(sockfd, 5);
        receive_image(sockfd);
        printf("Escuchando");
    }

    // Destruye el semáforo
    sem_destroy(&sem);


    // Cierra el socket principal
    close(sockfd);

    return 0;
}