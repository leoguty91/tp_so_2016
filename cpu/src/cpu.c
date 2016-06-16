/*
 ============================================================================
 Name        : cpu.c
 Author      : losmallocados
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "cpu.h"

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

int main(void) {

	logger_manager = log_create("cpu.log", "CPU", true, LOG_LEVEL_TRACE); // Creo archivo de log
	inicio_seniales_semaforos();

	tamanio_pagina = 512;

	// Cargo configuraciones desde archivo ini
	t_config_cpu *configuracion = malloc(sizeof(t_config_cpu));
	carga_configuracion_cpu("src/config.cpu.ini", configuracion);
	log_trace(logger_manager, "Proceso CPU creado.");

	socket_nucleo = conecto_con_nucleo(configuracion);

	socket_umc = conecto_con_umc(configuracion);

	// Test para probar primitivas
	//printf("Ejecutando '%s'\n", DEFINICION_VARIABLES);
	//analizadorLinea(strdup(DEFINICION_VARIABLES), &functions,
	//		&kernel_functions);
	//printf("Ejecutando '%s'\n", ASIGNACION);
	//analizadorLinea(strdup(ASIGNACION), &functions, &kernel_functions);

	//char *codigo =
	//		"function imprimir\n    wait mutexA\n        print $0+1\n    signal mutexB\nend\n\nbegin\nvariables f,  A,  g\n    A = \t0\n    !Global = 1+A\n    print !Global\n    jnz !Global Siguiente \n:Proximo\n\t\n    f = 8\t  \n";
	//char *codigo = "function prueba\nvariables a, b\na = 2\nb = 16\nprint b\nprint a\na = a + b\nreturn a\nend\nbegin\nvariables a, b\na = 20\nprint a\nb <- prueba\nprint b\nprint a\nend";
	//char *codigo = "begin\n# primero declaro las variables\nvariables a, b\na = 20\nprint a\nend";
	//t_pcb *pcb = crear_PCB(codigo);
	t_pcb_quantum *pcb_quantum = malloc(sizeof(t_pcb_quantum));
	//pcb_quantum->pcb = pcb;
	pcb_quantum->quantum = 1;

	//t_buffer *pcb_q = serializar_pcb_quantum(pcb_quantum);

	ejecuto_instrucciones(pcb_quantum);

	sem_wait(&s_cpu_finaliza);
	log_trace(logger_manager, "Cerrando CPU.");

	log_destroy(logger_manager);
	sem_destroy(&s_codigo);
	sem_destroy(&s_cpu_finaliza);
	free(pcb_quantum);
	free(configuracion);
	close(socket_umc);
	return EXIT_SUCCESS;
}

t_pcb *crear_PCB(char *codigo_de_consola) {
	t_metadata_program *metadata = malloc(sizeof(t_metadata_program));
	metadata = metadata_desde_literal(codigo_de_consola);
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones_size = metadata->instrucciones_size;
	/*pcb->instrucciones_serializadas = malloc(
	 sizeof(t_intructions) * pcb->instrucciones_size);*/
	//pcb->instrucciones_serializadas = metadata->instrucciones_serializado;
	pcb->estado = NEW;
	pcb->cant_paginas_codigo_stack = obtener_cantidad_paginas_codigo_stack(
			codigo_de_consola);
	pcb->pc = 0;
	pcb->etiquetas = "hola";
	//pcb->etiquetas_size = metadata->etiquetas_size;
	pcb->stack_size_actual = 1;
	pcb->indice_stack = malloc(sizeof(t_indice_stack));
	pcb->indice_stack[0]->posicion_retorno = 2;
	pcb->indice_stack[0]->posicion_variable_retorno = malloc(
			sizeof(t_posicion_memoria));
	pcb->indice_stack[0]->posicion_variable_retorno->offset = 4;
	pcb->indice_stack[0]->posicion_variable_retorno->pagina = 4;
	pcb->indice_stack[0]->posicion_variable_retorno->size = 4;
	pcb->indice_stack[0]->argumentos = malloc(sizeof(t_posicion_memoria) * 1);
	pcb->indice_stack[0]->argumentos[0]->offset = 4;
	pcb->indice_stack[0]->argumentos[0]->pagina = 4;
	pcb->indice_stack[0]->argumentos[0]->size = 4;
	pcb->indice_stack[0]->variables = malloc(sizeof(t_variables_stack) * 1);
	pcb->indice_stack[0]->variables[0]->id = 1;
	pcb->indice_stack[0]->variables[0]->posicion_memoria->offset = 2;
	pcb->indice_stack[0]->variables[0]->posicion_memoria->pagina = 2;
	pcb->indice_stack[0]->variables[0]->posicion_memoria->size = 2;
	pcb->stack_size_maximo = 10;
	pcb->stack_position = strlen(codigo_de_consola);
	pcb->pid = 1;

	return pcb;
}

