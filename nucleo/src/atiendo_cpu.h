/*
 * atiendo_cpu.h
 *
 *  Created on: 7/6/2016
 *      Author: utnso
 */

#ifndef ATIENDO_CPU_H_
#define ATIENDO_CPU_H_

#include "serializacion.h"
#include "nucleo.h"

// Funciones globales de comunicacion
#define MENSAJE_HANDSHAKE 0
#define REPUESTA_HANDSHAKE 10
#define ERROR_HANDSHAKE 20

// Funciones CPU - Nucleo
#define MENSAJE_OBTENER_VALOR_COMPARTIDA 1
#define MENSAJE_ASIGNAR_VARIABLE_COMPARTIDA 2
#define MENSAJE_IMPRIMIR 3
#define MENSAJE_IMPRIMIR_TEXTO 4
#define MENSAJE_ENTRADA_SALIDA 5
#define MENSAJE_WAIT 6
#define MENSAJE_SIGNAL 7
#define MENSAJE_QUANTUM 8
#define MENSAJE_PROGRAMA_FINALIZADO 9
// Respuestas OK
#define RESPUESTA_OBTENER_VALOR_COMPARTIDA 11
#define RESPUESTA_ASIGNAR_VARIABLE_COMPARTIDA 12
#define RESPUESTA_IMPRIMIR 13
#define RESPUESTA_IMPRIMIR_TEXTO 14
#define RESPUESTA_ENTRADA_SALIDA 15
// Respuesta de 16, me bloqueo
#define RESPUESTA_WAIT 16
// Cuando recibo un 16, respondo con un 56 y mando PCB
#define RESPUESTA_PCB 56
// Recibio el PCB correctamente
#define RESPUESTA_PCB_OK 66
#define ERROR_RESPUESTA_PCB 76
// Sigo con mi rafaga actual
#define RESPUESTA_SEGUI_RAFAGA 86
#define RESPUESTA_SIGNAL 17
#define RESPUESTA_QUANTUM 18
#define RESPUESTA_PROGRAMA_FINALIZADO 19
// Respuestas Error
#define ERROR_OBTENER_VALOR_COMPARTIDA 21
#define ERROR_ASIGNAR_VARIABLE_COMPARTIDA 22
#define ERROR_IMPRIMIR 23
#define ERROR_IMPRIMIR_TEXTO 24
#define ERROR_ENTRADA_SALIDA 25
#define ERROR_WAIT 26
#define ERROR_SIGNAL 27
#define ERROR_QUANTUM 28
#define ERROR_PROGRAMA_FINALIZADO 29

// Estructuras para atender CPU
typedef struct {
	char *nombre;
} t_variable;

typedef struct {
	int valor;
} t_variable_valor;

typedef struct {
	char *nombre;
	int valor;
} t_variable_completa;

typedef struct {
	char *texto;
} t_texto;

typedef struct {
	char *nombre_dispositivo;
	int tiempo;
} t_entrada_salida;
//TODO cambiar por otro nombre para que no haya conflictos con el t_semaforo de nucleo.h
typedef struct {
	char *nombre;
} t_semaforo;

// Funciones para atender a CPU
void atender_cpu(t_paquete *paquete, int socket_cpu,
		t_config_nucleo *configuracion);
void atiendo_handshake(void *buffer, int socket_conexion);
void atiendo_obtener_compartida(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_asignar_compartida(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_imprimir(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_imprimir_texto(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_entrada_salida(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_wait(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_signal(void *buffer, int socket_conexion,
		t_config_nucleo *configuracion);
void atiendo_quantum(void *buffer, int socket_conexion);
void atiendo_programa_finalizado(void *buffer, int socket_conexion);

// Funciones de Serializacion
t_buffer *serializar_variable(t_variable *variable);
void deserializar_variable(void *buffer, t_variable *variable);

t_buffer *serializar_variable_valor(t_variable_valor *variable);
void deserializar_variable_valor(void *buffer, t_variable_valor*variable);

t_buffer *serializar_variable_completa(t_variable_completa *variable);
void deserializar_variable_completa(void *buffer, t_variable_completa *variable);

t_buffer *serializar_texto(t_texto *texto);
void deserializar_texto(void *buffer, t_texto *texto);

void deserializar_entrada_salida(void *buffer, t_entrada_salida *entrada_salida);

void deserializar_semaforo(void *buffer, t_semaforo *entrada_salida);

t_buffer *serializar_pcb(t_pcb *pcb);

void deserializar_pcb(void *buffer, t_pcb *pcb);

#endif /* ATIENDO_CPU_H_ */