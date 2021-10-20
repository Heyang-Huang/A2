#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <map>
#include <queue>
#include <array>
#include <mutex>
#include <thread>
#include <set>
#include <condition_variable>

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
        void run(IRunnable* runnable, int num_total_tasks);
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
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 * 
 * 
 * deps_map looks like:
 * 0 : []
 * 1 : [0]
 * 2 : [0, 1]
 * 3 : [2]
 * 4 : [1，3]
 * 
 * coresponding deps_map_inverse looks like:
 * 0 : [1, 2]
 * 1 : [2, 4]
 * 2 : [3]
 * 3 : [4]
 * 
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    private:
        // std::pair is a pair of runnable and num_total_tasks
        std::map<TaskID, std::pair<IRunnable*, int>> id_to_task;
        std::map<TaskID, std::set<TaskID>> deps_map;
        std::map<TaskID, std::set<TaskID>> deps_map_inverse;
        // array<int, 3> of processing_progress have three elements:
        // 1. total task number
        // 2. current task number picked up
        // 3. current task number finished
        std::map<TaskID, std::array<int, 3>> processing_progress;
        std::mutex dep_lock;
        int task_count;
        int finished_task_count;
        int workers_ready;
        void task_finished(TaskID tid);
        std::condition_variable thread_waiting;
        bool all_tasks_finished;

        void worker(int workerId);
        bool allWorkersIdle();
        int num_total_tasks;
        int cur_task;
        TaskID cur_tid;
        bool deconstruct;
        IRunnable* runnable;
        bool startup;
        std::vector<std::condition_variable_any> wakeThread;
        std::condition_variable_any checkWorkLeft;
        std::condition_variable_any readyToStart;
        std::vector<std::thread> thread_vec;
        std::vector<bool> idle;
        std::mutex task_lock;

        void addRunnable(IRunnable* run, int total_tasks);
        bool all_done;
};

#endif
