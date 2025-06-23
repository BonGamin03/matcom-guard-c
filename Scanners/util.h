#ifndef UTIL_H
#define UTIL_H
#include <pthread.h>
#include <stdio.h>

extern pthread_mutex_t mutex_alertas;

void Write_Alert(const char* type_scanner,const char* message);

void inicializar_alertas_guard(void); 

void inicializar_mutex_alertas();

void limpiar_usb_baseline(void);

void destruir_mutex_alertas();

void destruir_alertas_guard(void);

#endif