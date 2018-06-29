#ifndef CHUCKNORRIS_STRUCTURES_H_
#define CHUCKNORRIS_STRUCTURES_H_

/*Communication Structures*/
#define IP_LENGTH_MAX 1024
typedef enum {
	TEST,

	//Handshakes
	HSK_ESI_COORD,
	HSK_ESI_COORD_OK,
	HSK_ESI_PLANNER,
	HSK_ESI_PLANNER_OK,
	HSK_PLANNER_COORD,
	HSK_PLANNER_COORD_OK,
	HSK_INST_COORD,
	HSK_INST_COORD_OK,
	HSK_INST_COORD_RELOAD,

	INSTRUCTION_ESI_COORD,
	INSTRUCTION_COORD_INST,

	INSTRUCTION_PERMISSION,
	INSTRUCTION_ALLOWED,
	INSTRUCTION_NOT_ALLOWED,
	INSTRUCTION_NOT_ALLOWED_AND_ABORT,

	INSTRUCTION_OK_TO_COORD,
	INSTRUCTION_FAILED_TO_COORD,

	INSTRUCTION_OK_TO_PLANNER,
	INSTRUCTION_FAILED_TO_ESI,

	NEW_RESOURCE_ALLOCATION,

	EXECUTE_NEXT_INSTRUCTION,
	EXECUTE_PREV_INSTRUCTION,

	ESI_FINISHED

} MessageType;

#define MSG_COMMENT_MAX 255
typedef struct {
	MessageType type;
	char comment[MSG_COMMENT_MAX];
} MessageHeader;





/*ESI Structures*/
typedef struct {
	char coord_ip[IP_LENGTH_MAX];
	int coord_port;
	char planner_ip[IP_LENGTH_MAX];
	int planner_port;
	int id;
	int coordinator_socket;
	int planner_socket;
} ESIConfig;

typedef enum {
	SET_OP,
	GET_OP,
	STORE_OP
} InstructionType;

#define KEY_NAME_MAX 40
#define KEY_VALUE_MAX 2048
typedef struct {
	int esi_id;
	char key[KEY_NAME_MAX];
	InstructionType type;
	char opt_value[KEY_VALUE_MAX];
} InstructionDetail;





/*Planner Structures*/
typedef enum {
	FIFO,
	SJF_CD,
	SJF_SD,
	HRRN
} PlanningAlgorithm;

typedef struct {
	int port;
	PlanningAlgorithm planning_alg;
	int alpha;
	int init_est;
	char coord_ip[IP_LENGTH_MAX];
	int coord_port;
	char blocked_keys;
} PlannerConfig;

typedef enum {
	BLOCKED,
	WAITING,
	RELEASED
} AllocationType;

typedef struct {
	char key[KEY_NAME_MAX];
	int esi_id;
	AllocationType type;
} ResourceAllocation;

typedef enum {
	S_READY,
	S_RUNNING,
	S_BLOCKED,
	S_FINISHED
} ESIStatus;

typedef struct {
	int esi_id;
	int socket;
	int rerun_last_instruction;
	ESIStatus status;

	float estimation;
	float response_ratio;
	int job_counter;
	int waiting_counter;
} ESIRegistration;





/*Coordinator Structures*/
typedef enum {
	LSU,
	EL,
	KE
} DistributionAlgorithm;

typedef struct {
	int port;
	DistributionAlgorithm dist_alg;
	int entry_count;
	int entry_size;
	int delay;
	int planner_socket;
} CoordinatorConfig;

typedef struct {
	int entry_count;
	int entry_size;
} InstanceData;

#define INSTANCE_NAME_MAX 2048
typedef struct {
	char name[INSTANCE_NAME_MAX];
	int socket;
	int inf;
	int sup;
} InstanceRegistration;

typedef struct {
	char key[KEY_NAME_MAX];
	InstanceRegistration * instance;
} ResourceRegistration;





/*Instance Structures*/
#define INSTANCE_MOUNTING_POINT_MAX 2048
typedef enum {
	CIRC,
	LRU,
	BSU
} ReplacementAlgorithm;

typedef struct {
	char coord_ip[IP_LENGTH_MAX];
	int coord_port;
	ReplacementAlgorithm replacement_alg;
	char mounting_point[INSTANCE_MOUNTING_POINT_MAX];
	char name[INSTANCE_NAME_MAX];
	int dump;
} InstanceConfig;

typedef struct {
	int id;
	int content_size;
	int atomic_value;
	char * content;
} StorageCell;

typedef struct {
	char key[KEY_NAME_MAX];
	int size;
	int cell_id;
	int cell_count;
} ResourceStorage;

#endif /* CHUCKNORRIS_STRUCTURES_H_ */
