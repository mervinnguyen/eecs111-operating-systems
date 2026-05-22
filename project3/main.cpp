#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include "types_p3.h"
#include "p3_threads.h"
#include "utils.h"

// pthread conditional variable to start/resume the thread
pthread_cond_t resume[4];

// pthread conditional variable to wait for threads to finish init
pthread_cond_t init[4];

// pthread conditional variable to signal when the thread's task is done
pthread_cond_t a_task_is_done;

// pthread conditional variable to allow the scheduler to wait for a thread to preempt
pthread_cond_t preempt_task;

// tcbs for each thread
ThreadCtrlBlk tcb[4];

// ready queue of threads
std::vector<int> ready_queue;

// number of tasks that did not miss deadline
int num_of_alive_tasks = 4;

// -1 = no thread working, <number> = thread <number> currently working
int running_thread = -1;

// 0 = don't preempt, 1 = preempt current running thread
int preempt = 0;

// mutex used to protect variables defined in this file
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// mutex used for the task done pthread conditional variable
pthread_mutex_t taskDoneMutex = PTHREAD_MUTEX_INITIALIZER;

// marks the "start time"
struct timeval t_global_start;

// used to tell threads when to stop working (after 240 iterations)
int global_work = 0;

void fifo_schedule(void);
void edf_schedule(void);
void rm_schedule(void);

int main(int argc, char** argv)
{
	char *endptr = NULL;
	long schedule_arg = -1;

	if (argc == 2) {
		schedule_arg = strtol(argv[1], &endptr, 10);
	}

	if(argc != 2 || argv[1][0] == '\0' || *endptr != '\0' || schedule_arg < 0 || schedule_arg > 2)
	{
		std::cout << "[ERROR] Expecting 1 argument, but got " << argc-1 << std::endl;
		std::cout<< "[USAGE] p3_exec <0, 1, or 2>" << std::endl;
		return 0;
	}
	int schedule = (int)schedule_arg;

	// pthreads we are creating
	pthread_t tid[4];

	// This is to set the global start time
	gettimeofday(&t_global_start, NULL);

	// initialize all tcbs
	tcb[0].id = 0;
	tcb[0].start_offset = 0;
	tcb[0].task_time = 500;
	tcb[0].period = 3000;
	tcb[0].relative_deadline = 1700;
	tcb[0].deadline = tcb[0].start_offset + tcb[0].relative_deadline;

	tcb[1].id = 1;
	tcb[1].start_offset = 800;
	tcb[1].task_time = 400;
	tcb[1].period = 1000;
	tcb[1].relative_deadline = 1000;
	tcb[1].deadline = tcb[1].start_offset + tcb[1].relative_deadline;

	tcb[2].id = 2;
	tcb[2].start_offset = 200;
	tcb[2].task_time = 200;
	tcb[2].period = 1500;
	tcb[2].relative_deadline = 600;
	tcb[2].deadline = tcb[2].start_offset + tcb[2].relative_deadline;

	tcb[3].id = 3;
	tcb[3].start_offset = 700;
	tcb[3].task_time = 400;
	tcb[3].period = 4000;
	tcb[3].relative_deadline = 3000;
	tcb[3].deadline = tcb[3].start_offset + tcb[3].relative_deadline;

	// initialize all pthread conditional variables
	for (int i = 0; i < 4; i++) {
		pthread_cond_init(&resume[i], NULL);
		pthread_cond_init(&init[i], NULL);
	}
	pthread_cond_init(&a_task_is_done, NULL);
	pthread_cond_init(&preempt_task, NULL);

	printf("[Main] Create worker threads\n");

	// create pthreads and pass their respective tcb as a parameter
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < 4; i++) {
		if(pthread_create(&tid[i], NULL, threadfunc, &tcb[i])) {
			fprintf(stderr, "Error creating thread\n");
		}
		// Wait until the thread is initialized
		pthread_cond_wait(&init[i], &mutex);
	}
	pthread_mutex_unlock(&mutex);

	// Reset the global time and skip the initial wait
	gettimeofday(&t_global_start, NULL);

	// allow all threads to start their first release offset
	pthread_mutex_lock(&mutex);
	global_work = 1;
	for (int i = 0; i < 4; i++) {
		pthread_cond_signal(&resume[i]);
	}
	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < 240; i++) {
		// Select scheduler based on argv[1]
		switch(schedule) {
			case 0:
				fifo_schedule();
				break;
			case 1:
				edf_schedule();
				break;
			case 2:
				rm_schedule();
				break;
		}

		// Wait until the next 100ms interval or until a task is done
		int sleep = 100 - (get_time_stamp() % 100);
		if (num_of_alive_tasks > 0) {
			timed_wait_for_task_complition(sleep);
		} else {
			printf("All the tasks missed the deadline\n");
			break;
		}
	}

	// after 240 iterations, finish off all threads
	printf("[Main] It's time to finish the threads\n");

	printf("[Main] Locks\n");
	pthread_mutex_lock(&mutex);
	global_work = 0;

	// wake any worker that is waiting for the scheduler
	for (int i = 0; i < 4; i++) {
		pthread_cond_signal(&resume[i]);
	}
	ready_queue.clear();

	printf("[Main] Unlocks\n");
	pthread_mutex_unlock(&mutex);

	/* wait for the threads to finish */
	for (int i = 0; i < 4; i++) {
		if(pthread_join(tid[i], NULL)) {
			fprintf(stderr, "Error joining thread\n");
		}
	}

	return 0;
}

