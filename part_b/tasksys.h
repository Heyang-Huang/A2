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
 * 4 : [1???3]
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
        std::map<TaskID, std::pair<IRunnable*, int>> id_to_task;
        std::map<TaskID, std::set<TaskID>> deps_map;
        std::map<TaskID, std::set<TaskID>> deps_map_inverse;
        std::map<TaskID, std::array<int, 3>> processing_progress;
        std::mutex dep_lock;
        int task_count;
        int finished_task_count;
        int numRdyWorker;
        std::condition_variable thread_waiting;
        bool all_tasks_finished;

        void worker(int workerId);
        bool isAllIdle();
        int num_total_tasks;
        int this_task;
        TaskID this_tid;
        bool isDeconstruct;
        IRunnable* runnable;
        bool startup;
        std::vector<std::condition_variable_any> wakeThread;
        std::condition_variable_any checkWorkLeft;
        std::condition_variable_any readyToStart;
        std::vector<std::thread> thread_vec;
        std::vector<bool> idle;
        std::mutex task_lock;

        void rdy2Run(IRunnable* run, int total_tasks);
        bool all_done;
};

#endif
