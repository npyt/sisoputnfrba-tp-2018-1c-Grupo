#include "libgrupo/headers.h"
#include <parsi/parser.h>

int main(int argc, char **argv){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

	t_log * logger;
	t_config * config;

	logger = log_create("esi_logger.log", "ESI", true, LOG_LEVEL_TRACE);
	config = config_create("esi_config.cfg");

    fp = fopen(argv[1], "r");
    if (fp == NULL){
        perror("Error al abrir el archivo: ");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
    	t_esi_operacion parsed = parse(line);

        if(parsed.valido){
            switch(parsed.keyword){
                case GET:
                    printf("GET\tclave: <%s>\n", parsed.argumentos.GET.clave);
                    break;
                case SET:
                    printf("SET\tclave: <%s>\tvalor: <%s>\n", parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
                    break;
                case STORE:
                    printf("STORE\tclave: <%s>\n", parsed.argumentos.STORE.clave);
                    break;
                default:
                    fprintf(stderr, "No pude interpretar <%s>\n", line);
                    exit(EXIT_FAILURE);
            }

            destruir_operacion(parsed);
        } else {
            fprintf(stderr, "La linea <%s> no es valida\n", line);
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);
    if (line)
        free(line);

    return EXIT_SUCCESS;
}
