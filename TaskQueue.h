#pragma once
#include <queue>
#include <pthread.h>
using namespace std;

// 定义任务结构体
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

    //添加任务
    void addTask(Task task);
    void addTask(callback f, void* arg);
    //取出一个任务
    Task takeTask();

    //获取当前任务的个数
    int getTaskNumber();
private:
    pthread_mutex_t mutexQueue;    //
    queue<Task> taskQ; // 任务队列

};

