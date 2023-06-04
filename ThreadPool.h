#pragma once
#include "TaskQueue.h"
class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(int min, int max);
	~ThreadPool();

	// 添加任务
	void addTask(Task task);

	// 获取忙线程的个数
	int getBusyNumber();

	// 获取活着的线程个数
	int getAliveNumber();

private:

	// 工作的线程的任务函数
	static void* worker(void* arg);

	// 管理者线程的任务函数
	static void* manager(void* arg);

	// 为了输出线程退出信息，封装的线程退出函数
	void threadExit();
private:
	//任务队列
	TaskQueue *taskQ;

	pthread_t managerID;	//	管理者线程ID
	pthread_t* threadIDs;	// 工作的线程ID
	int minNum;				//最小线程数量
	int maxNum;				//最大线程数量
	int busyNum;			//忙的线程的个数
	int liveNum;			//存活的线程的个数
	int exitNum;			//要销毁的线程的个数
	bool shutdown = false;	//是不是要销毁线程池，销毁为1，不销毁为0

	pthread_mutex_t mutexPool;	//锁整个的线程池
	pthread_cond_t notEmpty;	//任务队列是不是空了
};

