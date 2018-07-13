#include <stdio.h>
#include <stdlib.h>

#include "commons/config.h"
#include "commons/error.h"
#include "commons/log.h"
#include "commons/string.h"
#include "commons/collections/list.h"

t_config * planner_config;
t_config * coordinator_config;
t_config * esi_config;


int main(void) {
	char * buffer = malloc(128 * sizeof(char));
	char * ip_coord_buffer = malloc(128 * sizeof(char));
	int instances;
	planner_config = config_create("../../planner/src/config.cfg");
	coordinator_config = config_create("../../coordinator/src/config.cfg");
	esi_config = config_create("../../esi/src/config.cfg");

	printf("## BRONTOSAURUS INICIADO ##\n");
	printf("# COORDINATOR IP\n> ");
	scanf("%s", ip_coord_buffer);
	config_set_value(planner_config, "COORD_IP", ip_coord_buffer);
	config_set_value(esi_config, "COORD_IP", ip_coord_buffer);
	printf("Cargado %s\n", buffer);

	printf("# PLANNER IP\n> ");
	scanf("%s", buffer);
	config_set_value(esi_config, "PLANNER_IP", buffer);
	printf("Cargado %s\n", buffer);

	printf("## CONFIGURACIONES DEL PLANNER ##\n");
	printf("# PLAN_ALG\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "PLAN_ALG", buffer);
	printf("Cargado %s\n", buffer);

	printf("# ALPHA\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "ALPHA", buffer);
	printf("Cargado %s\n", buffer);

	printf("# INIT_EST\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "INIT_EST", buffer);
	printf("Cargado %s\n", buffer);

	printf("# BLOCKED_KEYS (Con comas, sin espacios, [] si es vacía)\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "BLOCKED_KEYS", buffer);
	printf("Cargado %s\n", buffer);

	printf("## CONFIGURACIONES DEL COORDINADOR ##\n");
	printf("# DIST_ALG\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "DIST_ALG", buffer);
	printf("Cargado %s\n", buffer);

	printf("# ENTRY_COUNT\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "ENTRY_COUNT", buffer);
	printf("Cargado %s\n", buffer);

	printf("# ENTRY_SIZE\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "ENTRY_SIZE", buffer);
	printf("Cargado %s\n", buffer);

	printf("# DELAY\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "DELAY", buffer);
	printf("Cargado %s\n", buffer);

	printf("## CANTIDAD DE INSTANCIAS\n> ");
	scanf("%s", buffer);
	instances = atoi(buffer);
	for(int i = 1; i <= instances ; i++){
		t_config * instance_config;
		char * path = malloc(128 * sizeof(char));
		if(i==1){
			printf("asd");
			sprintf(path, "../../instance/src/config.cfg");
			printf("asd");
		}else{
			sprintf(path, "../../instance/src/config%d.cfg", i);
		}
		instance_config = config_create(path);

		config_set_value(instance_config, "COORD_IP", ip_coord_buffer);
		printf("## CONFIGURACIONES INSTANCIA %d ##\n", i);
		printf("# DUMP\n> ");
		scanf("%s", buffer);
		config_set_value(instance_config, "DUMP", buffer);
		printf("Cargado %s\n", buffer);

		printf("# REPLACEMENT_ALG\n> ");
		scanf("%s", buffer);
		config_set_value(instance_config, "REPLACEMENT_ALG", buffer);
		printf("Cargado %s\n", buffer);

		config_save(instance_config);
		free(path);
		config_destroy(instance_config);
	}

	config_save(coordinator_config);
	config_save(esi_config);;
	config_save(planner_config);
	free(buffer);
	free(ip_coord_buffer);
	return EXIT_SUCCESS;

}


