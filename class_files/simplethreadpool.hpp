#pragma once
#include "jthread.hpp"
#include <vector>
#include <functional>
#include <mutex>
#include "condition_variable_any2.hpp"
#include <queue>

class thread_pool {
public:
  using task_type = std::function<void()>;

  explicit thread_pool(unsigned num_threads);
  ~thread_pool();

  thread_pool(thread_pool const &) = delete;
  thread_pool &operator=(thread_pool const &) = delete;

  void submit(task_type func);

private:
  void thread_func(std::stop_token stop);

  std::mutex mut;
  std::condition_variable_any2 cond;
  std::queue<task_type> queue;
  std::vector<std::jthread> threads;
};
