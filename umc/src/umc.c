/*
 ============================================================================
 Name        : umc.c
 Author      : losmallocados
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Funcion sleep y close
#include <commons/config.h>
#include <comunicaciones.h>
#include <serializacion.h>
#include <commons/collections/list.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <commons/string.h>
#include <string.h>
#include "umc.h"

int socket_swap;
int socket_nucleo;
void *memoria_principal;
//void *cache_tlb;
t_lista_algoritmo * listas_algoritmo;
t_list *lista_de_marcos,*lista_paginas_tlb,*lista_tablas,*lista_buffer_escritura,*lista_tabla_entradas;
t_config_umc *configuracion;
FILE * dump_file;
t_log * log_umc;

int main(void) {
	//creo archivo log
	log_umc = log_create("umc.out", "Proceso_UMC", 1, LOG_LEVEL_INFO);

	// Estructura de configuracion de la UMC
	configuracion = malloc(sizeof(t_config_umc));
	carga_configuracion_UMC("src/config.umc.ini", configuracion);

	// Se crea el bloque de la memoria principal
	memoria_principal = calloc(configuracion->marcos,
			configuracion->marco_size);
	log_info(log_umc,"Iniciando UMC...  Memoria disponible: %d bytes",configuracion->marcos * configuracion->marco_size );

	//Para usar los algoritmos
	listas_algoritmo = (t_lista_algoritmo *) malloc(CANT_TABLAS_MAX * sizeof(t_lista_algoritmo));

	// Creo Cache TLB
	//cache_tlb = calloc(configuracion->entradas_tlb, sizeof(t_tlb));

	//iniciar listas
	crear_listas();

	//creo los marcos
	crear_marcos();

	//Inicio el archivo dump
	dump_file = txt_open_for_append("./dump.out");
	txt_write_in_file(dump_file, "\n\n NUEVO INICIO DE UMC \n\n");

	// Se realiza la conexión con el swap
	//socket_swap = conecto_con_swap(configuracion);

	// Inicio servidor UMC
	t_configuracion_servidor* servidor_umc = creo_servidor_umc(configuracion);

	//test();

	menu_principal(configuracion);

	free(configuracion);
	free(servidor_umc);
	close(socket_swap);
	free(memoria_principal);
	//free(cache_tlb);
	return EXIT_SUCCESS;
}

void carga_configuracion_UMC(char *archivo, t_config_umc *configuracionUMC) {
	t_config *configuracion = malloc(sizeof(t_config));
	configuracion = config_create(archivo);
	if (config_has_property(configuracion, "PUERTO")) {
		configuracionUMC->puerto = config_get_int_value(configuracion,
				"PUERTO");
	}
	if (config_has_property(configuracion, "IP_SWAP")) {
		configuracionUMC->ip_swap = config_get_string_value(configuracion,
				"IP_SWAP");
	}
	if (config_has_property(configuracion, "PUERTO_SWAP")) {
		configuracionUMC->puerto_swap = config_get_int_value(configuracion,
				"PUERTO_SWAP");
	}
	if (config_has_property(configuracion, "MARCOS")) {
		configuracionUMC->marcos = config_get_int_value(configuracion,
				"MARCOS");
	}
	if (config_has_property(configuracion, "MARCO_SIZE")) {
		configuracionUMC->marco_size = config_get_int_value(configuracion,
				"MARCO_SIZE");
	}
	if (config_has_property(configuracion, "MARCO_X_PROC")) {
		configuracionUMC->marco_x_proc = config_get_int_value(configuracion,
				"MARCO_X_PROC");
	}
	if (config_has_property(configuracion, "ENTRADAS_TLB")) {
		configuracionUMC->entradas_tlb = config_get_int_value(configuracion,
				"ENTRADAS_TLB");
	}
	if (config_has_property(configuracion, "RETARDO")) {
		configuracionUMC->retardo = config_get_int_value(configuracion,
				"RETARDO");
	}
	if (config_has_property(configuracion, "ALGORITMO")) {
		configuracionUMC->algoritmo = config_get_string_value(configuracion,
				"ALGORITMO");
	}
	free(configuracion);
}

t_configuracion_servidor* creo_servidor_umc(t_config_umc* configuracion) {
	t_configuracion_servidor* servidor_umc = malloc(
			sizeof(t_configuracion_servidor));
	servidor_umc->puerto = configuracion->puerto;
	servidor_umc->funcion = &atender_peticiones;
	servidor_umc->parametros_funcion = configuracion;
	crear_servidor(servidor_umc);
	log_info(log_umc,"Servidor UMC disponible para escuchar conexiones");
	return servidor_umc;
}

int conecto_con_swap(t_config_umc* configuracion) {
	int socket_servidor;
	if ((socket_servidor = conectar_servidor(configuracion->ip_swap,
			configuracion->puerto_swap, &atender_swap)) > 0) {
		log_info(log_umc,"Se conecta con el SWAP de manera satisfactoria");
		handshake_umc_swap(socket_servidor, configuracion);
	} else {
		log_error(log_umc,"Error de conexión con el SWAP\n");
		log_info(log_umc,"Se finaliza la UMC");
		exit(1);
	}
	return socket_servidor;
}

void menu_principal(t_config_umc *configuracion) {
	int comando;
	// Comandos ingresados de la consola de UMC
	while(1){
	printf("Ingrese uno de los siguientes comandos para continuar:\n");
	printf("1 - Cambiar retardo de la consola UMC\n");
	printf("2 - Generar reporte y archivo Dump\n");
	printf("3 - Limpiar contenido de LTB o paginas\n");
	scanf("%d", &comando);
	switch (comando) {
	case RETARDO:
		cambiar_retardo(configuracion);
		break;
	case DUMP:
		generar_dump(comando);
		break;
	case FLUSH:
		printf("1 - Limpiar la TLB\n");
		printf("2 - Marcar todas las paginas como modificadas\n");
		scanf("%d", &comando);
		switch (comando) {
			case TLB:
				flush_tlb();
				break;
			case MEMORY:
				marcar_todas_modificadas();
				break;
		}
		break;
	default:
		printf("Comando no reconocido\n");
		break;
	}
	}
}

void atender_peticiones(t_paquete *paquete, int socket_conexion,
		t_config_umc *configuracion) {
	switch (paquete->header->id_proceso_emisor) {
	case PROCESO_CPU:
		atender_cpu(paquete, socket_conexion, configuracion);
		break;
	case PROCESO_NUCLEO:
		atender_nucleo(paquete, socket_conexion, configuracion);
		break;
	default:
		log_error(log_umc,"Proceso ID:%d No tiene permisos para comunicarse con la UMC",paquete->header->id_proceso_emisor);
		break;
	}
}

void atender_cpu(t_paquete *paquete, int socket_conexion,
		t_config_umc *configuracion) {
	switch (paquete->header->id_mensaje) {
	case MENSAJE_HANDSHAKE:
		log_info(log_umc,"Handshake recibido de CPU");
		handshake_umc_cpu(socket_conexion, configuracion);
		break;
	case MENSAJE_LEER_PAGINA:
		log_info(log_umc,"Recibido mensaje derefenciar");
		leer_pagina(paquete->payload, socket_conexion, configuracion);
		break;
	case MENSAJE_ESCRIBIR_PAGINA:
		log_info(log_umc,"Recibido mensaje asignar");
		escribir_pagina(paquete->payload, socket_conexion);
		break;
	case MENSAJE_CAMBIO_PROCESO_ACTIVO:
		// TODO Terminar funcion
		break;
	default:
		log_error(log_umc,"Id mensaje enviado por el CPU no reconocido: %d",paquete->header->id_mensaje);
		break;
	}
}

void atender_nucleo(t_paquete *paquete, int socket_conexion,
		t_config_umc *configuracion) {
	switch (paquete->header->id_mensaje) {
	case MENSAJE_HANDSHAKE:
		log_info(log_umc,"Handshake recibido de Nucleo.");
		handshake_umc_nucleo(socket_conexion, configuracion);
		break;
	case MENSAJE_INICIALIZAR_PROGRAMA:
		log_info(log_umc,"Solicitud de un nuevo programa.");
		iniciar_programa(paquete->payload);
		break;
	case MENSAJE_MATAR_PROGRAMA:
		log_info(log_umc,"Solicitud de finalización de un programa.");
		finalizar_programa(paquete->payload);
		break;
	default:
		log_error(log_umc,"Id mensaje enviado por el NUCLEO no reconocido: %d",paquete->header->id_mensaje);
		break;
	}
}

void atender_swap(t_paquete *paquete, int socket_conexion) {
	switch (paquete->header->id_mensaje) {
	case REPUESTA_HANDSHAKE:
		respuesta_handshake_umc_swap();
		break;
	case RESPUESTA_LEER_PAGINA:
		respuesta_leer_pagina(paquete->payload,RESPUESTA_LEER_PAGINA);
		break;
	case RESPUESTA_ESCRIBIR_PAGINA:
		respuesta_escribir_pagina(paquete->payload,RESPUESTA_ESCRIBIR_PAGINA);
		break;
	case RESPUESTA_ESCRIBIR_PAGINA_NUEVA:
		respuesta_escribir_pagina_nueva(paquete->payload,RESPUESTA_ESCRIBIR_PAGINA_NUEVA);
		break;
	case RESPUESTA_INICIAR_PROGRAMA:
		respuesta_iniciar_programa(paquete->payload,RESPUESTA_INICIAR_PROGRAMA);
		break;
	case RESPUESTA_FINALIZAR_PROGRAMA:
		respuesta_finalizar_programa(paquete->payload,RESPUESTA_FINALIZAR_PROGRAMA);
		break;
	case RESPUESTA_LEER_PAGINA_PARA_ESCRIBIR:
		respuesta_leer_pagina_para_escribir(paquete->payload,RESPUESTA_LEER_PAGINA_PARA_ESCRIBIR);
		break;
	case ERROR_INICIAR_PROGRAMA:
		respuesta_iniciar_programa(paquete->payload,ERROR_INICIAR_PROGRAMA);
		break;
	case ERROR_LEER_PAGINA:
		respuesta_leer_pagina(paquete->payload,ERROR_LEER_PAGINA);
		break;
	case ERROR_ESCRIBIR_PAGINA:
		respuesta_escribir_pagina(paquete->payload,ERROR_ESCRIBIR_PAGINA);
		break;
	case ERROR_ESCRIBIR_PAGINA_NUEVA:
		respuesta_escribir_pagina_nueva(paquete->payload,ERROR_ESCRIBIR_PAGINA_NUEVA);
		break;
	case ERROR_FINALIZAR_PROGRAMA:
		respuesta_finalizar_programa(paquete->payload,ERROR_FINALIZAR_PROGRAMA);
		break;
	case ERROR_LEER_PAGINA_PARA_ESCRIBIR:
		respuesta_leer_pagina_para_escribir(paquete->payload,ERROR_LEER_PAGINA_PARA_ESCRIBIR);
		break;
	default:
		log_error(log_umc,"Id mensaje enviado por el SWAP no reconocido: %d",paquete->header->id_mensaje);
		break;
	}
}

/*void definir_variable() {
 // TODO Quien se encarga de guardar el espacio de la variable?
 // Como se en que pagina debo escribir?
 }*/