void fifo_schedule()
{
	// This function should schedule the tasks in a FIFO manner from the ready queue.
	// Hints:
	// - Protect any global variables you read or change using mutex.
	// - Do not schedule a new task while running_thread is not -1.
	// - FIFO means selecting ready_queue[0].
	// - "Schedule" a task using the conditional variable resume
	// 		- Check p3_threads.cpp:threadfunc to see how the conditional variable is used
	// - Remove the scheduled task from ready_queue after signaling it.
	// - Print one scheduler decision line when you select a thread:
	//   [<Time> ms][Scheduler] FIFO selected Thread <id> (deadline <deadline> ms, period <period> ms)
	// Your code goes here
	pthread_mutex_lock(&mutex);

	if (running_thread == -1 && !ready_queue.empty()) {
		int id = ready_queue[0];
		ready_queue.erase(ready_queue.begin());
		printf("[%6lu ms][Scheduler] FIFO selected Thread %d 
			  (deadline %lu ms, period %lu ms)\n", get_time_stamp(), 
			  id, tcb[id].deadline, tcb[id].period);
		pthread_cond_signal(&resume[id]);
	}
	pthread_mutex_unlock(&mutex);
}

void edf_schedule(void)
{
	// This function should schedule the ready task with the earliest absolute deadline.
	// Hints:
	// - Search ready_queue and compare tcb[id].deadline values.
	// - If no thread is running, signal the selected thread and remove it from ready_queue.
	// - If a thread is running and a ready thread has an earlier deadline, preempt the
	//   running thread, then schedule the ready thread with the earliest deadline.
	// - Preemption can be accomplished using this snippet of code while mutex is held:
	//		preempt = 1;
	//		pthread_cond_wait(&preempt_task, &mutex);
	// - Print one line when EDF preempts:
	//   [<Time> ms][Scheduler] EDF preempts Thread <running_id> for Thread <ready_id>
	// - Print one scheduler decision line when you select a thread:
	//   [<Time> ms][Scheduler] EDF selected Thread <id> (deadline <deadline> ms, period <period> ms)
	// Your code goes here
	pthread_mutex_lock(&mutex);
	if(ready_queue.empty()) {
		pthread_mutext_unlock(&mutex);
		return;
	}
	/*Find the ready thread with the earliest absolute deadline*/
	int best_idx = 0;
	for(int i = 1; i < (int)read_queue.size(); i++) {
		if(tcb[ready_queue[i]].deadline < tcb[ready_queue[best_idx]].deadline) {
			best_idx = i;
		}
	}
	int best_id = ready_queue[best_idx];

	if(running_thread == -1) {
		/*No thread running - schedule the best candidate immediately*/
		ready_queue.erase(ready_queue.begin() + best_idx);
		printf("[%6lu ms][Scheduler] EDF selected Thread %d (deadline %lu ms, period %lu ms)\n", get_time_stamp(), best_id, tcb[best_id].deadline, tcb[best_id].period);
		pthread_cond_signal(&resume[best_id]);
	} else if(tcb[best_id].deadline < tcb[running_thread].deadline) {
		/*A ready thread has an earlier deadline, therefore preempt the running thread*/
		printf("[%6lu ms][Scheduler] EDF preempts Thread %d for Thread %d\n", get_time_stamp(), running_thread, best_id);
		preempt = 1;
		pthread_cond_wait(&preempt_task, &mutex);
		/*Worker has requeued itself; running_thread is now -1*/
		/*Re-find the best candidate (preempted thread is back in the queue)*/
		best_idx = 0;
		for(int i = 1; i < (int)ready_queue.size(); i++) {
			if (tcb[ready_queue[i]].deadline < tcb[ready_queue[best_idx]].deadline) {
				best_idx = i;
			}
		}
		best_id = ready_queue[best_idx];
		ready_queue.erase(ready_queue.begin() + best_idx);
		printf("[%6lu ms][Scheduler] EDF selected Thread %d (deadline %lu ms, period %lu ms)\n", get_time_stamp(), best_id, tcb[best_id].deadline, tcb[best_id].period);
		pthread_cond_signal(&resume[best_id]);
	}
	pthread_mutex_unlock(&mutex);
}

