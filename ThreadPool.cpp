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
	// ʵ�����������
	taskQ = new TaskQueue;
	do {
		//��ʼ���̳߳�����
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;
		exitNum = 0;

		//��ʼ�������������������������ʼ��ʧ�ܣ�����ֵ��Ϊ0�����������־
		if (pthread_mutex_init(&mutexPool, NULL) != 0
			|| pthread_cond_init(&notEmpty, NULL) != 0)
		{
			cout << "mutex or condition init fail..." << endl;
			break;
		}

		// �����̵߳�������޸��߳���������ڴ�
		threadIDs = new pthread_t[maxNum];
		if (threadIDs == nullptr)
		{
			cout << "malloc thread_t[] ʧ��...." << endl;;
			break;
		}
		// ��ʼ��
		memset(threadIDs, 0, sizeof(pthread_t) * maxNum);


		/////////////////// �����߳� //////////////////
		// ������С�̸߳���, �����߳�
		for (int i = 0; i < minNum; ++i)
		{
			pthread_create(&threadIDs[i], NULL, worker, this);
			cout << "�������߳�, ID: " << to_string(threadIDs[i]) << endl;
		}
		// �����������߳�, 1��
		pthread_create(&managerID, NULL, manager, this);
	} while (0);
}

ThreadPool::~ThreadPool()
{
	//���̳߳��е�shutdown��Ϊ1
	shutdown = true;
	// �رչ������߳�
		pthread_join(managerID, NULL);
	//�����������������߳�
	for (int i = 0; i < liveNum; i++) {
		pthread_cond_signal(&notEmpty);
	}
	//�ͷŶ��ڴ�
	if (taskQ) {
		delete taskQ;
	}
	if (threadIDs) {
		delete []threadIDs;
	}

	//���ٻ���������������
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);
}

void ThreadPool::addTask(Task task)
{
	if (shutdown)
	{
		return;
	}
	// ������񣬲���Ҫ�������������������
	taskQ->addTask(task);
	// ���ѹ������߳�
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
			//��ǰ�������Ϊ�գ������̳߳�û�йر�
			//������������notEmpty�ͻ�����mutexPool�������߳�
			//cout << "thread " << to_string(pthread_self()) << " waiting..." << endl;
			printf("thread %ld waiting...\n", pthread_self());
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			//�������߳̽���֮����Ҫ�ж��Ƿ�Ҫ��������
			if (pool->exitNum > 0) {
				pool->exitNum--;
				if (pool->liveNum > pool->minNum) {
					//��������̴߳�����С�߳����������߳�����
					pool->liveNum--;
					//����
					pthread_mutex_unlock(&pool->mutexPool);
					pool->threadExit();
				}
			}
		}

		//�ж��̳߳��Ƿ񱻹ر���
		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			pool->threadExit();
		}

		//�������߳̿�ʼ�����������������ִ������
		Task task = pool->taskQ->takeTask();
		// �������߳�+1
		pool->busyNum++;
		
		//�̷߳��������񣬽����߳�
		pthread_mutex_unlock(&pool->mutexPool);

		//�߳̿�ʼִ������
		//cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
		printf("thread %ld start working...\n", pthread_self());
		//�߳�ִ������,��ִ��������������ڴ�
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;
		//����ִ����ϣ��޸�busyNum
		//cout << "thread " << to_string(pthread_self()) << " end working..."<<endl;
		printf("thread %ld end working...\n", pthread_self());
		pthread_mutex_lock(&pool->mutexPool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexPool);
	}
	return nullptr;
}

//�������̵߳�������
//1.�ж��̳߳��Ƿ�رգ�δ�رգ���ÿ3����һ�µ�ǰ�����̣߳������̣߳���������еĹ�ϵ
//2.���ݴ����̣߳������̣߳���������У����е��ȣ������̻߳��������̣߳�
void* ThreadPool::manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown) {
		//ÿ��3����һ��
		sleep(5);

		//ȡ���̳߳��е����������������̣߳���æ�̵߳�����
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->getTaskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexPool);


		//�������������̵߳���
		//����̣߳��������>����̸߳���&& �����߳���<�����߳���
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

		//�����߳�:æ���߳�*2<�����߳�&&����߳�>��С�߳���
		if (busyNum * 2 < liveNum && liveNum > pool->minNum) {
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			//ͨ��������������notEmpty���ù����߳��Զ�����
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
