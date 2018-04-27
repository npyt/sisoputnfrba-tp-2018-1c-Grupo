#ifndef STRUCTURES_H_
#define STRUCTURES_H_

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


typedef enum {
	FIFO,
	SJF_SD,
	SJF_CD,
	HRRN
} PlannerAlgorithm;

#endif
