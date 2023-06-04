#include "ThreadPool.h"
#include <iostream>
#include <string.h>
#include <unistd.h>

const int NUMBER = 2;
ThreadPool::ThreadPool()
{
}

ThreadPool::ThreadPool(int min, int max)
{
	// 实例化任务队列
	taskQ = new TaskQueue;
	do {
		//初始化线程池属性
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;
		exitNum = 0;

		//初始化互斥锁和条件变量，如果初始化失败，返回值则不为0，输出报错日志
		if (pthread_mutex_init(&mutexPool, NULL) != 0
			|| pthread_cond_init(&notEmpty, NULL) != 0)
		{
			cout << "mutex or condition init fail..." << endl;
			break;
		}

		// 根据线程的最大上限给线程数组分配内存
		threadIDs = new pthread_t[maxNum];
		if (threadIDs == nullptr)
		{
			cout << "malloc thread_t[] 失败...." << endl;;
			break;
		}
		// 初始化
		memset(threadIDs, 0, sizeof(pthread_t) * maxNum);


		/////////////////// 创建线程 //////////////////
		// 根据最小线程个数, 创建线程
		for (int i = 0; i < minNum; ++i)
		{
			pthread_create(&threadIDs[i], NULL, worker, this);
			cout << "创建子线程, ID: " << to_string(threadIDs[i]) << endl;
		}
		// 创建管理者线程, 1个
		pthread_create(&managerID, NULL, manager, this);
	} while (0);
}

ThreadPool::~ThreadPool()
{
	//将线程池中的shutdown设为1
	shutdown = true;
	// 关闭管理者线程
		pthread_join(managerID, NULL);
	//唤醒阻塞的消费者线程
	for (int i = 0; i < liveNum; i++) {
		pthread_cond_signal(&notEmpty);
	}
	//释放堆内存
	if (taskQ) {
		delete taskQ;
	}
	if (threadIDs) {
		delete []threadIDs;
	}

	//销毁互斥锁和条件变量
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);
}

void ThreadPool::addTask(Task task)
{
	if (shutdown)
	{
		return;
	}
	// 添加任务，不需要加锁，任务队列中有锁
	taskQ->addTask(task);
	// 唤醒工作的线程
	pthread_cond_signal(&notEmpty);

}

int ThreadPool::getBusyNumber()
{
	int busyNumber = 0;
	pthread_mutex_lock(&mutexPool);
	busyNumber = busyNum;
	pthread_mutex_unlock(&mutexPool);
	return busyNumber;
}

int ThreadPool::getAliveNumber()
{
	int aliveNumber = 0;
	pthread_mutex_lock(&mutexPool);
	aliveNumber = liveNum;
	pthread_mutex_unlock(&mutexPool);
	return aliveNumber;
}

void* ThreadPool::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (1) {
		pthread_mutex_lock(&pool->mutexPool);
		while (pool->taskQ->getTaskNumber() == 0 && !pool->shutdown) {
			//当前任务队列为空，但是线程池没有关闭
			//利用条件变量notEmpty和互斥锁mutexPool来阻塞线程
			//cout << "thread " << to_string(pthread_self()) << " waiting..." << endl;
			printf("thread %ld waiting...\n", pthread_self());
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			//阻塞的线程解锁之后，需要判断是否要销毁自身
			if (pool->exitNum > 0) {
				pool->exitNum--;
				if (pool->liveNum > pool->minNum) {
					//如果存活的线程大于最小线程数，进行线程销毁
					pool->liveNum--;
					//解锁
					pthread_mutex_unlock(&pool->mutexPool);
					pool->threadExit();
				}
			}
		}

		//判断线程池是否被关闭了
		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			pool->threadExit();
		}

		//消费者线程开始工作，从任务队列中执行任务
		Task task = pool->taskQ->takeTask();
		// 工作的线程+1
		pool->busyNum++;
		
		//线程分配完任务，解锁线程
		pthread_mutex_unlock(&pool->mutexPool);

		//线程开始执行任务
		//cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
		printf("thread %ld start working...\n", pthread_self());
		//线程执行任务,并执行完后回收任务的内存
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;
		//任务执行完毕，修改busyNum
		//cout << "thread " << to_string(pthread_self()) << " end working..."<<endl;
		printf("thread %ld end working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexPool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexPool);
	}
	return nullptr;
}

//管理者线程的任务函数
//1.判断线程池是否关闭，未关闭，则每3秒检测一下当前存在线程，工作线程，与任务队列的关系
//2.根据存在线程，工作线程，与任务队列，进行调度（创建线程或者销毁线程）
void* ThreadPool::manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown) {
		//每个3秒检测一次
		sleep(5);

		//取出线程池中的任务数量，存在线程，在忙线程的数量
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->getTaskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexPool);


		//根据数量进行线程调度
		//添加线程：任务个数>存活线程个数&& 存活的线程数<最大的线程数
		if (queueSize > liveNum && liveNum < pool->maxNum) {
			pthread_mutex_lock(&pool->mutexPool);
			int counter = 0;
			for (int i = 0; i < pool->maxNum && counter < NUMBER
				&& pool->liveNum < pool->maxNum; i++) {
				if (pool->threadIDs[i] == 0) {
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					counter++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		//销毁线程:忙的线程*2<存活的线程&&存活线程>最小线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum) {
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			//通过解锁条件变量notEmpty来让工作线程自动销毁
			for (int i = 0; i < NUMBER; i++) {
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
	return nullptr;
}

void ThreadPool::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; i++) {
		if (threadIDs[i] == tid) {
			threadIDs[i] = 0;
			cout << "threadExit() function: thread "<< to_string(pthread_self()) << " exiting..." << endl;
			break;
		}
	}
	pthread_exit(NULL);
}
