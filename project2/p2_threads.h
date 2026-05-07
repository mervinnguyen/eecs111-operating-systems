#ifndef __P2_THREADS_H
#define __P2_THREADS_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include "types_p2.h"

/*Argument bundle passed to each per-person thread */
struct ThreadArg {
    Person *person;
    ResourceRoom *room;
};

void *person_thread(void *arg);

#endif
