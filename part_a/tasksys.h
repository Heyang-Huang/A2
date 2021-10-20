#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdio.h>


/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        int num_threads_;
        void run(IRunnable* runnable, int num_total_tasks);
        void runThread(int thread_id, IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
typedef struct Task
{
   IRunnable* runnable;
   int task_id;
   int num_total_tasks;
} Task;

class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        int num_threads_;
        std::thread* thread_pool_;
        int* is_thread_idle;
        bool should_terminate_;
        IRunnable* runnable_;
        int task_id_;
        int num_total_tasks_;
        std::mutex* queue_mutex_;
        void run(IRunnable* runnable, int num_total_tasks);
        void runThread(int thread_id);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
 class ThreadState {
     public:
         std::condition_variable* condition_variable_;
         std::mutex* mutex_;
         int counter_;
         int num_waiting_threads_;
         ThreadState(int num_waiting_threads) {
             condition_variable_ = new std::condition_variable();
             mutex_ = new std::mutex();
             counter_ = 0;
             num_waiting_threads_ = num_waiting_threads;
         }
         ~ThreadState() {
             delete condition_variable_;
             delete mutex_;
         }
 };

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        int num_threads_;
        std::thread* thread_pool_;
        int* is_thread_idle;
        bool should_terminate_;
        std::atomic<bool> no_more_work_;
        IRunnable* runnable_;
        int task_id_;
        int num_total_tasks_;
        ThreadState* thread_state_;
        void run(IRunnable* runnable, int num_total_tasks);
        void runThread(int thread_id);
        void notifyAllThreads();
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
