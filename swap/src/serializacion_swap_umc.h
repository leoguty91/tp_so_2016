/*
 * serializacion_umc_swap.h
 *
 *  Created on: 14/5/2016
 *      Author: utnso
 */

#ifndef SERIALIZACION_UMC_H_
#define SERIALIZACION_UMC_H_


typedef struct {
	int id_programa;
} t_programa;

typedef struct {
	int id_programa;
	int paginas_requeridas;
	char *codigo;
} t_programa_completo;

typedef struct {
	int id_programa;
	int pagina;
	int offset;
	int tamanio;
	int socket_pedido;
} t_pagina;

typedef struct {
	int tamanio;
} t_pagina_tamanio;

typedef struct {
	int id_programa;
	int pagina;
	int offset;
	int tamanio;
	void *valor;
	int socket_pedido;
} t_pagina_completa;

void deserializar_programa_completo(void *buffer, t_programa_completo *programa);

void deserializar_pagina(void *buffer, t_pagina *pagina);

void deserializar_pagina_completa(void *buffer, t_pagina_completa *pagina);

void deserializar_programa(void *buffer, t_programa *programa);

void deserializar_pagina_tamanio(void *buffer, t_pagina_tamanio *pagina_tamanio);

#endif /* SERIALIZACION_UMC_H_ */