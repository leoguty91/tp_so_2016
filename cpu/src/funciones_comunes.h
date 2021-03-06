/*
 * funciones_comunes.h
 *
 *  Created on: 28/5/2016
 *      Author: utnso
 */

#ifndef SEMAFORO_SOCKETS_CPU_H_
#define SEMAFORO_SOCKETS_CPU_H_

#include <semaphore.h>
#include <commons/log.h> // Libreria de Logs
#include <commons/collections/list.h> // Libreria para listas
#include "serializaciones_cpu.h"

// Semaforos para continuar proceso y finalizar
sem_t s_codigo; // Para cuando pido lectura de codigo
sem_t s_instruccion_finalizada; // Para esperar a recibir respuesta del UMC o Nucleo
sem_t s_cambio_proceso; // Para esperar la confirmacion de cambio de UMC
sem_t s_variable_stack; // Para cuando pido lectura de variable
sem_t s_variable_compartida; // Para cuando pido lectura de var compartida
sem_t s_matar_cpu; // Para cuando llega SIGUSR1
sem_t s_cpu_finaliza; // Cuando llega senial de finalizar cpu
sem_t s_escribir_pagina; // Para cuando pido escribir una pagina en umc
sem_t s_envio_pcb; // Para indicar que el pcb fue enviado
void *valor_pagina; // Contenido de pagina de UMC
int size_pagina; // Tamanio de pagina de UMC
int pagina_es_codigo; // 1 para codigo, 0 para valor int
int wait_nucleo; // 1 para avisar que se mando wait al nucleo
int matar_proceso; // 1 para avisar que mato al proceso
int fin_proceso; // 1 para avisar que termino el proceso
int excepcion_umc; // 1 para avisar que hubo un problema con la UMC
int matar_cpu; // 1 para avisar que llego SIGUSR1
int cpu_ocupada; // 1 si tiene un PCB asignado
int entrada_salida; // 1 si tiene que mandar el PCB a Block
int matar_cpu_sigint;

// Hilos
pthread_t hilo_io; // Thread para la ejecucion de instruccion de IO
pthread_t hilo_instruccion; // Thread para la ejecucion de las instrucciones
pthread_attr_t attr_io;
pthread_attr_t attr_instruccion;

// Sockets de los procesos a los cuales me conecto
int socket_nucleo;
int socket_umc;

// Tamanio de pagina de UMC
int tamanio_pagina;

// Logger
t_log *logger_manager;

// PCB - Quantum
t_pcb_quantum *pcb_quantum;

int calcula_pagina(t_puntero_instruccion instruccion);
int calcula_offset(t_puntero instruccion);
int calcula_offset_instruccion(t_puntero_instruccion start, int pagina);
int instruccion_en_una_pagina(t_intructions *instruccion,
		t_pagina_pedido *pagina);

#endif /* SEMAFORO_SOCKETS_CPU_H_ */
