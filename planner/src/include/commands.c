#include "commands.h"

#include <stdio.h>
#include <stdlib.h>

//Falta implementacion
void pause(){};
void resume(){};
void block(char* key, char* id){};
void unblock(char* key){};
void list(char* resource){};
void kill(char* id){};
void status(char* key){};
void deadlock(){};


void info(){
    printf("pausar/continuar: El Planificador no le dará nuevas órdenes de ejecución a ningún ESI mientras se encuentre pausado.\n");

    printf("bloquear <clave> <ID>: Se bloqueará el proceso ESI hasta ser desbloqueado (ver más adelante), especificado por dicho ID(^3) en la cola del recurso clave. Vale recordar que cada línea del script a ejecutar es atómica, y no podrá ser interrumpida; si no que se bloqueará en la próxima oportunidad posible. Solo se podrán bloquear de esta manera ESIs que estén en el estado de listo o ejecutando.\n");

    printf("desbloquear <clave>: Se desbloqueara el primer proceso ESI bloquedo por la clave especificada. Solo se bloqueará ESIs que fueron bloqueados con la consola. Si un ESI está bloqueado esperando un recurso, no podrá ser desbloqueado de esta forma.\n");

    printf("listar <recurso>: Lista los procesos encolados esperando al recurso.\n");

    printf("kill <ID>: finaliza el proceso. Recordando la atomicidad mencionada en “bloquear”.\n");

    printf("status <clave>: Debido a que para la correcta coordinación de las sentencias de acuerdo a los algoritmos de distribución(^4) se requiere de cierta información sobre las instancias del sistema.\n");

    printf("deadlock: Esta consola también permitirá analizar los deadlocks que existan en el sistema y a que ESI están asociados. Pudiendo resolverlos manualmente con la sentencia de kill previamente descrita.\n");
}
