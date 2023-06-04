#pragma once
#include "TaskQueue.h"
class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(int min, int max);
	~ThreadPool();

	// �������
	void addTask(Task task);

	// ��ȡæ�̵߳ĸ���
	int getBusyNumber();

	// ��ȡ���ŵ��̸߳���
	int getAliveNumber();

private:

	// �������̵߳�������
	static void* worker(void* arg);

	// �������̵߳�������
	static void* manager(void* arg);

	// Ϊ������߳��˳���Ϣ����װ���߳��˳�����
	void threadExit();
private:
	//�������
	TaskQueue *taskQ;

	pthread_t managerID;	//	�������߳�ID
	pthread_t* threadIDs;	// �������߳�ID
	int minNum;				//��С�߳�����
	int maxNum;				//����߳�����
	int busyNum;			//æ���̵߳ĸ���
	int liveNum;			//�����̵߳ĸ���
	int exitNum;			//Ҫ���ٵ��̵߳ĸ���
	bool shutdown = false;	//�ǲ���Ҫ�����̳߳أ�����Ϊ1��������Ϊ0

	pthread_mutex_t mutexPool;	//���������̳߳�
	pthread_cond_t notEmpty;	//��������ǲ��ǿ���
};

