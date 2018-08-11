#include <pthread.h>

typedef struct thread_pinner_t thread_pinner_t;

thread_pinner_t * thread_pinner_create();
int pin_thread(thread_pinner_t *thread_pinner, pthread_t thread);