//TODO respuesta_escribir_pagina y respuesta_escribir_pagina_nueva,  que tipo de dato van a recibir?

void handshake_umc_swap(int socket_servidor, t_config_umc *configuracion) {
	handshake_proceso(socket_servidor, configuracion, PROCESO_SWAP,
	MENSAJE_HANDSHAKE);
}

void respuesta_handshake_umc_swap() {
	log_info(log_umc,"Handshake de Swap confirmado");
}

void iniciar_programa(void* buffer) {
	t_programa_completo *programa = malloc(sizeof(t_programa_completo));
	deserializar_programa_completo(buffer, programa);

	int i;
	t_fila_tabla_pagina * tabla_paginas = (t_fila_tabla_pagina *) malloc(programa->paginas_requeridas * sizeof(t_fila_tabla_pagina));

	listas_algoritmo[programa->id_programa].lista_paginas_mp = list_create();
	listas_algoritmo[programa->id_programa].puntero = 0;

	//armo la tabla:
	for(i = 0; i < programa->paginas_requeridas; i++){

		tabla_paginas[i].frame = 0;
		tabla_paginas[i].modificado = 0;
		tabla_paginas[i].pid = programa->id_programa;
		tabla_paginas[i].presencia = 0;
		tabla_paginas[i].uso = 0;
		tabla_paginas[i].numero_pagina = i;
		list_add(listas_algoritmo[programa->id_programa].lista_paginas_mp,(tabla_paginas + i));
	}
	guardar_cant_entradas(programa->id_programa,programa->paginas_requeridas);

	list_replace(lista_tablas,programa->id_programa,tabla_paginas); // lista de tablas. El index va a coincidir con el pid
	// ejemplo: t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas, 1); -> me retorna la tabla del programa con pid 1

	//el contenido lo guardo en un buffer
	list_add(lista_buffer_escritura,programa);//guardo en un buffer para mandar luego al swap
	t_header *header_swap = malloc(sizeof(t_header));
	header_swap->id_proceso_emisor = PROCESO_UMC;
	header_swap->id_proceso_receptor = PROCESO_SWAP;
	header_swap->id_mensaje = MENSAJE_INICIAR_PROGRAMA;

	t_programa_nuevo *programa_swap = malloc(sizeof(t_programa_nuevo));
	programa_swap->id_programa = programa->id_programa;
	programa_swap->paginas_requeridas = programa->paginas_requeridas;

	t_buffer *payload_swap = serializar_programa_nuevo(programa_swap);

	header_swap->longitud_mensaje = payload_swap->longitud_buffer;

	if (enviar_buffer(socket_swap, header_swap, payload_swap)
			< sizeof(header_swap) + payload_swap->longitud_buffer) {
		log_error(log_umc,"Fallo la comunicacion con SWAP para iniciar el programa PID:%d",programa->id_programa);
	}
	log_info(log_umc,"Se envia a SWAP una solicitud de reserva de espacio para un nuevo programa - PID:%d",programa->id_programa);

	free(header_swap);
	free(programa_swap);
	free(payload_swap);
}

void respuesta_iniciar_programa(void *buffer,int id_mensaje){

	t_programa * programa = malloc(sizeof(t_programa));
	deserializar_programa(buffer,programa);

	if(id_mensaje == RESPUESTA_INICIAR_PROGRAMA){
		mandar_a_swap(programa->id_programa,0,id_mensaje);//al ser nuevo escribe en la pagina cero

	}
	else if(id_mensaje == ERROR_INICIAR_PROGRAMA){
		eliminar_programa_nuevo_en_buffer(programa->id_programa);
	}
}

void respuesta_iniciar_programa_backup(void *buffer,int id_mensaje) {
	t_programa_completo *programa_completo = malloc (sizeof (t_programa_completo));
	deserializar_programa_completo(buffer , programa_completo);

	t_header *header_nucleo = malloc(sizeof(t_header));
	header_nucleo->id_proceso_emisor = PROCESO_UMC;
	header_nucleo->id_proceso_receptor = PROCESO_NUCLEO;
	// TODO Definir los mensajes entre el nucleo y umc y actualizar esta linea
	header_nucleo->id_mensaje = id_mensaje;
	header_nucleo->longitud_mensaje = PAYLOAD_VACIO;
	if(id_mensaje == ERROR_INICIAR_PROGRAMA){
		log_error(log_umc,"Se produjo un error en el SWAP al guardar un nuevo programa - PID:%d",programa_completo->id_programa);
		t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *)  list_get(lista_tablas, programa_completo->id_programa);
		tabla[0].pid = 0;
		free(tabla);//libero la memoria de la que falla el inicio pero no la saco de la lista para no perder el index
	}
	else {log_info(log_umc,"PID:%d guardado de manera satisfactoria en SWAP",programa_completo->id_programa);}
	if (enviar_header(socket_nucleo, header_nucleo) < sizeof(header_nucleo)) {
		log_error(log_umc,"Error de comunicacion al terminar de iniciar un programa - PID:%d",programa_completo->id_programa);
	}
	free(programa_completo);
	free(header_nucleo);
}