void rm_schedule(void)
{
	// This function should schedule tasks using a rate-monotonic algorithm.
	// Hints:
	// - Rate-monotonic priority is fixed: shorter period means higher priority.
	// - RM does not choose by deadline, so a correct RM implementation may miss a deadline.
	// - Preempt the running thread only when a ready thread has a shorter period.
	// - Use the same scheduler decision and preemption output style as EDF, replacing
	//   "EDF" with "RM".
 	// Your code goes here
	pthread_mutex_lock(&mutex);
	if(ready_queue.empty()) {
		pthread_mutex_unlock(&mutex);
		return;
	}
	/*Find the ready thread with the shortest period (highest RM priority)*/
	int best_idx = 0;
	for(int i = 1; i < (int)ready_queue.size(); i++) {
		if (tcb[ready_queue[i]].period < tcb[ready_queue[best_idx]].period) {
			best_idx = i;
		}
	}
	int best_id = ready_queue[best_idx];

	if(running_thread == -1) {
		/*No thread running - schedule the highest-priority ready thread*/
		ready_queue.erase(ready_queue.begin() + best_idx);
		printf("[%6lu ms][Scheduler] RM selected Thread %d (deadline %lu ms. period %lu ms)\n", get_time_stamp(), best_id, tcb[best_id].deadline, tcb[best_id].period);

		pthread_cond_signal(&resume[best_id]);
	} else if (tcb[best_id].period < tcb[running_thread].period) {
		/*A ready thread has a shorter period, therefore preempt the current runnning thread*/
		printf("[%6lu ms][Scheduler] RM preempts Thread %d for Thread %d\n", get_time_stamp(), running_thread, best_id);
		preempt = 1;
		pthread_cond_wait(&preempt_task, &mutex);
		/*Worker has requeued itself; running_thread is now -1*/
		/*Re-find the best candidate (preempted thread is back in the queue)*/
		best_idx = 0;
		for(int i = 1; i < (int)ready_queue.size(); i++) {
			if (tcb[ready_queue[i]].period < tcb[ready_queue[best_idx]].period) {
				best_idx = i;
			}
		}
		best_id = ready_queue[best_idx];
		ready_queue.erase(ready_queue.begin() + best_idx);
		printf("[%6lu ms][Scheduler] RM selected Thread %d (deadline %lu ms, period %lu ms)\n", get_time_stamp(), best_id, tcb[best_id].deadline, tcb[best_id].period);
		pthread_cond_signal(&resume[best_id]);
	}
	pthread_mutex_unlock(&mutex);
}
