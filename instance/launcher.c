#include "headers.h"

t_log * logger;
t_config * config;

int main() {
	logger = log_create("instance_logger.log", "INSTANCE", true, LOG_LEVEL_TRACE);
	config = config_create("instance_config.cfg");

	return 0;
}
