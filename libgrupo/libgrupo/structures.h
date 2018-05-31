#ifndef STRUCTURES
#define STRUCTURES

#define RESOURCE_KEY_MAX_SIZE 40
#define ESI_NAME_MAX_SIZE 64

typedef enum {
	STATUS_BLOCKED,
	STATUS_RUNNING,
	STATUS_FINISHED,
	STATUS_READY,
	STATUS_NEW
} ESIStatus;

typedef struct {
	char id[ESI_NAME_MAX_SIZE];
	int program_counter;
	ESIStatus status;
	float last_estimate;
	int last_job;
	int idle_counter;
} ESI;

typedef enum {
	FIFO,
	SJF_SD,
	SJF_CD,
	HRRN
} PlannerAlgorithm;



typedef struct {
	char key[RESOURCE_KEY_MAX_SIZE];
	void * address;
	int size;
	int owner_instance;
} Resource;



typedef enum {
	CIRC,
	LRU,
	BSY
} ReplacementAlgorithm;

typedef enum {
	LSU,
	EL,
	KE
} DistributionAlgorithm;

typedef struct {
	int entry_number;
	char * data;
	int data_size;
} EntryBlock;

typedef struct {
	int entry_number;
	char key[RESOURCE_KEY_MAX_SIZE];
	int size;
	int blocked;
	char blockedBy[ESI_NAME_MAX_SIZE];
} DiccionaryEntry;

typedef struct {
	char name[255];
	ReplacementAlgorithm algorithm;
	char mounting_point[255];
	int dump;
	int socket;
	DiccionaryEntry * entry_table_fst;
} Instance;

typedef struct {
	int entry_count;
	int entry_size;
} InstanceInitConfig;



typedef enum {
	GET_OP,
	SET_OP,
	STORE_OP
} InstructionOperation;

typedef struct {
	char key[RESOURCE_KEY_MAX_SIZE];
	InstructionOperation operation;
	char * opt_value;
	char ESIName[ESI_NAME_MAX_SIZE];
} InstructionDetail;



typedef enum {
	BLOCKED,
	WAITING,
	RELEASED
} ResourceAllocationStatus;

typedef struct {
	char ESIName[ESI_NAME_MAX_SIZE];
	char key[RESOURCE_KEY_MAX_SIZE];
	ResourceAllocationStatus status;
} ResourceAllocation;

#endif /* STRUCTURES */
