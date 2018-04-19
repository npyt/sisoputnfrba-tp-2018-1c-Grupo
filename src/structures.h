#ifndef STRUCTURES
#define STRUCTURES

typedef enum {
	STATUS_BLOCKED,
	STATUS_RUNNING,
	STATUS_FINISHED,
	STATUS_READY
} ESIStatus;

typedef struct {
	int id;
	int program_counter;
	ESIStatus status;
	float last_estimate;
	int last_job;
	int idle_counter;
} ESI;

#define RESOURCE_KEY_MAX_SIZE 40

typedef struct {
	char key[RESOURCE_KEY_MAX_SIZE];
	void * address;
	int size;
	int owner_instance;
} Resource;

typedef struct {
	int id;
	void * memory_fst_ptr;
} Instance;

typedef enum {
	BLOCKED,
	WAITING,
	RELEASED
} ResourceAllocationStatus;

typedef struct {
	int esi;
	char key[RESOURCE_KEY_MAX_SIZE];
	ResourceAllocationStatus status;
} ResourceAllocation;

#endif /* STRUCTURES */
