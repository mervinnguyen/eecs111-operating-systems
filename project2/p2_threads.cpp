#include "p2_threads.h"
#include "utils.h"

extern pthread_cond_t  cond;
extern pthread_mutex_t mutex;

/*-------------------------------------------------------
	person_thread
	One pthread per person. Algorithm:
	1. Call person_wants_to_enter (may block until room state allows entry).
	   Prints [Queue] Send and [Resource Room] entry lines on admission.
	2. Call perform_visit and sleep for the assigned duration (usleep).
	3. Call person_leaves, which prints out [Resource Room] exit line and 
	   signals waiting threads of the appropriate group.
-------------------------------------------------------*/

void person_wants_to_enter(Person &p, ResourceRoom &room) {
	if (p.get_role() == ROLE_FACULTY) {
		room.faculty_wants_to_enter(p);
	} else {
		room.student_wants_to_enter(p);
	}
}

void person_leaves(Person &p, ResourceRoom &room) {
	if (p.get_role() == ROLE_FACULTY) {
		room.faculty_leaves(p);
	} else {
		room.student_leaves(p);
	}
}

void perform_visit(Person &p) {
	p.start();
	usleep(MSEC(p.get_time()));
	p.complete();
}


void *person_thread(void *arg) {
	ThreadArg *targ = (ThreadArg *)arg;
	Person *p = targ->person;
	ResourceRoom *room = targ->room;

	person_wants_to_enter(*p, *room);
	perform_visit(*p);
	person_leaves(*p, *room);

	return NULL;
}