void leer_pagina(void *buffer, int socket_conexion, t_config_umc *configuracion) {
	t_pagina *pagina = malloc(sizeof(t_pagina));
	deserializar_pagina(buffer, pagina);
	log_info(log_umc,"Solicitud de lectura de PID:%d PAGINA:%d",pagina->id_programa,pagina->pagina);
	int marco = 0;
	if(configuracion->entradas_tlb != 0){
		log_info(log_umc,"Accediendo a la caché TLB ...");
		marco = buscar_pagina_tlb(pagina->id_programa,pagina->pagina);
		log_info(log_umc,"Pagina encontrada en la caché TLB. Marco: %d",marco);
	}
	//1° caso: esta en TLB
	if (marco) { //al ser mayor a cero quiere decir que esta en la tlb

		t_pagina_completa *pagina_cpu = malloc(sizeof(t_pagina_completa));
		inicializar_pagina_cpu(pagina_cpu,pagina, socket_conexion);

		int direccion_mp = retornar_direccion_mp(marco);

		pagina_cpu->valor = malloc(pagina->tamanio);
		memcpy(pagina_cpu->valor,direccion_mp + pagina->offset,pagina->tamanio);

		//memcpy(pagina_cpu->valor, "variables a, b", 14);
		//pagina_cpu->valor = "variables a, b";

		enviar_pagina(socket_conexion, PROCESO_CPU, pagina_cpu,RESPUESTA_LEER_PAGINA);

		free(pagina_cpu);

		//2° caso: esta en Memoria Principal
		} else {
		log_info(log_umc,"Pagina no encontrada en la caché TLB. Accediendo a la Memoria Principal......");
		sleep(configuracion->retardo);
		log_info(log_umc,"Se accede satisfactoriamente en %d ms",configuracion->retardo);
		t_pagina_completa *pagina_cpu = malloc(sizeof(t_pagina_completa));
		inicializar_pagina_cpu(pagina_cpu,pagina, socket_conexion);
		t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *)list_get(lista_tablas,pagina->id_programa);
		if (tabla[pagina->pagina].presencia){
			int direccion_mp = retornar_direccion_mp(tabla[pagina->pagina].frame);
			log_info(log_umc,"Pagina encontrada en Memoria. Marco: %d",tabla[pagina->pagina].frame);
			memcpy(pagina_cpu->valor,direccion_mp + pagina->offset,pagina->tamanio);
			if(configuracion->entradas_tlb != 0){ //valido si esta habilitada
			guardar_en_TLB(pagina_cpu,tabla[pagina->pagina].frame);//pongo la pagina en la cache TLB
			}
			enviar_pagina(socket_conexion, PROCESO_CPU, pagina_cpu,RESPUESTA_LEER_PAGINA);
			free(pagina_cpu);
		}
		//3° caso: esta en Swap
			else{
				// Pido la pagina a Swap
				log_info(log_umc,"Pagina no encontrada en Memoria Principal. Se procede a solicitar la pagina a SWAP");
				t_header *header_swap = malloc(sizeof(t_header));
				header_swap->id_proceso_emisor = PROCESO_UMC;
				header_swap->id_proceso_receptor = PROCESO_SWAP;
				header_swap->id_mensaje = MENSAJE_LEER_PAGINA;

				t_pagina *pagina_swap = malloc(sizeof(t_pagina));
				pagina_swap->pagina = pagina->pagina;
				pagina_swap->offset = pagina->offset;
				pagina_swap->tamanio = pagina->tamanio;
				pagina_swap->socket_pedido = socket_conexion;

				t_buffer *payload_swap = serializar_pagina(pagina_swap);

				header_swap->longitud_mensaje = payload_swap->longitud_buffer;

				if (enviar_buffer(socket_swap, header_swap, payload_swap)
						< sizeof(t_header) + payload_swap->longitud_buffer) {
					log_error(log_umc,"Error de comunicacion al enviar buffer Leer pagina a SWAP - PID:%d",pagina->id_programa);
				}

		free(header_swap);
		free(pagina_swap);
		free(payload_swap);
		}
	}

	free(pagina);
}

void respuesta_leer_pagina(void *buffer, int id_mensaje) {

	if(id_mensaje == RESPUESTA_LEER_PAGINA){
		t_pagina_completa *pagina = malloc(sizeof(t_pagina_completa));
		deserializar_pagina_completa(buffer, pagina);

		log_info(log_umc,"Se recibe del SWAP la respuesta de lectura de PID:%d PAGINA:%d",pagina->id_programa,pagina->pagina);

		t_pagina_completa *pagina_cpu = malloc(sizeof(t_pagina_completa));

		pagina_cpu->id_programa = pagina->id_programa;
		pagina_cpu->pagina = pagina->pagina;
		pagina_cpu->offset = pagina->offset;
		pagina_cpu->tamanio = pagina->tamanio;
		pagina_cpu->socket_pedido = pagina->socket_pedido;

		t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pagina->id_programa);

		if(guardar_en_mp(pagina) == 0){// guardo la pagina en memoria
			log_error(log_umc,"Se produce un error al guardar la pagina en memoria PID:%d PAGINA:%d",pagina->id_programa,pagina->pagina);
			id_mensaje = ERROR_LEER_PAGINA;
		}
		if(configuracion->entradas_tlb != 0){ //valido que este habilitada
			guardar_en_TLB(pagina,tabla[pagina->pagina].frame);//pongo la pagina en la cache TLB
		}

		int direccion_mp = retornar_direccion_mp(tabla[pagina->pagina].frame);

		pagina_cpu->valor = malloc(pagina->tamanio);

		memcpy(pagina_cpu->valor,direccion_mp + pagina->offset,pagina->tamanio);

		enviar_pagina(pagina->socket_pedido, PROCESO_CPU, pagina_cpu, id_mensaje);
		log_info(log_umc,"Se envía al CPU la lectura solicitada de PID:%d PAGINA:%d",pagina->id_programa,pagina->pagina);
		free(pagina);

	}
	else if(id_mensaje == ERROR_LEER_PAGINA){
		t_pagina *pagina = malloc(sizeof(t_pagina_completa));
		deserializar_pagina(buffer, pagina);

		t_header * header_cpu = malloc(sizeof(t_header));
		header_cpu->id_proceso_emisor = PROCESO_UMC;
		header_cpu->id_proceso_receptor = PROCESO_CPU;
		header_cpu->id_mensaje = id_mensaje;
		header_cpu->longitud_mensaje = PAYLOAD_VACIO;

		if (enviar_header(pagina->socket_pedido, header_cpu) < sizeof(header_cpu)) {
			log_error(log_umc,"Error de comunicacion");
		}
		free(pagina);
	}

}

