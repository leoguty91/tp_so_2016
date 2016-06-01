/*
 * primitivas_ansisop.h
 *
 *  Created on: 22/4/2016
 *      Author: utnso
 */

#ifndef SRC_PRIMITIVAS_ANSISOP_H_
#define SRC_PRIMITIVAS_ANSISOP_H_

#include <stdio.h>
#include <parser/parser.h>

t_puntero ansisop_definir_variable(t_nombre_variable identificador_variable);
t_puntero ansisop_obtener_posicion_variable(t_nombre_variable indentificador_variable);
t_valor_variable ansisop_derefenciar(t_puntero direccion_variable);
void ansisop_asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable ansisop_obtener_valor_compartida(t_nombre_compartida variable);
t_valor_variable ansisop_asignar_valor_compartida(t_nombre_compartida variable,
		t_valor_variable valor);
void ansisop_ir_a_label(t_nombre_etiqueta etiqueta);
t_puntero_instruccion ansisop_llamar_funcion(t_nombre_etiqueta etiqueta,
		t_puntero donde_retornar, t_puntero_instruccion linea_en_ejecucion);
void ansisop_retornar(t_valor_variable retorno);
void ansisop_imprimir(t_valor_variable valor_mostrar);
void ansisop_imprimir_texto(char* texto);
void ansisop_entrada_salida(t_nombre_dispositivo dispositivo, int tiempo);
void ansisop_wait(t_nombre_semaforo identificador_semaforo);
void ansisop_signal(t_nombre_semaforo identificador_semaforo);

#endif /* SRC_PRIMITIVAS_ANSISOP_H_ */
