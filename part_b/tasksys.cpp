#include "tasksys.h"

#include <iostream>
using namespace std;
IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads):
                                                                            ITaskSystem(num_threads),
                                                                            thread_vec(num_threads),
                                                                            cur_task(0),
                                                                            num_total_tasks(0),
                                                                            idle(num_threads, true),
                                                                            isDeconstruct(false),
                                                                            wakeThread(num_threads),
                                                                            task_lock(),
                                                                            task_count(0),
                                                                            workers_ready(0),
                                                                            all_tasks_finished(true),
                                                                            all_done(true),
                                                                            startup(false) {
    for (int i = 0; i < thread_vec.size(); i++) {
        thread_vec[i] = thread([this, i]() {
            worker(i);
        });
    }
}

bool TaskSystemParallelThreadPoolSleeping::isAllIdle() {
    bool isIdle = true;
    for (bool flag : idle) {
        if (!flag) {
            isIdle = false;
            break;
        }
    }
    return isIdle;
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {

    task_lock.lock();
    while (!isAllIdle()) {
        checkWorkLeft.wait(task_lock);
    }
    task_lock.unlock();

    isDeconstruct = true;
    for (auto& wake_head : wakeThread) {
        wake_head.notify_all();
    }
    for (thread& thread_head : thread_vec) {
        thread_head.join();
    }
}

void TaskSystemParallelThreadPoolSleeping::worker(int workerId){

    task_lock.lock();
    workers_ready++;
    readyToStart.notify_all();
    wakeThread[workerId].wait(task_lock);
    task_lock.unlock();
    while (!isDeconstruct) {
        task_lock.lock();
        if (cur_task == num_total_tasks) {
            idle[workerId] = true;
            if (!all_done && isAllIdle()) {
                dep_lock.lock();
                deps_map.erase(cur_tid);
                for (const TaskID id_to_delete: deps_map_inverse[cur_tid]) {
                    deps_map[id_to_delete].erase(cur_tid);

                    if (deps_map[id_to_delete].size() == 0) {
                        processing_progress[id_to_delete] = {id_to_task[id_to_delete].second, 0, 0};
                    }
                }
                deps_map_inverse.erase(cur_tid);

                finished_task_count++;
    
                processing_progress.erase(cur_tid);

                id_to_task.erase(cur_tid);
                
                if (processing_progress.size() != 0) {
                    for (auto curr_task: processing_progress) {
                        TaskID id = curr_task.first;
                        array<int, 3> task = curr_task.second;
                        if (task[1] == 0) {
                            cur_tid = id;
                            addRunnable(id_to_task[id].first, id_to_task[id].second);
                            break;
                        }
                    }
                } else {
                        all_done = true;
                  }
    
    
                dep_lock.unlock();
                if (deps_map.size() == 0) {
                    checkWorkLeft.notify_all();
                }

            } else {
                wakeThread[workerId].wait(task_lock);
            }

            task_lock.unlock();
            if (isDeconstruct) {
                break;
            }
            continue;
        }

        int my_task = cur_task;
        int my_total_tasks = num_total_tasks;
        cur_task++;
        task_lock.unlock();

        runnable->runTask(my_task, my_total_tasks);

    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* run, int total_tasks) {
    const vector<TaskID> no_deps{};
    runAsyncWithDeps(run, total_tasks,no_deps);
    sync();
}


void TaskSystemParallelThreadPoolSleeping::addRunnable(IRunnable* run, int total_tasks) {
    cur_task = 0;
    num_total_tasks = total_tasks;
    runnable = run;
    for (int i =0; i < idle.size(); i++) {
        idle[i] = false;
    }
    task_lock.lock();
    while (workers_ready != thread_vec.size()) {
        readyToStart.wait(task_lock);
    }
    task_lock.unlock();
    for (auto& wake_head : wakeThread) {
        wake_head.notify_all();
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {



    dep_lock.lock();
    task_count++;
    TaskID curr_task_id = task_count;

    id_to_task[curr_task_id] = {runnable, num_total_tasks};

    std::set<TaskID> valid_deps;
    
    for (const auto& dep_task_id: deps) {

        if (deps_map.count(dep_task_id)) {
            valid_deps.insert(dep_task_id);

            if (deps_map_inverse.count(dep_task_id)) {
                deps_map_inverse[dep_task_id].insert(curr_task_id);
            } else {
                std::set<TaskID> deps_inverse = {curr_task_id};
                deps_map_inverse[dep_task_id] = deps_inverse;
            }
        }
    }
    deps_map[curr_task_id] = valid_deps;

    if (valid_deps.size() == 0) {
        processing_progress[curr_task_id] = {num_total_tasks, 0, 0};
    }
    all_done = false;
    if (isAllIdle()) {
        cur_tid = curr_task_id;
        addRunnable(runnable, num_total_tasks);
    }
    dep_lock.unlock();
    
    return curr_task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    dep_lock.lock();
    while (deps_map.size() != 0) {
        checkWorkLeft.wait(dep_lock);
    }
    dep_lock.unlock();

    return;
}