void escribir_pagina(void *buffer, int socket_conexion){
	t_pagina_completa *pagina = malloc(sizeof(t_pagina_completa));
	deserializar_pagina_completa(buffer, pagina);
	log_info(log_umc,"Solicitud de escritura de PID:%d PAGINA:%d",pagina->id_programa,pagina->pagina);
	int marco = 0;
	if(configuracion->entradas_tlb != 0){
		log_info(log_umc,"Accediendo a la caché TLB ...");
		marco = buscar_pagina_tlb(pagina->id_programa,pagina->pagina);
		log_info(log_umc,"Pagina encontrada en la caché TLB. Marco: %d",marco);
	}
	t_header *header_cpu = malloc(sizeof(t_header));
	header_cpu->id_proceso_emisor = PROCESO_UMC;
	header_cpu->id_proceso_receptor = PROCESO_CPU;
	header_cpu->id_mensaje = RESPUESTA_ESCRIBIR_PAGINA;
	header_cpu->longitud_mensaje = PAYLOAD_VACIO;

		//1° caso: esta en TLB
		if (marco) { //al ser mayor a cero quiere decir que esta en la tlb

			int direccion_mp = retornar_direccion_mp(marco);
			memcpy(direccion_mp + pagina->offset,pagina->valor,pagina->tamanio);
			marcar_modificada(pagina->id_programa,pagina->pagina);

			if (enviar_header(socket_nucleo, header_cpu) < sizeof(header_cpu)) {
				perror("Error al finalizar programa");
			}
			free(header_cpu);
		//2° caso: esta en memoria
		}else {
			log_info(log_umc,"Pagina no encontrada en la caché TLB. Accediendo a la Memoria Principal......");
			sleep(configuracion->retardo);
			log_info(log_umc,"Se accede satisfactoriamente en %d ms",configuracion->retardo);
			t_pagina_completa *pagina_cpu = malloc(sizeof(t_pagina_completa));
			t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pagina->id_programa);
			if (tabla[pagina->pagina].presencia){
				int direccion_mp = retornar_direccion_mp(tabla[pagina->pagina].frame);
				log_info(log_umc,"Pagina encontrada en Memoria. Marco: %d",tabla[pagina->pagina].frame);
				memcpy(direccion_mp + pagina->offset,pagina->valor,pagina->tamanio);
				marcar_modificada(pagina->id_programa,pagina->pagina);
				if(configuracion->entradas_tlb != 0){
					guardar_en_TLB(pagina_cpu,tabla[pagina->pagina].frame);//pongo la pagina en la cache TLB
				}
				if (enviar_header(socket_nucleo, header_cpu) < sizeof(header_cpu)) {
					log_error(log_umc,"Error de comunicacion con el CPU");
				}
				free(header_cpu);
			}
			//3° caso: esta en Swap
				else{
					// Pido la pagina a Swap. La guardo en memoria y solo escribo en la memoria
					log_info(log_umc,"Pagina no encontrada en Memoria Principal. Se procede a solicitar la pagina a SWAP");
					list_add(lista_buffer_escritura,pagina);//pongo en un buffer lo que se pide escribir .

					t_header *header_swap = malloc(sizeof(t_header));
					header_swap->id_proceso_emisor = PROCESO_UMC;
					header_swap->id_proceso_receptor = PROCESO_SWAP;
					header_swap->id_mensaje = MENSAJE_LEER_PAGINA_PARA_ESCRIBIR;

					t_pagina *pagina_swap = malloc(sizeof(t_pagina));
					pagina_swap->id_programa = pagina->id_programa;
					pagina_swap->pagina = pagina->pagina;
					pagina_swap->offset = 0;
					pagina_swap->tamanio = pagina->tamanio;
					pagina_swap->socket_pedido = socket_conexion;//es del cpu

					t_buffer *payload_swap = serializar_pagina(pagina_swap);

					header_swap->longitud_mensaje = payload_swap->longitud_buffer;

					if (enviar_buffer(socket_swap, header_swap, payload_swap)
							< sizeof(t_header) + payload_swap->longitud_buffer) {
						log_error(log_umc,"Error de comunicacion al enviar Leer pagina a SWAP - PID:%d",pagina->id_programa);
					}
					if (enviar_header(socket_swap, header_swap) < sizeof(header_swap)) {
							perror("Error al finalizar programa");
					}
					free(header_cpu);
					}
		}
	free(pagina);
}

void respuesta_leer_pagina_para_escribir(void *buffer, int id_mensaje){
	t_header *header_cpu = malloc(sizeof(t_header));
	header_cpu->id_proceso_emisor = PROCESO_UMC;
	header_cpu->id_proceso_receptor = PROCESO_CPU;
	header_cpu->longitud_mensaje = PAYLOAD_VACIO;

	t_pagina_completa *pagina_recibida_de_swap = malloc(sizeof(t_pagina_completa));
	deserializar_pagina_completa(buffer, pagina_recibida_de_swap);

	t_pagina_completa *pagina_buffer;
	copiar_pagina_escritura_desde_buffer(pagina_recibida_de_swap->id_programa, pagina_recibida_de_swap->pagina,pagina_buffer);

	if(id_mensaje == RESPUESTA_LEER_PAGINA_PARA_ESCRIBIR){
		//Primero guardo en memoria la pagina como esta en swap y luego la sobreescribo con lo que manda la cpu
		log_info(log_umc,"Se recibe del SWAP la Pagina:%d PID:%d",pagina_recibida_de_swap->pagina,pagina_recibida_de_swap->id_programa);
		if(guardar_en_mp(pagina_recibida_de_swap) == 0){// guardo la pagina en memoria
			header_cpu->id_mensaje = ERROR_ESCRIBIR_PAGINA;
		}
		t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pagina_recibida_de_swap->id_programa);
		tabla[pagina_recibida_de_swap->pagina].presencia = 1;
		if(configuracion->entradas_tlb != 0){
			guardar_en_TLB(pagina_recibida_de_swap,tabla[pagina_recibida_de_swap->pagina].frame);//pongo la pagina en la cache TLB
		}
		int direccion_mp = retornar_direccion_mp(tabla[pagina_buffer->pagina].frame);
		memcpy(direccion_mp + pagina_buffer->offset,pagina_buffer->valor,pagina_buffer->tamanio);
		tabla[pagina_recibida_de_swap->pagina].modificado = 1;

		header_cpu->id_mensaje = RESPUESTA_ESCRIBIR_PAGINA;

	}
	else if(id_mensaje == ERROR_LEER_PAGINA_PARA_ESCRIBIR){
		log_error(log_umc,"Hubo un error al recibir del swap la PAGINA:%d PID:%d",pagina_recibida_de_swap->pagina,pagina_recibida_de_swap->id_programa);
		header_cpu->id_mensaje = ERROR_ESCRIBIR_PAGINA;
	}
	if (enviar_header(pagina_recibida_de_swap->socket_pedido, header_cpu) < sizeof(header_cpu)) {
			log_error(log_umc,"Error al enviar header a cpu");
	}
	free(pagina_buffer);
	free(pagina_recibida_de_swap);
	free(header_cpu);
}


void respuesta_escribir_pagina(void *buffer,int id_mensaje){ //por reemplazo
	t_pagina_completa *pagina_recibida_de_swap = malloc(sizeof(t_pagina_completa));
	deserializar_pagina_completa(buffer, pagina_recibida_de_swap);
	if(id_mensaje == RESPUESTA_ESCRIBIR_PAGINA ){ // para cuando pedi escribir por reemplazo
		log_info(log_umc,"ESCRITURA SATISFACTORIA EN SWAP - PID:%d - PAGINA:%d",pagina_recibida_de_swap->id_programa,pagina_recibida_de_swap->pagina);
	}
	else if(id_mensaje == ERROR_ESCRIBIR_PAGINA ){ // el error se produjo al escribir en swap la pagina a reemplazar => no pude escribir la pagina que pidio el cpu en memoria
		log_error(log_umc,"ERROR DE ESCRITURA EN SWAP - PID:%d - PAGINA:%d",pagina_recibida_de_swap->id_programa,pagina_recibida_de_swap->pagina);
		t_header * header_cpu = malloc(sizeof(t_header));
		header_cpu->id_proceso_emisor = PROCESO_UMC;
		header_cpu->id_proceso_receptor = PROCESO_CPU;
		header_cpu->id_mensaje = ERROR_ESCRIBIR_PAGINA;
		t_buffer * payload_cpu = serializar_pagina_completa(pagina_recibida_de_swap);
		header_cpu->longitud_mensaje = payload_cpu->longitud_buffer;

		if (enviar_buffer(pagina_recibida_de_swap->socket_pedido, header_cpu, payload_cpu)
				< sizeof(t_header) + payload_cpu->longitud_buffer) {
			log_error(log_umc,"Fallo enviar buffer Leer pagina de Swap");
		}
	}
}

