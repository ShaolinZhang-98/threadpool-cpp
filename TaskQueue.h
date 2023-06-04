#pragma once
#include <queue>
#include <pthread.h>
using namespace std;

// ��������ṹ��
using callback = void(*)(void*);
struct Task
{
    Task()
    {
        function = nullptr;
        arg = nullptr;
    }
    Task(callback f, void* arg)
    {
        function = f;
        this->arg = arg;
    }
    callback function;
    void* arg;
};


class TaskQueue
{
public:
    TaskQueue();
    ~TaskQueue();

    //�������
    void addTask(Task task);
    void addTask(callback f, void* arg);
    //ȡ��һ������
    Task takeTask();

    //��ȡ��ǰ����ĸ���
    int getTaskNumber();
private:
    pthread_mutex_t mutexQueue;    //
    queue<Task> taskQ; // �������

};

