#ifndef INCLUDE_HEADERS_H_
#define INCLUDE_HEADERS_H_

typedef struct {
	char * name;
	int socket;
	enum {
		AVAILABLE,
		UNAVAIBLABLE
	} status;
} InstanceRegistration;

void * w_thread(int a);

#endif /* INCLUDE_HEADERS_H_ */