void respuesta_escribir_pagina_nueva(void *buffer,int id_mensaje){
	t_programa *programa_id = malloc(sizeof(t_programa));
	deserializar_programa(buffer, programa_id);

	t_buffer * payload_nucleo = serializar_programa(programa_id);

	t_header * header_nucleo = malloc(sizeof(t_header));
	header_nucleo->id_proceso_emisor = PROCESO_UMC;
	header_nucleo->id_proceso_receptor = PROCESO_NUCLEO;
	header_nucleo->longitud_mensaje = payload_nucleo->longitud_buffer;

	if(id_mensaje == RESPUESTA_ESCRIBIR_PAGINA_NUEVA){
		log_info(log_umc,"El SWAP almaceno el nuevo programa PID:%d satisfactoriamente",programa_id->id_programa);
		header_nucleo->id_mensaje = RESPUESTA_INICIALIZAR_PROGRAMA;
	}
	else if(id_mensaje == ERROR_ESCRIBIR_PAGINA_NUEVA){
		log_error(log_umc,"El SWAP fallo al almacenar el nuevo programa PID:%d",programa_id->id_programa);
		header_nucleo->id_mensaje = ERROR_INICIALIZAR_PROGRAMA;
	}
	if (enviar_buffer(socket_nucleo, header_nucleo, payload_nucleo)
			< sizeof(t_header) + payload_nucleo->longitud_buffer) {
		log_error(log_umc,"Error de comunicacion con el Nucleo");
	}
}


void enviar_pagina(int socket, int proceso_receptor, t_pagina_completa *pagina,int id_mensaje) {

	t_header *header_cpu = malloc(sizeof(t_header));
	header_cpu->id_proceso_emisor = PROCESO_UMC;
	header_cpu->id_proceso_receptor = proceso_receptor;
	header_cpu->id_mensaje = id_mensaje;

	t_buffer *payload_cpu = serializar_pagina_completa(pagina);

	header_cpu->longitud_mensaje = payload_cpu->longitud_buffer;

	if (enviar_buffer(socket, header_cpu, payload_cpu)
			< sizeof(t_header) + payload_cpu->longitud_buffer) {
		perror("Fallo al responder pedido CPU");
	}

	free(header_cpu);
	free(payload_cpu);
}


void finalizar_programa(void *buffer) {
	t_programa *programa = malloc(sizeof(t_programa));
	deserializar_programa(buffer, programa);
	t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,programa->id_programa);
	tabla[0].pid = 0; //ver si hay alguna forma mejor
	//saco de tlb paginas asociadas al proceso
	flush_programa_tlb(programa->id_programa);
	//libero los marcos usados por ese proceso
	liberar_marcos(programa->id_programa);
	//saco de memoria - pongo toda la pagina con \0 , hace falta?
	//elimino la lista usada para los algoritmos
	list_clean(listas_algoritmo[programa->id_programa].lista_paginas_mp);
	list_destroy(listas_algoritmo[programa->id_programa].lista_paginas_mp);

	//lista_tabla_entradas
	//pedir a swap limpiarlo

	t_header *header_swap = malloc(sizeof(t_header));
	header_swap->id_proceso_emisor = PROCESO_UMC;
	header_swap->id_proceso_receptor = PROCESO_SWAP;
	header_swap->id_mensaje = MENSAJE_FINALIZAR_PROGRAMA;

	t_programa *programa_swap = malloc(sizeof(t_programa));
	programa_swap->id_programa = programa->id_programa;

	t_buffer *payload_swap = serializar_programa(programa_swap);

	header_swap->longitud_mensaje = payload_swap->longitud_buffer;

	if (enviar_buffer(socket_swap, header_swap, payload_swap)
			< sizeof(header_swap) + payload_swap->longitud_buffer) {
		perror("Fallo al finalizar el programa");
	}

	free(programa);
	free(header_swap);
	free(programa_swap);
	free(payload_swap);

}

void respuesta_finalizar_programa(void *buffer,int id_mensaje) {

	t_programa *programa = malloc(sizeof(t_programa_completo));
	deserializar_programa(buffer, programa);

	t_header *header_nucleo = malloc(sizeof(t_header));
	header_nucleo->id_proceso_emisor = PROCESO_UMC;
	header_nucleo->id_proceso_receptor = PROCESO_NUCLEO;
	header_nucleo->id_mensaje = id_mensaje;
	header_nucleo->longitud_mensaje = PAYLOAD_VACIO;// TODO: ver si mando asi o mando tambien el id

	if (enviar_header(socket_nucleo, header_nucleo) < sizeof(header_nucleo)) {
		log_error(log_umc,"Error de comunicacion con el nucleo");
	}

	free(programa);
	free(header_nucleo);
}

void handshake_umc_cpu(int socket_cpu, t_config_umc *configuracion) {
	handshake_proceso(socket_cpu, configuracion, PROCESO_CPU,
	REPUESTA_HANDSHAKE);
}

void handshake_umc_nucleo(int socket_nucleo, t_config_umc *configuracion) {
	handshake_proceso(socket_nucleo, configuracion, PROCESO_NUCLEO,
	REPUESTA_HANDSHAKE);
}

void handshake_proceso(int socket, t_config_umc *configuracion,
		int proceso_receptor, int id_mensaje) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_UMC;
	header->id_proceso_receptor = proceso_receptor;
	header->id_mensaje = id_mensaje;

	t_pagina_tamanio *pagina = malloc(sizeof(t_pagina_tamanio));
	pagina->tamanio = configuracion->marco_size;

	t_buffer *payload = serializar_pagina_tamanio(pagina);

	header->longitud_mensaje = payload->longitud_buffer;

	if (enviar_buffer(socket, header, payload)
			< sizeof(t_header) + payload->longitud_buffer) {
		log_error(log_umc,"Fallo al enviar el tamanio de pagina");
	}

	free(header);
	free(pagina);
	free(payload);
}

void cambiar_retardo(t_config_umc *configuracion) {
	int comando_retardo;
	printf("Ingrese el retardo (en ms): ");
	scanf("%d", &comando_retardo);
	configuracion->retardo = comando_retardo;
	printf("\n");
	log_info(log_umc,"Retardo modificaco. Nuevo retardo: %d ms",comando_retardo);
	menu_principal(configuracion);
}

void generar_dump(int pid) {
	printf("Especificar el numero de proceso o cero para todos los procesos\n");
	scanf("%d", &pid);
	if (pid == 0){ // se pidio de todos los procesos
		pid = 1;
		while(pid < CANT_TABLAS_MAX){
			t_fila_tabla_pagina * tabla =  (t_fila_tabla_pagina *) list_get(lista_tablas,pid);
			if(tabla[0].pid != 0){ // si es 0 no esta ese proceso activo
				dump_tabla(pid);
				dump_contenido(pid);
			}
		}
	}
	else{ // se pidio de un proceso
		dump_tabla(pid);
		dump_contenido(pid);
	}
}

