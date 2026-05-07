#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include "types_p2.h"
#include "p2_threads.h"
#include "utils.h"

using namespace std;

pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct timeval t_global_start;


long get_current_elapsed_time(struct timeval &start) {
	struct timeval t_now;
	gettimeofday(&t_now, NULL);
	return get_elapsed_time(start, t_now);
}

void print_arrival(int role) {
	const char *role_str = (role == ROLE_FACULTY) ? "Faculty Member" : "Student";
	long ms = get_current_elapsed_time(t_global_start);
	printf("[%02ld ms][Input] A person (%s) goes into the queue.\n", ms, role_str);
}

void print_queue_status(ResourceRoom &room) {
	int qtotal, qfac, qstu;
	room.get_queue_counts(&qtotal, &qfac, &qstu);
	long ms = get_current_elapsed_time(t_global_start);
	printf("[%02ld ms][Queue] Waiting queue is not empty."
			" Status: Total: %d (Faculty: %d, Students: %d)\n", ms, qtotal, qfac, qstu);
}

int validate_arguments(int argc, char **argv) {
	if (argc != 2){
		fprintf(stderr, "[ERROR] Expected 1 argument, but got (%d)\n", argc-1);
		fprintf(stderr, "[USAGE] p2_exec <number>\n");
		return -1;
	}
	int N = atoi(argv[1]);
	if (N <= 0){
		fprintf(stderr, "[ERROR] Expected 1 argument, but got (%d)\n", argc - 1);
		fprintf(stderr, "[USAGE] p2_exec <number>\n");
		return -1;
	}
	return N;
}

vector<int> generate_randomized_role_sequence(int N) {
	int total = 2 * N;
	vector<int> roles;
	for (int i = 0; i < N; i++){
		roles.push_back(ROLE_FACULTY);
	}
	for (int i = 0; i < N; i++){
		roles.push_back(ROLE_STUDENT);
	}
	/* Fisher-Yates shuffle*/
	for (int i = total - 1; i > 0; i--){
		int j = rand() % (i + 1);
		swap(roles[i], roles[j]);	
	}
	return roles;
}

void create_and_spawn_person(int i, int role, const vector<int> &roles,ResourceRoom &room, vector<pthread_t> &tids, vector<Person *> &persons, vector<ThreadArg *> &args) {

	Person *p = new Person();
	p->set_order((unsigned long)(i + 1));
	p->set_role(role);

	/* Random stay time : 3-10 ms*/
	long stay_time_ms = 3 + (rand() % 8);
	p->set_time(stay_time_ms);

	print_arrival(role);

	/*Register in the queue (under lock inside ResourceRoom)*/
	if (role == ROLE_FACULTY){
		room.enqueue_faculty();
	} else {
		room.enqueue_student();
	}

	print_queue_status(room);
	
	/*Spawn the per-person thread*/
	ThreadArg *targ = new ThreadArg();
	targ->person = p;
	targ->room = &room;
	persons[i] = p;
	args[i] = targ;

	pthread_create(&tids[i], NULL, person_thread, (void *)targ);

	/*Wait 1-5 ms before generating the next person*/
	int wait_time_ms = 1 + (rand() % 5);
	usleep(MSEC(wait_time_ms));
}

int main(int argc, char** argv)
{
	// This is to set the global start time
	gettimeofday(&t_global_start, NULL);

	int N = validate_arguments(argc, argv);
	if (N == -1) return -1;

	/*Seed RNG*/
	srand((unsigned int)time(NULL));

	vector<int> roles = generate_randomized_role_sequence(N);

	int total = 2 * N;
	vector<pthread_t> tids(total);
	vector<Person *> persons(total);
	vector<ThreadArg *> args(total);

	ResourceRoom room;

	for (int i = 0; i < total; i++){
		create_and_spawn_person(i, roles[i], roles, room, tids, persons, args);
	}

	for (int i = 0; i < total; i++){
		pthread_join(tids[i], NULL);
	}

	long total_time_ms = get_current_elapsed_time(t_global_start);
	printf("[%02ld ms][Summary] Finished all %d people. Total runtime: %ld ms\n", 
            total_time_ms, total, total_time_ms);
	
	for (int i = 0; i < total; i++){
		delete persons[i];
		delete args[i];
	}
	return 0;
}
