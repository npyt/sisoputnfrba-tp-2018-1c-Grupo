#include "libgrupo/headers.h"

t_log * logger;
t_config * config;

int main() {
	logger = log_create("esi_logger.log", "ESI", true, LOG_LEVEL_TRACE);
	config = config_create("esi_config.cfg");

	return 0;
}
