#include "TaskQueue.h"

TaskQueue::TaskQueue()
{
	pthread_mutex_init(&mutexQueue, NULL);
}

TaskQueue::~TaskQueue()
{
	pthread_mutex_destroy(&mutexQueue);
}

void TaskQueue::addTask(Task task)
{
	pthread_mutex_lock(&mutexQueue);
	taskQ.push(task);
	pthread_mutex_unlock(&mutexQueue);
}

void TaskQueue::addTask(callback f, void* arg)
{
	pthread_mutex_lock(&mutexQueue);
	taskQ.push(Task(f,arg));
	pthread_mutex_unlock(&mutexQueue);
}

Task TaskQueue::takeTask()
{
	Task task;
	pthread_mutex_lock(&mutexQueue);
	if (taskQ.size() > 0) {
		task = taskQ.front();
		taskQ.pop();
	}
	pthread_mutex_unlock(&mutexQueue);
	return task;
}

int TaskQueue::getTaskNumber()
{
	int number;
	pthread_mutex_lock(&mutexQueue);
	number = taskQ.size();
	pthread_mutex_unlock(&mutexQueue);
	return number;
}
