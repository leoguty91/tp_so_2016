/*
 ============================================================================
 Name        : consola.c
 Author      : losmallocados
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "consola.h"

int main(int argc, char **argv) {


	FILE *archivo;
	char *buffer, ** Aux_Buff;
	int Largo_Buffer;
	struct sigaction sa;
	t_config_consola *configuracion = malloc(sizeof(t_config_consola)); // Estructura de configuracion de la consola
	cargaConfiguracionConsola("src/config.consola.ini", configuracion);
	int socket_nucleo = conectar_servidor(configuracion->ip, configuracion->puerto);

	sa.sa_handler = sigchld_handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}

	printf("Proceso Consola creado.\n");

	if (socket_nucleo > 0) {
			printf("Consola conectada con el Núcleo.\n");
			handshake_consola_nucleo(socket_nucleo);
		} else {
			perror("Error al conectarse con la Núcleo\n");
		}

	// Lee Archivo y envia el archivo Ansisop
	//archivo = fopen(argv[1], "r"); //El argv[1] tiene la direccion del archivo Ansisop
	archivo = fopen("./Programa", "r");

	if (archivo == NULL)   exit(1);

	//Envia linea por linea el archivo Ansisop
	//Largo_Buffer = Generar_Buffer_Programa(archivo,Aux_Buff);

	//buffer = *Aux_Buff;

	//printf ("\n\n\n\nEl Codigo linea: %s \n\n\n\n",buffer);
	//enviar_codigo (buffer,Largo_Buffer,socket_nucleo);
	enviar_codigo (archivo,socket_nucleo);

	getchar();

	fclose( archivo);
	close(socket_nucleo);
	free(configuracion);
	free (buffer);
	return EXIT_SUCCESS;
}

void cargaConfiguracionConsola(char *archivo, t_config_consola *configuracionConsola) {
	t_config *configuracion = malloc(sizeof(t_config));
	configuracion = config_create(archivo);
	if (config_has_property(configuracion, "PUERTO_NUCLEO")) {
		configuracionConsola->puerto = config_get_int_value(configuracion, "PUERTO_NUCLEO");
	} else {
		configuracionConsola->puerto = DEF_PUERTO_Nucleo;
	}
	if (config_has_property(configuracion, "IP_NUCLEO")) {
		configuracionConsola->ip = config_get_string_value(configuracion, "IP_NUCLEO");
	} else {
		configuracionConsola->ip = DEF_IP_Nucleo;
	}
	free(configuracion);
}

int Generar_Buffer_Programa(FILE *archivo,char **Aux_Buff) {
	char Aux_Archivo[100];
	int len, aux_len = 0;
	char * buffer;

	fseek(archivo, 0, SEEK_END);
	len=ftell(archivo);
	rewind (archivo);

	buffer = malloc (len);
	*buffer = NULL;

	while (feof(archivo) == 0){

		fgets(Aux_Archivo,100,archivo);

		strcat (buffer,Aux_Archivo);
		//strcpy (Aux_buffer,buffer);

	}

	printf ("\n\n\n\nEl Codigo linea: %s \n\n\n\n",buffer);
	*Aux_Buff = buffer;

	return (len);
}

void handshake_consola_nucleo(int socket_nucleo) {
	t_header *header = malloc(sizeof(t_header));
	header->id_proceso_emisor = PROCESO_CONSOLA;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = MENSAJE_HANDSHAKE;
	header->longitud_mensaje = 0;

	enviar_header(socket_nucleo, header);
	free(header);
}

void enviar_codigo (FILE * archivo, int socket_nucleo){
	t_header *header = malloc(sizeof(t_header));
	t_buffer * p_buffer;

	char Aux_Archivo[100];
	int Largo_Mensaje;
	char * buffer;

	fseek(archivo, 0, SEEK_END);
	Largo_Mensaje=ftell(archivo);
	rewind (archivo);

	buffer = malloc (Largo_Mensaje);
	*buffer = NULL;

	while (feof(archivo) == 0){

		fgets(Aux_Archivo,100,archivo);

		strcat (buffer,Aux_Archivo);

	}

	printf ("\n\n\n\nEl Codigo linea: %s \n\n\n\n",buffer);

	p_buffer = serializar_imprimir_texto (buffer);

	header->id_proceso_emisor = PROCESO_CONSOLA;
	header->id_proceso_receptor = PROCESO_NUCLEO;
	header->id_mensaje = CODIGO;
	header->longitud_mensaje = p_buffer->longitud_buffer;
	//header->longitud_mensaje = Largo_Mensaje;

	if (enviar_buffer(socket_nucleo, header, buffer)
				< sizeof(t_header) + Largo_Mensaje) {
			perror("Fallo enviar buffer");
	}
	free(buffer);
	free(header);
}

t_buffer *serializar_imprimir_texto(t_texto *texto) {
	int cantidad_a_reservar = sizeof(int) + strlen(texto->texto);
	void *buffer = malloc(cantidad_a_reservar);

	int posicion_buffer = 0;

	copiar_string_en_buffer(buffer, texto->texto, &posicion_buffer);

	t_buffer *estructura_buffer = malloc(sizeof(t_buffer));
	estructura_buffer->contenido_buffer = buffer;
	estructura_buffer->longitud_buffer = posicion_buffer;

	return (estructura_buffer);
}

void sigchld_handler(int s) {

}



