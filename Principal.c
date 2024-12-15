/*
Problema productor-consumidor
Este programa gestiona pedidos de comida en un sistema productor-consumidor que utiliza hilos (threads),
memoria compartida y semáforos para la sincronización. 
      Flores Anzurez Marco Antonio
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_PEDIDOS 100
#define LINE_MAX_LENGTH 256

typedef struct Pedido {
    char cliente[50];
    char comida[50];
    char bebida[50];
    char direccion[100];
    char fecha[20];
    char hora[20];
    int cantidad;
    float precio;
} Pedido;

typedef struct ColaPedidos {
    int inicio;
    int fin;
    Pedido pedidos[MAX_PEDIDOS];
} ColaPedidos;

// Lee un pedido del archivo de pedidos
Pedido crearPedido(FILE* archivoPedidos) {
    Pedido pedido;

    if (fscanf(archivoPedidos, "%[^,],%[^,],%[^,],%[^,],%d,%f\n",
               pedido.cliente, pedido.comida, pedido.bebida,
               pedido.direccion, &pedido.cantidad, &pedido.precio) != 6) {
        printf("Error al leer el archivo de pedidos\n");
        exit(1);
    }

    // Obtiene la fecha y hora actuales para el pedido
    time_t tiempoActual = time(NULL);
    struct tm* tiempoInfo = localtime(&tiempoActual);

    strftime(pedido.fecha, 20, "%Y-%m-%d", tiempoInfo);
    strftime(pedido.hora, 20, "%H:%M:%S", tiempoInfo);

    return pedido;
}

// Imprime los detalles de un pedido
void imprimirPedido(Pedido pedido) {
    printf("\n-------------------------\n");
    printf("Nombre del cliente: %s\n", pedido.cliente);
    printf("Comida: %s\n", pedido.comida);
    printf("Bebida: %s\n", pedido.bebida);
    printf("Dirección de entrega: %s\n", pedido.direccion);
    printf("Fecha del pedido: %s\n", pedido.fecha);
    printf("Hora del pedido: %s\n", pedido.hora);
    printf("Cantidad: %d\n", pedido.cantidad);
    printf("Precio: %.2f\n", pedido.precio);
    printf("-------------------------\n");
}

// Registra el consumo de un pedido en el archivo de registros
void registrarConsumoPedido(const Pedido* pedido) {
    FILE* archivoRegistros = fopen("registro_pedidos.txt", "a");
    if (archivoRegistros == NULL) {
        perror("Error al abrir el archivo de registro");
        exit(1);
    }

    char fechaConsumo[20];
    char horaConsumo[20];

    // Obtiene la fecha y hora actuales para el consumo del pedido
    time_t tiempoActual = time(NULL);
    struct tm* tiempoInfo = localtime(&tiempoActual);

    strftime(fechaConsumo, 20, "%Y-%m-%d", tiempoInfo);
    strftime(horaConsumo, 20, "%H:%M:%S", tiempoInfo);

    fprintf(archivoRegistros, "Cliente: %s, Comida: %s, Bebida: %s, Dirección: %s, Cantidad: %d, Precio: %.2f, Fecha de consumo: %s, Hora de consumo: %s\n",
            pedido->cliente, pedido->comida, pedido->bebida, pedido->direccion, pedido->cantidad, pedido->precio, fechaConsumo, horaConsumo);

    fclose(archivoRegistros);
}

sem_t* sem_espacio;
sem_t* sem_pedidos;
ColaPedidos* cola_pedidos;
int shm_id;

bool fin_archivo = false; // Variable que indica el final del archivo
pthread_mutex_t mutex_fin; // Mutex para proteger la variable fin_archivo

// Elimina la línea correspondiente a un pedido consumido en el archivo de pedidos
void eliminarLineaArchivo(const Pedido* pedido, FILE** archivoPedidos) {
    FILE* archivoTemporal;
    char strTempData[LINE_MAX_LENGTH];
    bool encontrado = false;

    // Abre el archivo temporal en modo escritura.
    archivoTemporal = fopen("archivoTemporal.txt", "w");
    if (archivoTemporal == NULL) {
        printf("Error al abrir archivoTemporal.txt.\n");
        exit(1);
    }

    rewind(*archivoPedidos);

    // Copia todas las líneas del archivo original al temporal, excepto la línea correspondiente al pedido consumido.
    while (fgets(strTempData, LINE_MAX_LENGTH, *archivoPedidos) != NULL) {
        Pedido pedidoLinea;

        sscanf(strTempData, "%[^,],%[^,],%[^,],%[^,],%d,%f\n",
               pedidoLinea.cliente, pedidoLinea.comida, pedidoLinea.bebida,
               pedidoLinea.direccion, &pedidoLinea.cantidad, &pedidoLinea.precio);

        if (strcmp(pedidoLinea.cliente, pedido->cliente) == 0 &&
            strcmp(pedidoLinea.comida, pedido->comida) == 0 &&
            strcmp(pedidoLinea.bebida, pedido->bebida) == 0 &&
            strcmp(pedidoLinea.direccion, pedido->direccion) == 0 &&
            pedidoLinea.cantidad == pedido->cantidad &&
            pedidoLinea.precio == pedido->precio) {
            encontrado = true;
        } else {
            fprintf(archivoTemporal, "%s", strTempData);
        }
    }

    fclose(archivoTemporal);
    fclose(*archivoPedidos);

    if (!encontrado) {
        printf("Error: No se encontró el pedido en el archivo.\n");
        exit(1);
    }

    // Elimina el archivo original y renombra el temporal.
    remove("pedidos.txt");
    rename("archivoTemporal.txt", "pedidos.txt");

    // Vuelve a abrir el archivo de pedidos.
    *archivoPedidos = fopen("pedidos.txt", "r");
    if (*archivoPedidos == NULL) {
        printf("Error al abrir el archivo de pedidos.\n");
        exit(1);
    }
}

// Función que ejecuta el hilo consumidor
void* consumidor(void* arg) {
    ColaPedidos* cola_pedidos = (ColaPedidos*)arg;
    FILE* archivoPedidos = fopen("pedidos.txt", "r");
    if (archivoPedidos == NULL) {
        perror("Error al abrir el archivo de pedidos");
        exit(1);
    }

    while (1) {
        sem_wait(sem_pedidos); // Espera a que haya pedidos disponibles para consumir

        pthread_mutex_lock(&mutex_fin);
        if (fin_archivo && cola_pedidos->inicio == cola_pedidos->fin) {
            pthread_mutex_unlock(&mutex_fin);
            break;
        }
        pthread_mutex_unlock(&mutex_fin);

        Pedido pedido = cola_pedidos->pedidos[cola_pedidos->inicio];
        cola_pedidos->inicio = (cola_pedidos->inicio + 1) % MAX_PEDIDOS;

        sem_post(sem_espacio); // Aumenta el contador de espacios disponibles en la cola

        printf("Consumidor: Consumido pedido de %s\n", pedido.cliente);

        eliminarLineaArchivo(&pedido, &archivoPedidos); // Elimina el pedido consumido del archivo de pedidos

        registrarConsumoPedido(&pedido); // Registra el consumo del pedido en el archivo de registros

        imprimirPedido(pedido); // Imprime los detalles del pedido consumido

        sleep(2);  // Añade un retardo de 2 segundos antes de consumir el siguiente pedido.
    }

    printf("Todos los pedidos han sido consumidos.\n");
    fclose(archivoPedidos);

    return NULL;
}

// Función que ejecuta el hilo productor
void* productor(void* arg) {
    ColaPedidos* cola_pedidos = (ColaPedidos*)arg;
    FILE* archivoPedidos = fopen("pedidos.txt", "r");
    if (archivoPedidos == NULL) {
        perror("Error al abrir el archivo de pedidos");
        exit(1);
    }

    bool terminado = false;

    while (!terminado) {
        Pedido pedido = crearPedido(archivoPedidos); // Crea un nuevo pedido

        sem_wait(sem_espacio); // Espera a que haya espacio disponible en la cola

        cola_pedidos->pedidos[cola_pedidos->fin] = pedido;
        cola_pedidos->fin = (cola_pedidos->fin + 1) % MAX_PEDIDOS;

        sem_post(sem_pedidos); // Aumenta el contador de pedidos en la cola

        printf("Productor: Producido pedido para %s\n", pedido.cliente);

        if (feof(archivoPedidos)) {
            pthread_mutex_lock(&mutex_fin);
            fin_archivo = true;
            pthread_mutex_unlock(&mutex_fin);
            terminado = true;
        }

        sleep(1); // Añade un retardo de 1 segundo antes de producir el siguiente pedido.
    }

    sem_post(sem_pedidos); // Asegura que el consumidor no quede bloqueado si el archivo ya fue leído completamente

    printf("Todos los pedidos han sido producidos.\n");
    fclose(archivoPedidos);

    return NULL;
}

// Libera los recursos utilizados por el programa
void liberarRecursos() {
    shmdt(cola_pedidos);
    shmctl(shm_id, IPC_RMID, NULL);
    sem_close(sem_espacio);
    sem_close(sem_pedidos);
    sem_unlink("sem_espacio");
    sem_unlink("sem_pedidos");
}

int main() {
    key_t key = ftok("archivo_key", 'R');
    size_t shm_size = sizeof(ColaPedidos);

    shm_id = shmget(key, shm_size, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error al crear la memoria compartida");
        exit(1);
    }

    cola_pedidos = (ColaPedidos*)shmat(shm_id, NULL, 0);
    if (cola_pedidos == (ColaPedidos*)-1) {
        perror("Error al adjuntar la memoria compartida");
        exit(1);
    }

    cola_pedidos->inicio = 0;
    cola_pedidos->fin = 0;

    sem_espacio = sem_open("sem_espacio", O_CREAT | O_EXCL, 0666, MAX_PEDIDOS);
    if (sem_espacio == SEM_FAILED) {
        perror("Error al abrir el semáforo sem_espacio en main");
        exit(EXIT_FAILURE);
    }

    sem_pedidos = sem_open("sem_pedidos", O_CREAT | O_EXCL, 0666, 0);
    if (sem_pedidos == SEM_FAILED) {
        perror("Error al abrir el semáforo sem_pedidos en main");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&mutex_fin, NULL);

    pthread_t hilo_productor;
    pthread_t hilo_consumidor;

    if (pthread_create(&hilo_productor, NULL, productor, (void*)cola_pedidos) != 0) {
        perror("Error al crear el hilo productor");
        exit(1);
    }

    if (pthread_create(&hilo_consumidor, NULL, consumidor, (void*)cola_pedidos) != 0) {
        perror("Error al crear el hilo consumidor");
        exit(1);
    }

    pthread_join(hilo_productor, NULL);
    pthread_join(hilo_consumidor, NULL);

    pthread_mutex_destroy(&mutex_fin);

    liberarRecursos(); // Libera los recursos utilizados por el programa

    printf("La ejecución del programa ha finalizado.\n");

    return 0;
}