//Busca una pagina dentro de la TLB. Si está retorna el marco asociado o si no está retorna null => Los marcos tendrian que empezar desde el 1
int buscar_pagina_tlb(int id_programa,int pagina){
	int marco;
	t_tlb * pagina_tlb;
	bool esta_en_tlb(t_tlb *un_tlb){
		return (un_tlb->pid == id_programa && un_tlb->pagina == pagina);
	}
	pagina_tlb = list_remove_by_condition(lista_paginas_tlb,(void*)esta_en_tlb);//si esta se elimina para luego agregarla al final
	if(pagina_tlb == NULL){
		marco = 0;
	}
	else{
		marco = pagina_tlb->frame;
		//la agrego al final
		list_add(lista_paginas_tlb,pagina_tlb);
	}
	return marco;
}

int reemplazar_pagina(t_fila_tabla_pagina * pagina_a_ubicar){
	t_fila_tabla_pagina * pagina_a_sustituir = NULL;
	int pid = pagina_a_ubicar->pid;

	//CLOCK
	if(string_equals_ignore_case(configuracion->algoritmo,"CLOCK")){
		while (pagina_a_sustituir == NULL ){
			pagina_a_sustituir = (t_fila_tabla_pagina *) list_get(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero);
			if (pagina_a_sustituir->uso == 0) {
				break;
			}
			else {
				pagina_a_sustituir->uso = 0;
				listas_algoritmo[pid].puntero++;
				pagina_a_sustituir = NULL;
				if (listas_algoritmo[pid].puntero == list_size(listas_algoritmo[pid].lista_paginas_mp)) {
					listas_algoritmo[pid].puntero = 0;
					}
			}
		}
		pagina_a_ubicar->frame = pagina_a_sustituir->frame;
		pagina_a_sustituir->frame = 0;
		pagina_a_sustituir->presencia = 0;
		if (pagina_a_sustituir->modificado ){
			mandar_a_swap(pagina_a_sustituir->pid,pagina_a_sustituir->numero_pagina,MENSAJE_ESCRIBIR_PAGINA);
		}
		list_replace(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero,pagina_a_ubicar);
		return pagina_a_ubicar->frame;
	}

	//CLOCK MODIFICADO
	if(string_equals_ignore_case(configuracion->algoritmo,"CLOCK_MODIFICADO")){

		// 1° caso: U = 0 y M = 0
		int i = 0;
		while(pagina_a_sustituir == NULL && i < list_size(listas_algoritmo[pid].lista_paginas_mp)){ // hago maximo una vuelta
			pagina_a_sustituir = (t_fila_tabla_pagina *) list_get(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero);
			if (pagina_a_sustituir->uso == 0 && pagina_a_sustituir->modificado == 0) {  break;}  //encuentro
			else {
				listas_algoritmo[pid].puntero++;
				pagina_a_sustituir = NULL;
				if (listas_algoritmo[pid].puntero == list_size(listas_algoritmo[pid].lista_paginas_mp)) {
					listas_algoritmo[pid].puntero = 0;
					}
				i++;
			}
		}

		// 2° caso: U = 0 y M = 1  .  Si hay U = 1 lo pongo en U = 0
		i = 0;
		while(pagina_a_sustituir == NULL && i < list_size(listas_algoritmo[pid].lista_paginas_mp)){ // hago maximo una vuelta
			pagina_a_sustituir = (t_fila_tabla_pagina *) list_get(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero);
			if (pagina_a_sustituir->uso == 0 && pagina_a_sustituir->modificado == 1) {  break;}  //encuentro
			else {
				listas_algoritmo[pid].puntero++;
				pagina_a_sustituir->uso = 0;
				pagina_a_sustituir = NULL;
				if (listas_algoritmo[pid].puntero == list_size(listas_algoritmo[pid].lista_paginas_mp)) {
					listas_algoritmo[pid].puntero = 0;
					}
				i++;
			}
		}

		// 3° caso: U = 1 y M = 0 ( En realidad al cambiarlo en el anterior check busco nuevamente U = 0 y M = 0)
		i = 0;
		while(pagina_a_sustituir == NULL && i < list_size(listas_algoritmo[pid].lista_paginas_mp)){ // hago maximo una vuelta
			pagina_a_sustituir = (t_fila_tabla_pagina *) list_get(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero);
			if (pagina_a_sustituir->uso == 0 && pagina_a_sustituir->modificado == 0) {  break;}  //encuentro
			else {
				listas_algoritmo[pid].puntero++;
				pagina_a_sustituir = NULL;
				if (listas_algoritmo[pid].puntero == list_size(listas_algoritmo[pid].lista_paginas_mp)) {
					listas_algoritmo[pid].puntero = 0;
					}
				i++;
			}
		}

		//4° caso: U = 1 y M = 1  ( busco U = 0 y M = 1)
		while(pagina_a_sustituir == NULL){ // hago toda una vuelta
			pagina_a_sustituir = (t_fila_tabla_pagina *) list_get(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero);
			if (pagina_a_sustituir->uso == 0 && pagina_a_sustituir->modificado == 0) {  break;}  //encuentro
			else {
				listas_algoritmo[pid].puntero++;
				pagina_a_sustituir = NULL;
				if (listas_algoritmo[pid].puntero == list_size(listas_algoritmo[pid].lista_paginas_mp)) {
					listas_algoritmo[pid].puntero = 0;
					}
			}
		}

	pagina_a_ubicar->frame = pagina_a_sustituir->frame;
	pagina_a_sustituir->frame = 0;
	pagina_a_sustituir->presencia = 0;
	if (pagina_a_sustituir->modificado ){
		mandar_a_swap(pagina_a_sustituir->pid,pagina_a_sustituir->numero_pagina,MENSAJE_ESCRIBIR_PAGINA);
	}
	list_replace(listas_algoritmo[pid].lista_paginas_mp,listas_algoritmo[pid].puntero,pagina_a_ubicar);
	}
	return pagina_a_ubicar->frame;
}

void guardar_en_TLB(t_pagina_completa * pagina,int marco){
	//armo la pagina tlb
	t_tlb * pagina_tlb = malloc(sizeof(t_tlb));
	pagina_tlb->frame = marco;
	pagina_tlb->pagina = pagina->pagina;
	pagina_tlb->pid = pagina->id_programa;
	LRU(pagina_tlb);
}

void LRU(t_tlb * pagina_a_ubicar){
	//validar antes de usuarlo que no este deshabilitado
	//valido si hay lugar y sino reemplazo:
	if(list_size(lista_paginas_tlb) < configuracion->entradas_tlb){
		list_add(lista_paginas_tlb,pagina_a_ubicar);
	}
	else{
		t_tlb * pagina_tlb_removida = list_remove(lista_paginas_tlb,0);//el que esta primero va a ser el Least recently used
		list_add(lista_paginas_tlb,pagina_a_ubicar);
		free(pagina_tlb_removida);
	}
}

//elimina todas las entradas de un programa en la tlb
void flush_programa_tlb(int pid){
	int i = 0;
	bool esta_en_tlb(t_tlb *un_tlb){
		return (un_tlb->pid == pid);
	}
	int count = list_count_satisfying(lista_paginas_tlb,(void*)esta_en_tlb);
	while(i < count){
		t_tlb * pagina_tlb = list_remove_by_condition(lista_paginas_tlb,(void*)esta_en_tlb);
		free(pagina_tlb);
		i++;
	}
}

void flush_tlb(){
	int i = 0;
	int count = list_size(lista_paginas_tlb);
	while(i < count){
		t_tlb * pagina_tlb = list_remove(lista_paginas_tlb,0);
		free(pagina_tlb);
		i++;
	}
}

void test(){
	t_programa_completo *programa1 = malloc(sizeof(t_programa_completo));
	programa1->id_programa = 1;
	programa1->paginas_requeridas = 5;
	programa1->codigo = malloc(10);
	programa1->codigo = "soy pid 1\0";
	t_buffer * buffer1 = serializar_programa_completo(programa1);
	iniciar_programa(buffer1->contenido_buffer);
}