int obtener_cantidad_paginas_codigo_stack(char *codigo_de_consola) {
	int modulo = (strlen(codigo_de_consola) + 10) % tamanio_pagina;
	int division = (strlen(codigo_de_consola) + 10) / tamanio_pagina;
	if (modulo == 0) {
		return division;
	} else
		return division + 1;
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

void inicio_seniales_semaforos() {
	// Funcion para atender seniales
	signal(SIGUSR1, atender_seniales);
	// Inicio semaforos
	sem_init(&s_cpu_finaliza, 0, 0); // Semaforo para la finalizacion de CPU
	sem_init(&s_codigo, 0, 0); // Semaforo para pedido de lectura de codigo en UMC
	sem_init(&s_instruccion_finalizada, 0, 0); // Semaforo para indicar inst fin con resp de UMC / Nucleo
	sem_init(&s_variable_stack, 0, 0); // Semaforo para pedido de lectura de variable en UMC
	sem_init(&s_variable_compartida, 0, 0); // Semaforo para pedido de lectura de var comp en Nucleo
	// Reservo memoria para Qunatum - PCB
	pcb_quantum = malloc(sizeof(t_pcb_quantum));
	// Inicio variable para instruccion wait de ansisop
	wait_nucleo = 0;
	// Inicio variable para que no mate al proceso
	matar_proceso = 0;
}

int conecto_con_nucleo(t_config_cpu* configuracion) {
	int socket_servidor;
	if ((socket_servidor = conectar_servidor(configuracion->ip_nucleo,
			configuracion->puerto_nucleo, &atender_nucleo)) > 0) {
		log_trace(logger_manager, "CPU conectado con Nucleo.");
		handshake_cpu_nucleo(socket_servidor);
	} else {
		log_warning(logger_manager, "Error al conectarse con Nucleo.");
	}
	return socket_servidor;
}

int conecto_con_umc(t_config_cpu* configuracion) {
	int socket_servidor;
	if ((socket_servidor = conectar_servidor(configuracion->ip_umc,
			configuracion->puerto_umc, &atender_umc)) > 0) {
		log_trace(logger_manager, "CPU conectado con UMC.");
		handshake_cpu_umc(socket_servidor);
	} else {
		log_warning(logger_manager, "Error al conectarse con UMC.");
	}
	return socket_servidor;
}

void atender_seniales(int signum) {
	switch (signum) {
	case SIGUSR1:
		log_trace(logger_manager, "Se recibio la senial SIGUSR1.");
		sem_post(&s_cpu_finaliza);
		break;
	default:
		log_trace(logger_manager, "Se recibio una senial desconocida.");
		break;
	}
}

// Funciones CPU - UMC
void atender_umc(t_paquete *paquete, int socket_conexion) {
	switch (paquete->header->id_mensaje) {
	case RESPUESTA_HANDSHAKE:
		log_info(logger_manager, "Handshake recibido de UMC.");
		respuesta_handshake_cpu_umc(paquete->payload);
		break;
	case RESPUESTA_LEER_PAGINA:
		log_info(logger_manager, "Se leyo una pagina de UMC.");
		respuesta_leer_pagina(paquete->payload);
		break;
	case RESPUESTA_ESCRIBIR_PAGINA:
		log_info(logger_manager, "Se escribio una pagina en UMC.");
		break;
	case ERROR_HANDSHAKE:
		log_error(logger_manager, "Error en Handshake con UMC.");
		break;
	case ERROR_LEER_PAGINA:
		log_error(logger_manager, "Error en lectura de pagina de UMC.");
		envio_excepcion_nucleo(ERROR_LEER_PAGINA,
				"Error en lectura de pagina de UMC.");
		break;
	case ERROR_ESCRIBIR_PAGINA:
		log_error(logger_manager, "Error en escritura de pagina en UMC.");
		envio_excepcion_nucleo(ERROR_ESCRIBIR_PAGINA,
				"Error en escritura de pagina en UMC.");
		break;
	default:
		log_warning(logger_manager, "Comando no reconocido de UMC.");
		break;
	}
}

void handshake_cpu_umc(int socket_servidor) {

	envio_header_a_proceso(socket_servidor, PROCESO_UMC, MENSAJE_HANDSHAKE,
			"Fallo al enviar Handshake a la UMC.");

}

void respuesta_handshake_cpu_umc(void *buffer) {
	t_pagina_tamanio *pagina = malloc(sizeof(t_pagina_tamanio));
	deserializar_pagina_tamanio(buffer, pagina);
	tamanio_pagina = pagina->tamanio;
	log_info(logger_manager, "Se cargo el tamanio de la pagina: %i",
			tamanio_pagina);
}

void respuesta_leer_pagina(void *buffer) {
	t_pagina_completa *pagina = malloc(sizeof(t_pagina_completa));
	deserializar_pagina_completa(buffer, pagina);

	valor_pagina = malloc(pagina->tamanio);
	size_pagina = pagina->tamanio;
	memcpy(valor_pagina, pagina->valor, pagina->tamanio);
	if (pagina_es_codigo) {
		sem_post(&s_codigo);
	} else {
		sem_post(&s_variable_stack);
	}
}

// Funciones CPU - Nucleo
void atender_nucleo(t_paquete *paquete, int socket_conexion) {
	switch (paquete->header->id_mensaje) {
	case RESPUESTA_HANDSHAKE:
		log_info(logger_manager, "Handshake recibido de Nucleo.");
		break;
	case MENSAJE_PCB:
		log_info(logger_manager, "Recibo PCB de Nucleo.");
		recibo_PCB(paquete->payload);
		break;
	case MENSAJE_MATAR:
		log_info(logger_manager, "Recibo matar de Nucleo.");
		matar_proceso = 1;
		break;
	case RESPUESTA_LEER_COMPARTIDA:
		log_info(logger_manager, "Recibo variable compartida de Nucleo.");
		sem_post(&s_variable_compartida);
		break;
	case RESPUESTA_ESCRIBIR_COMPARTIDA:
		log_info(logger_manager, "Se escribio variable compartida en Nucleo.");
		break;
	case RESPUESTA_IMPRIMIR:
		log_info(logger_manager, "Se imprimio variable en Nucleo.");
		break;
	case RESPUESTA_IMPRIMIR_TEXTO:
		log_info(logger_manager, "Se imprimio texto en Nucleo.");
		break;
	case RESPUESTA_ENTRADA_SALIDA:
		log_info(logger_manager, "Recibi respuesta de IO de Nucleo.");
		break;
	case RESPUESTA_WAIT:
		log_info(logger_manager, "Recibi wait y envio PCB a Nucleo.");
		enviar_PCB(RESPUESTA_PCB);
		break;
	case RESPUESTA_SEGUI_RAFAGA:
		log_info(logger_manager, "Continuo con mi rafaga actual.");
		sem_post(&s_instruccion_finalizada);
		break;
	case RESPUESTA_SIGNAL:
		log_info(logger_manager, "Recibi respuesta de signal de Nucleo.");
		break;
	case RESPUESTA_QUANTUM:
		log_info(logger_manager, "Recibi respuesta de quantum de Nucleo.");
		break;
	case ERROR_HANDSHAKE:
		log_error(logger_manager, "Error en Handshake con el Nucleo.");
		break;
	case ERROR_LEER_COMPARTIDA:
		log_error(logger_manager, "Error al leer var compartida de Nucleo.");
		break;
	case ERROR_ESCRIBIR_COMPARTIDA:
		log_error(logger_manager, "Error al escribir var comp de Nucleo.");
		break;
	case ERROR_IMPRIMIR:
		log_error(logger_manager, "Error al imprimir variable en Nucleo.");
		break;
	case ERROR_IMPRIMIR_TEXTO:
		log_error(logger_manager, "Error al imprimir texto en Nucleo.");
		break;
	case ERROR_ENTRADA_SALIDA:
		log_error(logger_manager, "Error al recibir IO de Nucleo.");
		break;
	case ERROR_WAIT:
		log_error(logger_manager, "Error al recibir Wait de Nucleo.");
		break;
	case ERROR_SIGNAL:
		log_error(logger_manager, "Error al recibir Signal de Nucleo.");
		break;
	case ERROR_QUANTUM:
		log_error(logger_manager, "Error al recibir quantum de Nucleo.");
		break;
	default:
		log_warning(logger_manager, "Comando no reconocido de Nucleo.");
		break;
	}
}

void handshake_cpu_nucleo(int socket_servidor) {
	envio_header_a_proceso(socket_servidor, PROCESO_NUCLEO, MENSAJE_HANDSHAKE,
			"Fallo al enviar Handshake al Nucleo.");
}

void recibo_PCB(void *buffer) {
	deserializar_pcb_quantum(buffer, pcb_quantum);
	ejecuto_instrucciones();
}

void enviar_PCB(int id_mensaje) {
	void *buffer = serializar_pcb_quantum(pcb_quantum);
	envio_buffer_a_proceso(socket_nucleo, PROCESO_NUCLEO, id_mensaje,
			"Fallo al enviar PCB a Nucleo", buffer);
}

void ejecuto_instrucciones() {
	while (pcb_quantum->quantum != FIN_QUANTUM && !wait_nucleo && !matar_proceso) {

		leo_instruccion_desde_UMC(pcb_quantum->pcb);
		// Espero hasta que llega la pagina
		sem_wait(&s_codigo);

		char instruccion_a_ejecutar[size_pagina + 1];
		memcpy(instruccion_a_ejecutar, valor_pagina, size_pagina);
		instruccion_a_ejecutar[size_pagina] = '\0';
		log_info(logger_manager, "Instruccion a ejecutar: %s",
				instruccion_a_ejecutar);
		analizadorLinea(strdup(instruccion_a_ejecutar), &functions,
				&kernel_functions);

		sem_wait(&s_instruccion_finalizada);

		++pcb_quantum->pcb->pc;
		--pcb_quantum->quantum;
	}

	int id_mensaje;

	if (wait_nucleo) {
		id_mensaje = MENSAJE_WAIT;
		wait_nucleo = 0;
	} else if (matar_proceso) {
		id_mensaje = RESPUESTA_MATAR;
		matar_proceso = 0;
	} else {
		id_mensaje = MENSAJE_QUANTUM;
	}

	enviar_PCB(id_mensaje);
}

void envio_excepcion_nucleo(int id_excepcion, char *mensaje_excepcion) {
	t_texto *p_texto = malloc(sizeof(t_texto));
	p_texto->texto = mensaje_excepcion;
	t_buffer *buffer = serializar_texto(p_texto);

	envio_buffer_a_proceso(socket_nucleo, PROCESO_NUCLEO, id_excepcion,
			"Fallo al enviar excepcion al Nucleo.", buffer);

	free(p_texto);
	free(buffer);
}

// TODO Verificar que funciona correctamente
int calcula_pagina(t_puntero_instruccion *instruccion) {
	int modulo = (int) (instruccion) % tamanio_pagina;
	int division = (int) (instruccion) / tamanio_pagina;
	if (modulo != 0) {
		++division;
	}
	return division;
}

void leo_instruccion_desde_UMC(t_pcb *pcb) {

	t_pagina *p_pagina = malloc(sizeof(t_pagina));
	p_pagina->pagina = calcula_pagina(
			pcb->instrucciones_serializadas[pcb->pc]->start);
	p_pagina->offset = pcb->instrucciones_serializadas[pcb->pc]->start;
	p_pagina->tamanio = pcb->instrucciones_serializadas[pcb->pc]->offset;
	p_pagina->socket_pedido = socket_umc;
	t_buffer *buffer = serializar_pagina(p_pagina);

	pagina_es_codigo = 1;

	envio_buffer_a_proceso(socket_umc, PROCESO_UMC, MENSAJE_LEER_PAGINA,
			"Fallo al enviar lectura de pagina a UMC.", buffer);

	free(p_pagina);
	free(buffer);
}

t_buffer *serializar_pcb_quantum(t_pcb_quantum *pcb_quantum) {

	int cantidad_a_reservar = sizeof(pcb_quantum->quantum)
			+ sizeof(pcb_quantum->pcb->pid) + sizeof(pcb_quantum->pcb->pc)
			+ sizeof(pcb_quantum->pcb->cant_paginas_codigo_stack)
			+ sizeof(pcb_quantum->pcb->estado)
			+ sizeof(pcb_quantum->pcb->stack_size_maximo)
			+ sizeof(pcb_quantum->pcb->stack_position) + sizeof(int)
			+ strlen(pcb_quantum->pcb->etiquetas)
			+ sizeof(pcb_quantum->pcb->instrucciones_size)
			+ sizeof(pcb_quantum->pcb->instrucciones_serializadas)
					* pcb_quantum->pcb->instrucciones_size
			+ sizeof(pcb_quantum->pcb->stack_size_actual)
			+ sizeof(pcb_quantum->pcb->indice_stack)
					* pcb_quantum->pcb->stack_size_actual
			+ sizeof(t_posicion_memoria) * pcb_quantum->pcb->cant_argumentos
			+ sizeof(t_variables_stack) * pcb_quantum->pcb->cant_variables;
	void *buffer = malloc(cantidad_a_reservar);

	int posicion_buffer = 0;

	copiar_int_en_buffer(buffer, pcb_quantum->quantum, &posicion_buffer);

	copiar_int_en_buffer(buffer, pcb_quantum->pcb->pid, &posicion_buffer);
	copiar_int_en_buffer(buffer, pcb_quantum->pcb->pc, &posicion_buffer);
	copiar_int_en_buffer(buffer, pcb_quantum->pcb->cant_paginas_codigo_stack,
			&posicion_buffer);
	copiar_int_en_buffer(buffer, pcb_quantum->pcb->estado, &posicion_buffer);
	copiar_int_en_buffer(buffer, pcb_quantum->pcb->stack_size_maximo,
			&posicion_buffer);
	copiar_int_en_buffer(buffer, pcb_quantum->pcb->stack_position,
			&posicion_buffer);
	copiar_string_en_buffer(buffer, pcb_quantum->pcb->etiquetas,
			&posicion_buffer);

	copiar_int_en_buffer(buffer, pcb_quantum->pcb->instrucciones_size,
			&posicion_buffer);
	int i_instrucciones;
	for (i_instrucciones = 0;
			i_instrucciones < pcb_quantum->pcb->instrucciones_size;
			++i_instrucciones) {
		copiar_int_en_buffer(buffer,
				pcb_quantum->pcb->instrucciones_serializadas[i_instrucciones]->offset,
				&posicion_buffer);
		copiar_int_en_buffer(buffer,
				pcb_quantum->pcb->instrucciones_serializadas[i_instrucciones]->start,
				&posicion_buffer);
	}

	copiar_int_en_buffer(buffer, pcb_quantum->pcb->stack_size_actual,
			&posicion_buffer);
	int i_stack;
	for (i_stack = 0; i_stack < pcb_quantum->pcb->stack_size_actual;
			++i_stack) {
		copiar_int_en_buffer(buffer,
				pcb_quantum->pcb->indice_stack[i_stack]->posicion_retorno,
				&posicion_buffer);
		copiar_int_en_buffer(buffer,
				pcb_quantum->pcb->indice_stack[i_stack]->posicion_variable_retorno->offset,
				&posicion_buffer);
		copiar_int_en_buffer(buffer,
				pcb_quantum->pcb->indice_stack[i_stack]->posicion_variable_retorno->pagina,
				&posicion_buffer);
		copiar_int_en_buffer(buffer,
				pcb_quantum->pcb->indice_stack[i_stack]->posicion_variable_retorno->size,
				&posicion_buffer);

		int cant_argumentos =
				sizeof(pcb_quantum->pcb->indice_stack[i_stack]->argumentos);

		copiar_int_en_buffer(buffer, cant_argumentos, &posicion_buffer);
		int i_argumentos;
		for (i_argumentos = 0; i_argumentos < cant_argumentos; ++i_argumentos) {

			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->argumentos[i_argumentos]->offset,
					&posicion_buffer);
			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->argumentos[i_argumentos]->pagina,
					&posicion_buffer);
			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->argumentos[i_argumentos]->size,
					&posicion_buffer);

		}

		int cant_variables =
				sizeof(pcb_quantum->pcb->indice_stack[i_stack]->variables);
		copiar_int_en_buffer(buffer, cant_variables, &posicion_buffer);
		int i_variables;
		for (i_variables = 0; i_variables < cant_variables; ++i_variables) {

			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->id,
					&posicion_buffer);
			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->posicion_memoria->offset,
					&posicion_buffer);
			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->posicion_memoria->pagina,
					&posicion_buffer);
			copiar_int_en_buffer(buffer,
					pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->posicion_memoria->size,
					&posicion_buffer);

		}

	}

	t_buffer *estructura_buffer = malloc(sizeof(t_buffer));
	estructura_buffer->contenido_buffer = buffer;
	estructura_buffer->longitud_buffer = posicion_buffer;

	return (estructura_buffer);
}

