#include "tasksys.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <utility>
#include <iostream>

#include <stdio.h>

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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    num_threads_ = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::runThread(int thread_id, IRunnable* runnable, int num_total_tasks) {
  // static allocation
  for (int i = 0; i < num_total_tasks; i++) {
    if (i % num_threads_ == thread_id) {
      runnable->runTask(i, num_total_tasks);
    }
  }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::thread* threads = new std::thread[num_threads_];
    for (int t = 0; t < num_threads_; t++) {
      threads[t] = std::thread(&TaskSystemParallelSpawn::runThread, this, t, runnable, num_total_tasks);
    }
    // this->runThread(num_threads_ - 1, runnable, num_total_tasks);

    for (int t = 0; t < num_threads_; t++) {
      threads[t].join();
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelSpawn::sync() {
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    // global task tracker
    runnable_ = nullptr;
    task_id_ = 0;
    num_total_tasks_ = 0;
    // mutex
    queue_mutex_ = new std::mutex();
    // idle thread tracker
    is_thread_idle = new int[num_threads];
    for (int i = 0; i < num_threads; i++) {
      is_thread_idle[i] = 1;
    }
    // threads
    should_terminate_ = false;
    num_threads_ = num_threads;
    thread_pool_ = new std::thread[num_threads_];
    for (int t = 0; t < num_threads_ ; t++) {
      thread_pool_[t] = std::thread(&TaskSystemParallelThreadPoolSpinning::runThread, this, t);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
  should_terminate_ = true;
  for (int i = 0; i < num_threads_; i++) {
    thread_pool_[i].join();
  }
  delete queue_mutex_;
}

void TaskSystemParallelThreadPoolSpinning::runThread(int thread_id) {
  int thread_task_id;

  // dynamic allocation
  while (!should_terminate_) {
    queue_mutex_->lock();
    bool queue_empty = task_id_ == num_total_tasks_;
    if (!queue_empty) {
      is_thread_idle[thread_id] = 0;
      thread_task_id = task_id_++;
      queue_mutex_->unlock();

      runnable_->runTask(thread_task_id, num_total_tasks_);
    } else {
      is_thread_idle[thread_id] = 1;
      queue_mutex_->unlock();
    }
  }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // add work to work queue
    queue_mutex_->lock();
    runnable_ = runnable;
    task_id_ = 0;
    num_total_tasks_ = num_total_tasks;
    queue_mutex_->unlock();

    // determine if all tasks are completed
    bool finish = false;
    while (!finish) {
      queue_mutex_->lock();
      bool queue_empty = task_id_ == num_total_tasks_;
      queue_mutex_->unlock();
      if (queue_empty) {
        finish = true;
        for (int t = 0; t < num_threads_; t++) {
          bool is_idle = is_thread_idle[t] == 1;

          if (!is_idle) {
            finish = false;
            break;
          }
        }
      }
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    // global task tracker
    runnable_ = nullptr;
    task_id_ = 0;
    num_total_tasks_ = 0;
    // mutex + condition variable
    thread_state_ = new ThreadState(num_threads);
    // idle thread tracker
    is_thread_idle = new int[num_threads];
    for (int i = 0; i < num_threads; i++) {
      is_thread_idle[i] = 1;
    }
    // threads
    should_terminate_ = false;
    no_more_work_ = false;
    num_threads_ = num_threads;
    thread_pool_ = new std::thread[num_threads_];
    for (int t = 0; t < num_threads_ ; t++) {
      thread_pool_[t] = std::thread(&TaskSystemParallelThreadPoolSleeping::runThread, this, t);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    thread_state_->mutex_->lock();
    should_terminate_ = true;
    thread_state_->counter_ = 0;
    while (thread_state_->counter_ < thread_state_->num_waiting_threads_) {
        // printf("notifying: thread counter %d\r", thread_state_->counter_);
        thread_state_->mutex_->unlock();
        thread_state_->condition_variable_->notify_all();
        thread_state_->mutex_->lock();
    }
    thread_state_->mutex_->unlock();

    for (int i = 0; i < num_threads_; i++) {
      thread_pool_[i].join();
    }
}

void TaskSystemParallelThreadPoolSleeping::runThread(int thread_id) {
  int thread_task_id;
  bool queue_empty;

  // dynamic allocation
  bool terminate = false;
  while (!terminate) {
    std::unique_lock<std::mutex> lock(*thread_state_->mutex_);
    thread_state_->condition_variable_->wait(lock);
    thread_state_->counter_++;
    // printf("thread %d: thread counter %d\n", thread_id, thread_state_->counter_);

    queue_empty = task_id_ == num_total_tasks_;
    while (!queue_empty) {
      is_thread_idle[thread_id] = 0;
      thread_task_id = task_id_++;
      lock.unlock();
      runnable_->runTask(thread_task_id, num_total_tasks_);

      lock.lock();
      queue_empty = task_id_ == num_total_tasks_;
    }
    is_thread_idle[thread_id] = 1;
    no_more_work_ = true;
    terminate = should_terminate_;
    lock.unlock();
  }
}

void TaskSystemParallelThreadPoolSleeping::notifyAllThreads() {
  thread_state_->mutex_->lock();
  thread_state_->counter_ = 0;
  while ((thread_state_->counter_ < thread_state_->num_waiting_threads_) && (task_id_ < num_total_tasks_)) {
      thread_state_->mutex_->unlock();
      thread_state_->condition_variable_->notify_all();
      thread_state_->mutex_->lock();
  }
  thread_state_->mutex_->unlock();
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // add work to work queue
    thread_state_->mutex_->lock();
    runnable_ = runnable;
    task_id_ = 0;
    num_total_tasks_ = num_total_tasks;
    no_more_work_ = false;
    thread_state_->mutex_->unlock();

    // signal all threads
    notifyAllThreads();

    // determine if all tasks are completed
    bool finish = false;
    while (!finish) {
      if (no_more_work_) {
        finish = true;
        for (int t = 0; t < num_threads_; t++) {
          if (is_thread_idle[t] == 0) {
            finish = false;
            break;
          }
        }
      }
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