void copiar_pagina_escritura_desde_buffer(int pid, int pagina, t_pagina_completa * pag_completa){
	bool es_buffer(t_pagina_completa *un_buffer){
		return (un_buffer->id_programa == pid && un_buffer->pagina == pagina);
	}
	pag_completa = (t_pagina_completa *) list_remove_by_condition(lista_buffer_escritura,(void*)es_buffer);
}

void copiar_programa_nuevo_desde_buffer(int pid, t_programa_completo * buffer_nuevo_programa){
	bool es_buffer(t_programa_completo *un_buffer){
		return (un_buffer->id_programa == pid);
	}
	buffer_nuevo_programa = (t_programa_completo *) list_remove_by_condition(lista_buffer_escritura,(void*)es_buffer);
}

void crear_listas(){
	int i;
	lista_buffer_escritura = list_create();//guardo t_programa_completo y t_fila_tabla_pagina - tener en cuenta por si eso da problemas
	lista_de_marcos = list_create();
	lista_paginas_tlb = list_create();
	lista_tablas = list_create();
	lista_tabla_entradas = list_create();
	t_fila_tabla_pagina * tabla_pagina = malloc(sizeof(t_fila_tabla_pagina));//meto paginas vacias para que me funcione el index por lista
	tabla_pagina[0].pid = 0;
	for(i=0;i < CANT_TABLAS_MAX; i++){
		list_add(lista_tablas,tabla_pagina);
	}
}

int obtener_marco(){

	bool hay_marco_libre(t_marco *un_marco){
	return (un_marco->libre == 1);
	}

	t_marco * marco = (t_marco *) list_find(lista_de_marcos,(void*)hay_marco_libre);
	marco->libre = 0;
	return marco->numero_marco;
}

//retorna la cantidad de paginas del proceso pid que estan en mp
int cant_pag_x_proc(int pid){
	int i = 0;
	int cantidad = 0;
	t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pid);
	while(tabla[i].frame != 0){
		if(tabla[i].presencia){
			cantidad++;
		}
		i++;
	}
	return cantidad;
}

int guardar_en_mp(t_pagina_completa *pagina){
	t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pagina->id_programa);

	bool hay_marco_libre(t_marco *un_marco){
		return (un_marco->libre == 0);
	}
	int numero_marco;
	int dir_mp;
	int paginas_en_mp = cant_pag_x_proc(pagina->id_programa);

	// Posibilidad 1: Tengo marcos libres y no llegue al limite de marcos por proceso
	if(list_any_satisfy(lista_de_marcos,(void*)hay_marco_libre) && paginas_en_mp < configuracion->marco_x_proc){
		numero_marco = obtener_marco();
		tabla[pagina->pagina].frame = numero_marco;
		tabla[pagina->pagina].presencia = 1;
		list_add(listas_algoritmo[pagina->id_programa].lista_paginas_mp,&tabla[pagina->pagina]);
		dir_mp = retornar_direccion_mp(numero_marco);
		memcpy(dir_mp,pagina->valor,configuracion->marco_size);
		return 1;
	}

	// Posibilidad 2: Tengo marcos libres y llegue al limite de marcos por proceso
	//  y Posibilidad 3: No tengo marcos libres y llegue al limite de marcos por procesos
	else if (list_any_satisfy(lista_de_marcos,(void*)hay_marco_libre) && paginas_en_mp == configuracion->marco_x_proc){
		numero_marco = reemplazar_pagina(&tabla[pagina->pagina]);
		tabla[pagina->pagina].frame = numero_marco;
		tabla[pagina->pagina].presencia = 1;
		dir_mp = retornar_direccion_mp(numero_marco);
		memcpy(dir_mp,pagina->valor,configuracion->marco_size);
		return 1;
	}
	// Posibilidad 4a: No tengo marcos libres y no llegue al limite de marcos por proceso ( ninguna esta en mp)
	// Posibilidad 4b: No tengo marcos libres y no llegue al limite de marcos por proceso ( x lo menos una esta en mp)
	else if (!(list_any_satisfy(lista_de_marcos,(void*)hay_marco_libre)) && paginas_en_mp < configuracion->marco_x_proc){
		if(paginas_en_mp == 0){
			return 0;
			log_error(log_umc,"No hay marcos libres y pagina del proceso esta en mp para poder ser reemplazada");
		}
		else{
			numero_marco = reemplazar_pagina(&tabla[pagina->pagina]);
			tabla[pagina->pagina].frame = numero_marco;
			tabla[pagina->pagina].presencia = 1;
			dir_mp = retornar_direccion_mp(numero_marco);
			memcpy(dir_mp,pagina->valor,configuracion->marco_size);
			return 1;
		}
	}
	return 0; // solo lo pongo para sacar el warning
}

void crear_marcos(){
	int i;
	int start = 0;
	for(i = 0; i < configuracion->marcos;i++){ // creo los marcos
		t_marco * nuevo_marco = malloc(sizeof(t_marco));
		nuevo_marco->libre = 1;
		nuevo_marco->numero_marco = i+1;
		nuevo_marco->direccion_mp = (int)memoria_principal + start;
		start = start + configuracion->marco_size;
		list_add(lista_de_marcos,nuevo_marco); // numero_marco = index_de_lista + 1
	}
	/*para pruebas:
	char * string2 =  malloc(100);
	string2 = "vamos a aprobar matando a quien sea xd\0";
	memcpy(memoria_principal + configuracion->marco_size,string2,strlen(string2));//lo guardo en lo que seria el marco 2
	int direccion = retornar_direccion_mp(2);
	char * string = malloc(100);
	memcpy(string,direccion+5,60);//quiero leer offset 5 y tamaño 60
	printf("%s\n",string);*/
}

int retornar_direccion_mp(int un_marco){
	bool es_marco(t_marco * marco){
		return (marco->numero_marco == un_marco);
	}
	t_marco * marco_buscado = (t_marco *) list_find(lista_de_marcos,(void*)es_marco);
	return marco_buscado->direccion_mp;
}

void inicializar_pagina_cpu(t_pagina_completa * pagina_cpu,t_pagina * una_pagina, int socket_conexion){
	pagina_cpu->id_programa = una_pagina->id_programa;
	pagina_cpu->pagina = una_pagina->pagina;
	pagina_cpu->offset = una_pagina->offset;
	pagina_cpu->tamanio = una_pagina->tamanio;
	pagina_cpu->socket_pedido = socket_conexion;
}

void inicializar_pagina_completa_cpu(t_pagina_completa * pagina_cpu,t_pagina_completa * una_pagina, int socket_conexion){
	pagina_cpu->id_programa = una_pagina->id_programa;
	pagina_cpu->pagina = una_pagina->pagina;
	pagina_cpu->offset = una_pagina->offset;
	pagina_cpu->tamanio = una_pagina->tamanio;
	pagina_cpu->socket_pedido = socket_conexion;
}

void marcar_todas_modificadas(){
	int pid = 1;
	int nro_pagina = 0;
	bool es_true(t_tabla_cantidad_entradas *elemento){
		return (elemento->pid == pid);
	}
	t_fila_tabla_pagina * tabla;
	t_tabla_cantidad_entradas * entradas;
	while(pid < CANT_TABLAS_MAX){
		tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pid);
		if(tabla[nro_pagina].pid != 0){ // si es 0 no esta ese proceso activo
			entradas = (t_tabla_cantidad_entradas *) list_find(lista_tabla_entradas,(void*)es_true);
			while(nro_pagina < entradas->cant_paginas){
				marcar_modificada(pid,nro_pagina);
				nro_pagina++;
			}
			nro_pagina = 0;
		}
		pid++;
	}
}

void marcar_modificada(int pid,int pagina){
	t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas, pid);
	if(tabla[pagina].presencia){
		tabla[pagina].modificado = 1;
	}
}