void deserializar_pcb_quantum(void *buffer, t_pcb_quantum *pcb_quantum) {
	int posicion_buffer = 0;

	escribir_atributo_desde_int_de_buffer(buffer, &(pcb_quantum->quantum),
			&posicion_buffer);

	escribir_atributo_desde_int_de_buffer(buffer, &(pcb_quantum->pcb->pid),
			&posicion_buffer);
	escribir_atributo_desde_int_de_buffer(buffer, &(pcb_quantum->pcb->pc),
			&posicion_buffer);
	escribir_atributo_desde_int_de_buffer(buffer,
			&(pcb_quantum->pcb->cant_paginas_codigo_stack), &posicion_buffer);
	escribir_atributo_desde_int_de_buffer(buffer, &(pcb_quantum->pcb->estado),
			&posicion_buffer);
	escribir_atributo_desde_int_de_buffer(buffer,
			&(pcb_quantum->pcb->stack_size_maximo), &posicion_buffer);
	escribir_atributo_desde_int_de_buffer(buffer,
			&(pcb_quantum->pcb->stack_position), &posicion_buffer);
	escribir_atributo_desde_string_de_buffer(buffer,
			&(pcb_quantum->pcb->etiquetas), &posicion_buffer);
	escribir_atributo_desde_int_de_buffer(buffer,
			(int*) &(pcb_quantum->pcb->instrucciones_size), &posicion_buffer);
	int i_instrucciones;
	pcb_quantum->pcb->instrucciones_serializadas = malloc(
			sizeof(t_intructions) * pcb_quantum->pcb->instrucciones_size);
	for (i_instrucciones = 0;
			i_instrucciones < pcb_quantum->pcb->instrucciones_size;
			++i_instrucciones) {
		escribir_atributo_desde_int_de_buffer(buffer,
				(int*) &(pcb_quantum->pcb->instrucciones_serializadas[i_instrucciones]->offset),
				&posicion_buffer);
		escribir_atributo_desde_int_de_buffer(buffer,
				(int*) &(pcb_quantum->pcb->instrucciones_serializadas[i_instrucciones]->start),
				&posicion_buffer);
	}

	escribir_atributo_desde_int_de_buffer(buffer,
			&(pcb_quantum->pcb->stack_size_actual), &posicion_buffer);
	int i_stack;
	pcb_quantum->pcb->indice_stack = malloc(
			sizeof(t_indice_stack) * pcb_quantum->pcb->stack_size_actual);
	for (i_stack = 0; i_stack < pcb_quantum->pcb->stack_size_actual;
			++i_stack) {
		escribir_atributo_desde_int_de_buffer(buffer,
				&(pcb_quantum->pcb->indice_stack[i_stack]->posicion_retorno),
				&posicion_buffer);
		escribir_atributo_desde_int_de_buffer(buffer,
				&(pcb_quantum->pcb->indice_stack[i_stack]->posicion_variable_retorno->offset),
				&posicion_buffer);
		escribir_atributo_desde_int_de_buffer(buffer,
				&(pcb_quantum->pcb->indice_stack[i_stack]->posicion_variable_retorno->pagina),
				&posicion_buffer);
		escribir_atributo_desde_int_de_buffer(buffer,
				&(pcb_quantum->pcb->indice_stack[i_stack]->posicion_variable_retorno->size),
				&posicion_buffer);

		int cant_argumentos;

		escribir_atributo_desde_int_de_buffer(buffer, &cant_argumentos,
				&posicion_buffer);
		if (cant_argumentos > 0) {

			pcb_quantum->pcb->indice_stack[i_stack]->argumentos = malloc(
					sizeof(t_posicion_memoria) * cant_argumentos);

			int i_argumentos;
			for (i_argumentos = 0; i_argumentos < cant_argumentos;
					++i_argumentos) {

				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->argumentos[i_argumentos]->offset),
						&posicion_buffer);
				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->argumentos[i_argumentos]->pagina),
						&posicion_buffer);
				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->argumentos[i_argumentos]->size),
						&posicion_buffer);

			}
		}
		int cant_variables;
		escribir_atributo_desde_int_de_buffer(buffer, &cant_variables,
				&posicion_buffer);
		if (cant_variables > 0) {
			pcb_quantum->pcb->indice_stack[i_stack]->variables = malloc(
					sizeof(t_variables_stack) * cant_variables);
			int i_variables;
			for (i_variables = 0; i_variables < cant_variables; ++i_variables) {

				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->id),
						&posicion_buffer);
				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->posicion_memoria->offset),
						&posicion_buffer);
				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->posicion_memoria->pagina),
						&posicion_buffer);
				escribir_atributo_desde_int_de_buffer(buffer,
						&(pcb_quantum->pcb->indice_stack[i_stack]->variables[i_variables]->posicion_memoria->size),
						&posicion_buffer);

			}
		}
	}

}

// {}[]
