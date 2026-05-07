#include "types_p2.h"
#include "utils.h"

extern struct timeval t_global_start;

/*Helper functions*/
const char *get_role_str(int role) {
	return (role == ROLE_FACULTY) ? "Faculty Member" : "Student";
}

void print_admission_status(const char *role_str, Person &p, int fac_q, int stu_q, int fac_in, int stu_in, long time_ms, int new_status) {
	const char *state_str = (new_status == FACULTYINSIDE) ? "FacultyInside" : "StudentInside";

	printf("[%02ld ms][Queue] Send (%s) into the resource room "
			"(Stay %ld ms), Status: Total: %d (Faculty: %d, Students: %d)\n",
			 time_ms, role_str, p.get_time(), fac_q + stu_q, fac_q, stu_q);

	printf("[%02ld ms][Resource Room] (%s) goes into the resource room, "
			"State is (%s): Total: %d (Faculty: %d, Students: %d)\n",
			 time_ms, role_str, state_str, fac_in + stu_in, fac_in, stu_in);
}

void print_exit_status(const char *role_str, int fac_in, int stu_in, long time_ms, int new_status) {
	const char *state_str = (new_status == FACULTYINSIDE) ? "FacultyInside" :
							(new_status == STUDENTINSIDE) ? "StudentInside" :
							"Empty";

	printf("[%02ld ms][Resource Room] (%s) left the resource room. "
			"Status is changed, Status is (%s) : Total: %d (Faculty: %d, Students: %d)\n",
			 time_ms, role_str, state_str, fac_in + stu_in, fac_in, stu_in);
}

void print_exit_status_same(const char *role_str, int fac_in, int stu_in, long time_ms, int current_status) {
	const char *state_str = (current_status == FACULTYINSIDE) ? "FacultyInside" :
							(current_status == STUDENTINSIDE) ? "StudentInside" :
							"Empty";

	printf("[%02ld ms][Resource Room] (%s) left the resource room. "
			"State is (%s) : Total: %d (Faculty: %d, Students: %d)\n",
			 time_ms, role_str, state_str, fac_in + stu_in, fac_in, stu_in);
}

/*Person Implementation*/
Person::Person() {
	gettimeofday(&t_create, NULL);
	role = ROLE_STUDENT;
	time_to_stay_ms = 0;
	order = 0;
	use_order = 0;
}

void Person::set_role(int data) { role = data; }
int Person::get_role(void)      { return role; }

void Person::set_order(unsigned long data) { order = data; }
unsigned long Person::get_order(void)      { return order; }

void Person::set_use_order(unsigned long data) { use_order = data; }
unsigned long Person::get_use_order(void)      { return use_order; }

void Person::set_time(long data) { time_to_stay_ms = data; }
long Person::get_time(void) { return time_to_stay_ms; }

int Person::ready_to_leave(void) {
	struct timeval t_curr;
	gettimeofday(&t_curr, NULL);
	return (get_elapsed_time(t_start, t_curr) >= time_to_stay_ms) ? 1 : 0;
}

void Person::start(void) {
	gettimeofday(&t_start, NULL);
}

void Person::complete(void) {
	gettimeofday(&t_end, NULL);
}

/* ResourceRoom Implementation*/
ResourceRoom::ResourceRoom() {
	status = EMPTY;
	faculty_inside = 0;
	students_inside = 0;
	faculty_queued = 0;
	students_queued = 0;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&faculty_cond, NULL);
	pthread_cond_init(&student_cond, NULL);
}

ResourceRoom::~ResourceRoom() {
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&faculty_cond);
	pthread_cond_destroy(&student_cond);
}

/* enqueue faculty / enqueue student 
*	Called by the main thread before pthread_create.
*	Registers the person in the logical queue under the lock
*	so counts are always consistent.
*/
void ResourceRoom::enqueue_faculty(void) {
	pthread_mutex_lock(&mutex);
	faculty_queued++;
	pthread_mutex_unlock(&mutex);
}

void ResourceRoom::enqueue_student() {
	pthread_mutex_lock(&mutex);
	students_queued++;
	pthread_mutex_unlock(&mutex);
}

/* get_queue_counts (snapshot for main's [Queue] line)*/
void ResourceRoom::get_queue_counts(int *total, int *fac, int *stu) {
	pthread_mutex_lock(&mutex);
	*fac = faculty_queued;
	*stu = students_queued;
	*total = *fac + *stu;
	pthread_mutex_unlock(&mutex);
}

// You need to use this function to print the ResourceRoom's status
void ResourceRoom::print_status(void) {
	pthread_mutex_lock(&mutex);
	struct timeval t_now;
	gettimeofday(&t_now, NULL);
	long time_ms = get_elapsed_time(t_global_start, t_now);
	const char *s = (status == FACULTYINSIDE) ? "FacultyInside" :
					(status == STUDENTINSIDE) ? "StudentInside" :
					"Empty";

	printf("[%02ld ms][Resource Room] Status is (%s): "
			"Total: %d (Faculty: %d, Students: %d)\n",
			time_ms, s, faculty_inside + students_inside, faculty_inside, students_inside);
	pthread_mutex_unlock(&mutex);
}

