/*
 ============================================================================
 Name        : cpu.c
 Author      : losmallocados
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h> // EXIT_SUCCES y otros
#include <unistd.h> // Funcion close
#include <commons/config.h> // Funciones para leer archivos ini
#include <semaphore.h> // Semaforos s_pagina y s_cpu_corriendo
#include <signal.h> // Signal sigusr1
#include <serializacion.h>
#include <comunicaciones.h>

#include "cpu.h"
#include "primitivas_ansisop.h"
#include "semaforo_cpu.h"
#include "serializacion_cpu_nucleo.h"
#include "serializacion_cpu_umc.h"

// Primirivas AnSISOP
AnSISOP_funciones functions = { .AnSISOP_definirVariable =
		ansisop_definir_variable, .AnSISOP_obtenerPosicionVariable =
		ansisop_obtener_posicion_variable, .AnSISOP_dereferenciar =
		ansisop_derefenciar, .AnSISOP_asignar = ansisop_asignar,
		.AnSISOP_obtenerValorCompartida = ansisop_obtener_valor_compartida,
		.AnSISOP_asignarValorCompartida = ansisop_asignar_valor_compartida,
		.AnSISOP_irAlLabel = ansisop_ir_a_label, .AnSISOP_retornar =
				ansisop_retornar, .AnSISOP_imprimir = ansisop_imprimir,
		.AnSISOP_imprimirTexto = ansisop_imprimir_texto,
		.AnSISOP_entradaSalida = ansisop_entrada_salida };
AnSISOP_kernel kernel_functions = { .AnSISOP_wait = ansisop_wait,
		.AnSISOP_signal = ansisop_signal };

// Test para probar primitivas
//static const char* DEFINICION_VARIABLES = "variables a, b, c";
//static const char* ASIGNACION = "a = b + 12";

// Sockets de los procesos a los cuales me conecto
int socket_nucleo;
int socket_umc;

int tamanio_pagina;

int main(void) {
	// Funcion para atender seniales
	signal(SIGUSR1, atender_seniales);
	signal(SIGUSR2, atender_seniales);
	// Inicio semaforos
	sem_init(&s_cpu_finaliza, 0, 0); // Semaforo para el funcionamiento de CPU
	sem_init(&s_pagina, 0, 0); // Semaforo para pedido de lectura de UMC

	// Cargo configuraciones desde archivo ini
	t_config_cpu *configuracion = malloc(sizeof(t_config_cpu));
	carga_configuracion_cpu("src/config.cpu.ini", configuracion);
	printf("Proceso CPU creado.\n");

	socket_nucleo = conecto_con_nucleo(configuracion);

	// TODO Recibir PCB del Nucleo
	// TODO Incrementar registro Program Counter en PCB
	// TODO Hacer el parser del indice de codigo

	socket_umc = conecto_con_umc(configuracion);

	// Test para probar primitivas
	//printf("Ejecutando '%s'\n", DEFINICION_VARIABLES);
	//analizadorLinea(strdup(DEFINICION_VARIABLES), &functions,
	//		&kernel_functions);
	//printf("Ejecutando '%s'\n", ASIGNACION);
	//analizadorLinea(strdup(ASIGNACION), &functions, &kernel_functions);

	// TODO Actualizar valores en UMC
	// TODO Actualizar PC en PCB
	// TODO Notificar al nucleo que termino el Quantum

	sem_wait(&s_cpu_finaliza);
	printf("Cerrando CPU\n");

	sem_destroy(&s_pagina);
	sem_destroy(&s_cpu_finaliza);
	free(configuracion);
	close(socket_umc);
	return EXIT_SUCCESS;
}

void carga_configuracion_cpu(char *archivo, t_config_cpu *configuracion_cpu) {
	t_config *configuracion = malloc(sizeof(t_config));
	configuracion = config_create(archivo);
	if (config_has_property(configuracion, "IP_NUCLEO")) {
		configuracion_cpu->ip_nucleo = config_get_string_value(configuracion,
				"IP_NUCLEO");
	}
	if (config_has_property(configuracion, "PUERTO_NUCLEO")) {
		configuracion_cpu->puerto_nucleo = config_get_int_value(configuracion,
				"PUERTO_NUCLEO");
	}
	if (config_has_property(configuracion, "IP_UMC")) {
		configuracion_cpu->ip_umc = config_get_string_value(configuracion,
				"IP_UMC");
	}
	if (config_has_property(configuracion, "PUERTO_UMC")) {
		configuracion_cpu->puerto_umc = config_get_int_value(configuracion,
				"PUERTO_UMC");
	}
	free(configuracion);
}

int conecto_con_nucleo(t_config_cpu* configuracion) {
	int socket_servidor;
	if ((socket_servidor = conectar_servidor(configuracion->ip_nucleo,
			configuracion->puerto_nucleo, &atender_nucleo)) > 0) {
		printf("CPU conectado con Nucleo\n");
		handshake_cpu_nucleo(socket_servidor);
	} else {
		perror("Error al conectarse con el Nucleo");
	}
	return socket_servidor;
}

int conecto_con_umc(t_config_cpu* configuracion) {
	int socket_servidor;
	if ((socket_servidor = conectar_servidor(configuracion->ip_umc,
			configuracion->puerto_umc, &atender_umc)) > 0) {
		printf("CPU conectado con UMC.\n");
		handshake_cpu_umc(socket_servidor);
	} else {
		perror("Error al conectarse con la UMC\n");
	}
	return socket_servidor;
}

void atender_seniales(int signum) {
	switch (signum) {
	case SIGUSR1:
		printf("Recibi sigusr1\n");
		sem_post(&s_cpu_finaliza);
		break;
	case SIGUSR2:
		printf("Recibi sigusr2, leyendo pag de umc\n");
		leer_pagina(5, 4, 4);
		break;
	default:
		printf("Recibi senial desconocida\n");
		break;
	}
}

// Funciones CPU - UMC

void atender_umc(t_paquete *paquete, int socket_conexion) {
	switch (paquete->header->id_mensaje) {
	case RESPUESTA_HANDSHAKE:
		printf("Handshake recibido de UMC\n");
		respuesta_handshake_cpu_umc(paquete->payload);
		break;
	case RESPUESTA_LEER_PAGINA:
		respuesta_leer_pagina(paquete->payload);
		printf("Recibi respuesta: se leyo la pagina\n");
		break;
	case RESPUESTA_ESCRIBIR_PAGINA:
		// TODO Terminar funcion
		printf("Recibi respuesta: se escribio la pagina\n");
		break;
	case ERROR_HANDSHAKE:
		printf("Error en el Handshake de UMC\n");
		break;
	case ERROR_LEER_PAGINA:
		printf("Error en lectura de pagina\n");
		break;
	case ERROR_ESCRIBIR_PAGINA:
		printf("Error al escribir en pagina");
		break;
	default:
		printf("Comando no reconocido\n");
		break;
	}
}

void definir_variable(char *variable) {
	// TODO Terminar funcion
}

void obtener_posicion_variable(char * variable) {
	// TODO Terminar funcion
}

void handshake_cpu_umc(int socket_servidor) {

	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_UMC;
	header->id_mensaje = MENSAJE_HANDSHAKE;
	header->longitud_mensaje = PAYLOAD_VACIO;

	enviar_header(socket_servidor, header);
	free(header);

}

void respuesta_handshake_cpu_umc(void *buffer) {

	t_pagina_tamanio *pagina = malloc(sizeof(t_pagina_tamanio));
	deserializar_pagina_tamanio(buffer, pagina);
	tamanio_pagina = pagina->tamanio;
	printf("Se cargo el tamanio de la pagina: %i\n", tamanio_pagina);

}

void leer_pagina(int pagina, int offset, int tamanio) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_UMC;
	header->id_mensaje = MENSAJE_LEER_PAGINA;

	t_pagina *p_pagina = malloc(sizeof(t_pagina));
	p_pagina->pagina = pagina;
	p_pagina->offset = offset;
	p_pagina->tamanio = tamanio;
	p_pagina->socket_pedido = socket_umc;
	t_buffer *payload = serializar_pagina(p_pagina);

	header->longitud_mensaje = payload->longitud_buffer;

	if (enviar_buffer(socket_umc, header, payload)
			< sizeof(t_header) + payload->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_pagina);
	free(payload);
}

void respuesta_leer_pagina(void *buffer) {

	t_pagina_completa *pagina = malloc(sizeof(t_pagina_completa));
	deserializar_pagina_completa(buffer, pagina);

	valor_pagina = pagina->valor;
	sem_post(&s_pagina);
}

void escribir_pagina(int pagina, int offset, int tamanio, void *valor) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_UMC;
	header->id_mensaje = MENSAJE_ESCRIBIR_PAGINA;

	t_pagina_completa *p_pagina = malloc(sizeof(t_pagina_completa));
	p_pagina->pagina = pagina;
	p_pagina->offset = offset;
	p_pagina->tamanio = tamanio;
	p_pagina->valor = valor;
	p_pagina->socket_pedido = socket_umc;

	t_buffer * payload = serializar_pagina_completa(p_pagina);

	header->longitud_mensaje = payload->longitud_buffer;

	if (enviar_buffer(socket_umc, header, payload)
			< sizeof(t_header) + payload->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_pagina);
	free(payload);
}

// Funciones CPU - Nucleo
void atender_nucleo(t_paquete *paquete, int socket_conexion) {
	switch (paquete->header->id_mensaje) {
	case RESPUESTA_HANDSHAKE:
		printf("Recibi respuesta de handshake del nucleo\n");
		break;
	default:
		printf("Comando no reconocido\n");
		break;
	}
}

void handshake_cpu_nucleo(int socket_servidor) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_HANDSHAKE;
	header->longitud_mensaje = PAYLOAD_VACIO;

	enviar_header(socket_nucleo, header);
	free(header);
}

void obtener_valor_compartida(char *variable) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_OBTENER_VALOR_COMPARTIDA;

	t_variable *p_compartida = malloc(sizeof(t_variable));
	p_compartida->nombre = variable;

	t_buffer * p_buffer = serializar_variable_compartida(p_compartida);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_compartida);
	free(p_buffer);
}

void asignar_valor_compartida(char *variable, int valor) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_ASIGNAR_VARIABLE_COMPARTIDA;

	t_variable_completa *p_compartida = malloc(sizeof(t_variable_completa));
	p_compartida->nombre = variable;
	p_compartida->valor = valor;

	t_buffer * p_buffer = serializar_asignar_variable_compartida(p_compartida);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_compartida);
	free(p_buffer);
}

void imprimir_variable(char *variable, int valor) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_IMPRIMIR;

	t_variable_completa *p_variable = malloc(sizeof(t_variable_completa));
	p_variable->nombre = variable;
	p_variable->valor = valor;

	t_buffer * p_buffer = serializar_imprimir_variable(p_variable);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_variable);
	free(p_buffer);
}

void imprimir_texto(char *texto) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_IMPRIMIR_TEXTO;

	t_texto *p_texto = malloc(sizeof(t_texto));
	p_texto->texto = texto;

	t_buffer * p_buffer = serializar_imprimir_texto(p_texto);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_texto);
	free(p_buffer);
}

void entrada_salida(char *nombre, int tiempo) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_ENTRADA_SALIDA;

	t_entrada_salida *p_entrada_salida = malloc(sizeof(t_entrada_salida));
	p_entrada_salida->nombre_dispositivo = nombre;
	p_entrada_salida->tiempo = tiempo;

	t_buffer * p_buffer = serializar_entrada_salida(p_entrada_salida);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_entrada_salida);
	free(p_buffer);
}

void wait_semaforo(char *semaforo) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_WAIT;

	t_semaforo *p_semaforo = malloc(sizeof(t_semaforo));
	p_semaforo->nombre = semaforo;

	t_buffer * p_buffer = serializar_semaforo(p_semaforo);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_semaforo);
	free(p_buffer);
}

void signal_semaforo(char *semaforo) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CPU;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_SIGNAL;

	t_semaforo *p_semaforo = malloc(sizeof(t_semaforo));
	p_semaforo->nombre = semaforo;

	t_buffer * p_buffer = serializar_semaforo(p_semaforo);

	header->longitud_mensaje = p_buffer->longitud_buffer;

	if (enviar_buffer(socket_umc, header, p_buffer)
			< sizeof(t_header) + p_buffer->longitud_buffer) {
		perror("Fallo enviar buffer");
	}

	free(header);
	free(p_semaforo);
	free(p_buffer);
}