void mandar_a_swap(int pid,int pagina,int id_mensaje){
	t_header *header_swap = malloc(sizeof(t_header));
	header_swap->id_proceso_emisor = PROCESO_UMC;
	header_swap->id_proceso_receptor = PROCESO_SWAP;
	header_swap->id_mensaje = id_mensaje;

	//t_programa_para_escritura * programa_para_escritura = malloc(sizeof(t_programa_para_escritura));
	t_pagina_completa * pagina_completa = malloc(sizeof(t_programa_para_escritura));
	//programa_para_escritura->id_programa = pid;
	//programa_para_escritura->nro_pagina_inicial = pagina;
	pagina_completa->id_programa = pid;
	pagina_completa->offset = 0;
	pagina_completa->pagina = pagina;
	pagina_completa->socket_pedido = 0;// TODO: si es el nucleo ya lo se, y si es un cpu fijarme donde lo estoy guardando

	if(id_mensaje == MENSAJE_ESCRIBIR_PAGINA_NUEVA){
		//busco el contenido que me mando el nucleo en el buffer en que lo guarde y se lo mando al swap
		t_programa_completo * programa_completo_en_buffer;
		copiar_programa_nuevo_desde_buffer(pid, programa_completo_en_buffer);
		pagina_completa->tamanio = strlen(programa_completo_en_buffer->codigo);
		memcpy(pagina_completa->valor,programa_completo_en_buffer->codigo,strlen(programa_completo_en_buffer->codigo));
		t_buffer * buffer = serializar_pagina_completa(pagina_completa);
		pagina_completa->tamanio = buffer->longitud_buffer;
		free(programa_completo_en_buffer);
	}
	else if(id_mensaje == MENSAJE_ESCRIBIR_PAGINA){
		//busco en memoria el contenido de la pagina y se lo mando al swap
		pagina_completa->tamanio = configuracion->marco_size;
		t_fila_tabla_pagina * tabla = (t_fila_tabla_pagina *) list_get(lista_tablas,pid);
		int dir_mp = retornar_direccion_mp(tabla[pagina].frame);
		memcpy(pagina_completa->valor,dir_mp,configuracion->marco_size);
	}
	t_buffer *payload_swap = serializar_pagina_completa(pagina_completa);

	header_swap->longitud_mensaje = payload_swap->longitud_buffer;

	if (enviar_buffer(socket_swap, header_swap, payload_swap)
			< sizeof(t_header) + payload_swap->longitud_buffer) {
		log_error(log_umc,"Fallo al responder pedido CPU");
	}
	free(pagina_completa);
	free(header_swap);
	free(payload_swap);
}

void dump_tabla(int pid){
	int nro_pagina = 0;
	t_fila_tabla_pagina * tabla =  (t_fila_tabla_pagina *) list_get(lista_tablas, pid);
	txt_write_in_file(dump_file, "Estructuras de memoria:\n\nTABLA DE PAGINAS DE PROCESO: ");
	txt_write_in_stdout("Estructuras de memoria:\n\nTABLA DE PAGINAS DE PROCESO: ");
	char * numero = malloc(sizeof(int));
	char * pagina = string_new();
	numero = (char*)string_itoa(pid);
	txt_write_in_file(dump_file,numero);//pid
	txt_write_in_stdout(numero);
	txt_write_in_file(dump_file,"\n--------------------------------------------------------\n|   PAGINA    |  PRESENCIA  | MODIFICADA  |    MARCO    |\n--------------------------------------------------------\n");
	txt_write_in_stdout("\n--------------------------------------------------------\n|   PAGINA    |  PRESENCIA  | MODIFICADA  |    MARCO    |\n--------------------------------------------------------\n");
	while(tabla[nro_pagina].pid != 0){
		string_append(&pagina,"|      ");
		numero = (char*)string_itoa(tabla[nro_pagina].numero_pagina);
		string_append(&pagina, numero);//pagina
		string_append(&pagina, "      |      ");
		numero = (char*)string_itoa(tabla[nro_pagina].presencia);
		string_append(&pagina, numero);//presencia
		string_append(&pagina, "      |      ");
		numero = (char*)string_itoa(tabla[nro_pagina].modificado);
		string_append(&pagina, numero);//modficada
		string_append(&pagina, "      |      ");
		numero = (char*)string_itoa(tabla[nro_pagina].frame);
		string_append(&pagina, numero);//marco
		string_append(&pagina, "      |");
		txt_write_in_file(dump_file,pagina);
		txt_write_in_stdout(pagina);
		txt_write_in_file(dump_file,"\n--------------------------------------------------------\n");
		txt_write_in_stdout("\n--------------------------------------------------------\n");
		nro_pagina++;
	}
	free(numero);
}

void dump_contenido(int pid){
	int nro_pagina = 0;
	int direccion_mp;
	t_fila_tabla_pagina * tabla =  (t_fila_tabla_pagina *) list_get(lista_tablas, pid);
	txt_write_in_file(dump_file, "Contenido de memoria:\n\nPROCESO: ");
	txt_write_in_stdout("Contenido de memoria:\n\nPROCESO: ");
	char * numero = malloc(sizeof(int));
	char * pagina = malloc(sizeof(configuracion->marco_size));
	numero = (char*)string_itoa(pid);
	txt_write_in_file(dump_file,numero);
	txt_write_in_stdout(numero);
	while((tabla + nro_pagina) != NULL){
		if(tabla[nro_pagina].presencia){
			txt_write_in_file(dump_file, "\nPAGINA: ");
			txt_write_in_stdout("\nPAGINA: ");
			numero = (char*)string_itoa(tabla[nro_pagina].numero_pagina);
			txt_write_in_file(dump_file, numero);
			txt_write_in_stdout(numero);
			direccion_mp = retornar_direccion_mp(tabla[nro_pagina].frame);
			memcpy(pagina,direccion_mp,configuracion->marco_size);
			txt_write_in_file(dump_file,pagina);
			txt_write_in_stdout(pagina);
		}
		nro_pagina++;
	}
	free(pagina);
	free(numero);
}

void guardar_cant_entradas(int pid,int cant_pag){
	t_tabla_cantidad_entradas * tabla_cant = malloc(sizeof(t_tabla_cantidad_entradas));
	tabla_cant->cant_paginas = cant_pag;
	tabla_cant->pid = pid;
	list_add(lista_tabla_entradas,tabla_cant);
} //TODO: ver de eliminarlas al finalizar pid

void liberar_marcos(int pid){
	int nro_pagina = 0;
	int un_marco;
	t_tabla_cantidad_entradas * entradas;
	t_fila_tabla_pagina * tabla =  (t_fila_tabla_pagina*) list_get(lista_tablas,pid);
	bool es_true(t_tabla_cantidad_entradas *elemento){
		return (elemento->pid == pid);
	}
	bool es_marco(t_marco * marco){
		return (marco->numero_marco == un_marco);
	}
	entradas = (t_tabla_cantidad_entradas *) list_find(lista_tabla_entradas,(void*)es_true);
	while(tabla[nro_pagina].numero_pagina < entradas->cant_paginas){
		if (tabla[nro_pagina].presencia){
			un_marco = tabla[nro_pagina].frame;
			tabla[nro_pagina].frame = 0;
			t_marco * marco = (t_marco *) list_find(lista_de_marcos,(void*)es_marco);
			marco->libre = 1;
			log_info(log_umc,"Se libera el marco %d",marco->numero_marco);
			nro_pagina++;
		}
	}
}

void eliminar_programa_nuevo_en_buffer(int pid){
	bool es_buffer(t_programa_completo *un_buffer){
		return (un_buffer->id_programa == pid);
	}
	t_programa_completo * buffer_nuevo_programa = (t_programa_completo *) list_remove_by_condition(lista_buffer_escritura,(void*)es_buffer);
	free(buffer_nuevo_programa);
}

