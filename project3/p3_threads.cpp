#include "p3_threads.h"
#include "utils.h"

extern pthread_mutex_t mutex;
extern pthread_mutex_t taskDoneMutex;

extern pthread_cond_t resume[4];
extern pthread_cond_t init[4];
extern pthread_cond_t a_task_is_done;
extern pthread_cond_t preempt_task;
extern std::vector<int> ready_queue;
extern int running_thread;
extern int preempt;
extern int num_of_alive_tasks;

extern int global_work; // DO NOT INITIALIZE!!

void signal_task_done(void)
{
	pthread_mutex_lock(&taskDoneMutex);
	pthread_cond_signal(&a_task_is_done);
	pthread_mutex_unlock(&taskDoneMutex);
}

int should_preempt(void)
{
	int value;

	pthread_mutex_lock(&mutex);
	value = preempt;
	pthread_mutex_unlock(&mutex);

	return value;
}

void wait_for_global_start(int id)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&init[id]);

	while (global_work == 0) {
		pthread_cond_wait(&resume[id], &mutex);
	}

	pthread_mutex_unlock(&mutex);
}

int become_ready_and_wait(ThreadCtrlBlk* tcb)
{
	int id = tcb->id;

	pthread_mutex_lock(&mutex);

	if (global_work == 0) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}

	ready_queue.push_back(id);
	printf("[%6lu ms][Thread %d] Ready to be scheduled\n", get_time_stamp(), id);

	while (global_work != 0) {
		pthread_cond_wait(&resume[id], &mutex);

		if (running_thread == -1) {
			running_thread = id;
			pthread_mutex_unlock(&mutex);
			return 1;
		}
	}

	pthread_mutex_unlock(&mutex);
	return 0;
}

int wait_after_preemption(ThreadCtrlBlk* tcb, int iter, long worked_ms)
{
	int id = tcb->id;

	printf("[%6lu ms][Thread %d] Thread preempted\n", get_time_stamp(), id);

	pthread_mutex_lock(&mutex);
	preempt = 0;
	running_thread = -1;
	ready_queue.push_back(id);
	printf("[%6lu ms][Thread %d] Ready to be scheduled\n", get_time_stamp(), id);
	pthread_cond_signal(&preempt_task);

	while (global_work != 0) {
		pthread_cond_wait(&resume[id], &mutex);

		if (running_thread == -1) {
			running_thread = id;
			pthread_mutex_unlock(&mutex);
			printf("[%6lu ms][Thread %d] Restarting task (iteration %d) with   %4lu ms remaining\n",
				get_time_stamp(), id, iter, tcb->task_time - worked_ms);
			return 1;
		}
	}

	pthread_mutex_unlock(&mutex);
	return 0;
}

int run_task(ThreadCtrlBlk* tcb, int iter)
{
	long worked_ms;

	printf("[%6lu ms][Thread %d] Starting task   (iteration %d) taking %4lu ms\n",
		get_time_stamp(), tcb->id, iter, tcb->task_time);

	for (worked_ms = 0; worked_ms < tcb->task_time; worked_ms += 100) {
		if (should_preempt()) {
			if (!wait_after_preemption(tcb, iter, worked_ms)) {
				return 0;
			}
		}
		usleep(MSEC(100));
	}

	return 1;
}

void release_cpu(void)
{
	pthread_mutex_lock(&mutex);
	running_thread = -1;
	if (preempt) {
		preempt = 0;
		pthread_cond_signal(&preempt_task);
	}
	pthread_mutex_unlock(&mutex);

	signal_task_done();
}

void *threadfunc(void *param)
{
	ThreadCtrlBlk* tcb = (ThreadCtrlBlk*) param;
	int iter = 1;
	int id = tcb->id;
	int fail = 0;

	printf(" - [Thread %d] Started\n", id);
	wait_for_global_start(id);

	if (tcb->start_offset > 0) {
		usleep(MSEC(tcb->start_offset));
	}

	// work loop
	while (true) {
		if (!become_ready_and_wait(tcb)) {
			break;
		}

		if (!run_task(tcb, iter)) {
			break;
		}

		long end_time = get_time_stamp();

		// task is completed
		printf("[%6lu ms][Thread %d] Completed task  (iteration %d) taking %4lu ms\n", end_time, id, iter, tcb->task_time);

		// check if missed deadline (50ms slack)
		if (end_time > (tcb->deadline + 50)) {
			printf("[%6lu ms][Thread %d] Task (iteration %d) missed its deadline!! (Deadline: %lu ms)\n", end_time, id, iter, tcb->deadline);
			fail = 1;
			release_cpu();
			break;
		}

		// find amount of time to wait to sync with the next iteration's period
		int sleep = tcb->start_offset + (iter * tcb->period) - get_time_stamp();
		iter++;

		// set deadline for next iteration and change running_thread to availble
		pthread_mutex_lock(&mutex);
		tcb->deadline = tcb->start_offset + ((iter - 1) * tcb->period) + tcb->relative_deadline;
		running_thread = -1;
		if (preempt) {
			preempt = 0;
			pthread_cond_signal(&preempt_task);
		}
		pthread_mutex_unlock(&mutex);

		// signal that the task is done for this iteration
		signal_task_done();

		// sleep until synced with next period
		if(sleep > 0) {
			usleep(MSEC(sleep));
		}
	}

	printf(" - [Thread %d] Complete\n", id);
	if (fail) {
		num_of_alive_tasks--;
		printf(" - [Thread %d] Missed deadline\n", id);
	}

	return NULL;
}