void ResourceRoom::add_person(Person& p) {
	if (p.get_role() == ROLE_FACULTY) {
		faculty_inside++;
	} else {
		students_inside++;
	}
}

void ResourceRoom::faculty_wants_to_enter(Person& p) {
	// TODO: implement faculty member entry synchronization.
	struct timeval t_now;
	long time_ms;
	pthread_mutex_lock(&mutex);

	/*Block while student are using the room*/
	while (status == STUDENTINSIDE) {
		pthread_cond_wait(&faculty_cond, &mutex);
	}

	/*Admitted: move from queue into room*/
	faculty_queued--;
	faculty_inside++;
	status = FACULTYINSIDE;

	gettimeofday(&t_now, NULL);
	time_ms = get_elapsed_time(t_global_start, t_now);

	/* [Queue] Send line : printed the moment this thread is admitted*/
	print_admission_status(get_role_str(ROLE_FACULTY), p, faculty_queued, students_queued, faculty_inside,
						   students_inside, time_ms, FACULTYINSIDE);
	
	pthread_mutex_unlock(&mutex);
}

void ResourceRoom::student_wants_to_enter(Person& p) {
	// TODO: implement student entry synchronization.
	struct timeval t_now;
	long time_ms;

	pthread_mutex_lock(&mutex);

	/*Block while faculty are using the room*/
	while(status == FACULTYINSIDE) {
		pthread_cond_wait(&student_cond, &mutex);
	}

	/*Admitted: move from queue into the room*/
	students_queued--;
	students_inside++;
	status = STUDENTINSIDE;

	gettimeofday(&t_now, NULL);
	time_ms = get_elapsed_time(t_global_start, t_now);

	/*[Queue] Send line*/
	print_admission_status(get_role_str(ROLE_STUDENT), p, faculty_queued, students_queued, faculty_inside,
						   students_inside, time_ms, STUDENTINSIDE);
	
	pthread_mutex_unlock(&mutex);
}

void ResourceRoom::faculty_leaves(Person& p) {
	// TODO: implement faculty member exit synchronization.
	struct timeval t_now;
	long time_ms;

	pthread_mutex_lock(&mutex);

	faculty_inside--;

	gettimeofday(&t_now, NULL);
	time_ms = get_elapsed_time(t_global_start, t_now);

	if(faculty_inside == 0) {
		/* Last faculty out : room empties*/
		status = EMPTY;
		if (students_queued > 0) {
			/* Students waiting -> hand room to them*/
			print_exit_status(get_role_str(ROLE_FACULTY), faculty_inside, students_inside, time_ms, EMPTY);
			pthread_cond_broadcast(&student_cond);
		} else if (faculty_queued > 0) {
			/*More faculty waiting (room is shared, some are blocked)*/
			print_exit_status(get_role_str(ROLE_FACULTY), faculty_inside, students_inside, time_ms, EMPTY);
			pthread_cond_broadcast(&faculty_cond);
		} else {
			/* No one currently waiting*/
			print_exit_status(get_role_str(ROLE_FACULTY), faculty_inside, students_inside, time_ms, EMPTY);
		}
		} else {
			/* Other faculty still inside */
			print_exit_status_same(get_role_str(ROLE_FACULTY), faculty_inside, students_inside, time_ms, FACULTYINSIDE);
		}
	pthread_mutex_unlock(&mutex);
}

void ResourceRoom::student_leaves(Person& p) {
	// TODO: implement student exit synchronization.
	struct timeval t_now;
	long time_ms;

	pthread_mutex_lock(&mutex);
	students_inside--;
	gettimeofday(&t_now, NULL);
	time_ms = get_elapsed_time(t_global_start, t_now);

	if(students_inside == 0) {
		/* Last student out -> room empties*/
		status = EMPTY;

		if(faculty_queued > 0) {
			/*Faculty waiting -> hand room to them*/
			print_exit_status(get_role_str(ROLE_STUDENT), faculty_inside, students_inside, time_ms, EMPTY);
			pthread_cond_broadcast(&faculty_cond);
		} else if (students_queued > 0) {
			/* More students queued */
			print_exit_status(get_role_str(ROLE_STUDENT), faculty_inside, students_inside, time_ms, EMPTY);
			pthread_cond_broadcast(&student_cond);
		} else {
			/* Empty */
			print_exit_status(get_role_str(ROLE_STUDENT), faculty_inside, students_inside, time_ms, EMPTY);
		}
	} else {
		/* Other students are still inside */
		print_exit_status_same(get_role_str(ROLE_STUDENT), faculty_inside, students_inside, time_ms, STUDENTINSIDE);
	}
	pthread_mutex_unlock(&mutex);
}
