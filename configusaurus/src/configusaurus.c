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
	char * buffer_name = malloc(128 * sizeof(char));
	int instances;
	planner_config = config_create("../../planner/src/config.cfg");
	coordinator_config = config_create("../../coordinator/src/config.cfg");
	esi_config = config_create("../../esi/src/config.cfg");

	printf("### [WELCOME_TO_BRONTOSAURUS] ###\n");
	printf("# [COORDINATOR_IP]\n> ");
	scanf("%s", ip_coord_buffer);
	config_set_value(planner_config, "COORD_IP", ip_coord_buffer);
	config_set_value(esi_config, "COORD_IP", ip_coord_buffer);
	printf("[LOADED_%s]\n", ip_coord_buffer);

	printf("# [PLANNER_IP]\n> ");
	scanf("%s", buffer);
	config_set_value(esi_config, "PLANNER_IP", buffer);
	printf("[LOADED_%s]\n", buffer);

	printf("### [PLANNER_CONFIG] ###\n");
	printf("# [PLAN_ALG]\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "PLAN_ALG", buffer);
	printf("[LOADED_%s]\n", buffer);

	printf("# [ALPHA]\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "ALPHA", buffer);
	printf("[LOADED_%s]\n", buffer);

	printf("# [INIT_EST]\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "INIT_EST", buffer);
	printf("[LOADED_%s]\n", buffer);

	printf("# [BLOCKED_KEYS]\n> ");
	scanf("%s", buffer);
	config_set_value(planner_config, "BLOCKED_KEYS", buffer);
	printf("[LOADED_%s]\n", buffer);

	printf("### [COORDINATOR_CONFIG] ###\n");
	printf("# [DIST_ALG]\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "DIST_ALG", buffer);
	printf("[LOADED_%s]\n", buffer);

	printf("# [ENTRY_COUNT]\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "ENTRY_COUNT", buffer);
	printf("Cargado %s\n", buffer);

	printf("# [ENTRY_SIZE]\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "ENTRY_SIZE", buffer);
	printf("Cargado %s\n", buffer);

	printf("# [DELAY]\n> ");
	scanf("%s", buffer);
	config_set_value(coordinator_config, "DELAY", buffer);
	printf("Cargado %s\n", buffer);

	printf("## [NUMBER_OF_INSTANCES]\n> ");
	scanf("%s", buffer);
	instances = atoi(buffer);
	printf("[LOADED_%s_INSTANCES]\n", buffer);

	printf("## [INSTANCE_GENERIC_NAME]\n> ");
	scanf("%s", buffer_name);
	printf("[LOADED_%s]\n", buffer_name);

	for(int i = 1; i <= instances ; i++){
		t_config * instance_config;
		char * path = malloc(128 * sizeof(char));
		if(i==1){
			sprintf(path, "../../instance/src/config.cfg");
		}else{
			sprintf(path, "../../instance/src/config%d.cfg", i);
		}
		if(!fopen(path, "w")){
		}
		instance_config = config_create(path);
		sprintf(buffer, "%s%d", buffer_name, i);
		config_set_value(instance_config, "NAME", buffer);
		config_set_value(instance_config, "COORD_PORT", "8000");
		config_set_value(instance_config, "MOUNTING_POINT", "/home/utnso/inst1");
		config_set_value(instance_config, "COORD_IP", ip_coord_buffer);
		printf("### [INSTANCE_NUMBER_%d_CONFIGURATION] ###\n", i);

		printf("# [REPLACEMENT_ALG]\n> ");
		scanf("%s", buffer);
		config_set_value(instance_config, "REPLACEMENT_ALG", buffer);
		printf("[LOADED_%s]\n", buffer);

		printf("# [DUMP]\n> ");
		scanf("%s", buffer);
		config_set_value(instance_config, "DUMP", buffer);
		printf("[LOADED_%s]\n", buffer);

		config_save(instance_config);
		free(path);
		config_destroy(instance_config);
	}

	config_save(coordinator_config);
	config_save(esi_config);;
	config_save(planner_config);
	free(buffer);
	free(buffer_name);
	free(ip_coord_buffer);
	printf("[ESITOSO]");
	return EXIT_SUCCESS;

}


