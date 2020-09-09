#include <atomic>
#include <optional>
#include <chrono>
#include <thread>
#include <iostream>
#include <jss/rcu.hpp>

template <typename T> class queue {
  struct node {
    std::optional<T> value;
    std::atomic<node *> next{nullptr};
  };
  std::atomic<node *> head, tail;

public:
  queue() : head(new node), tail(head.load()) {}
  void push(T value);
  T pop();
};

template <typename T> void queue<T>::push(T value) {
  jss::rcu_reader r;
  node *nn = new node{std::move(value)};
  while(true) {
    auto last = tail.load();
    auto next = last->next.load();
    if(last == tail.load()) {
      if(!next &&
        last->next.compare_exchange_strong(next, nn)) {
        tail.compare_exchange_strong(last, nn);
        return;
      } else
        tail.compare_exchange_strong(last, next);
    }
  }
}

template <typename T> T queue<T>::pop() {
  jss::rcu_reader r;

  while(true) {
    auto first = head.load();
    auto last = tail.load();
    auto next = first->next.load();
    if(first == head.load()) {
      if(first == last) {
        if(!next)
          throw std::runtime_error("empty");
        tail.compare_exchange_strong(last, next);
      } else if(head.compare_exchange_strong(first, next)) {
        T res = std::move(*next->value);
        jss::rcu_retire(first);
        return res;
      }
    }
  }
}

int main() {
  queue<int> q;

  unsigned const count = 10000000;

  typedef std::chrono::high_resolution_clock clock;

  clock::time_point start, end;

  start = clock::now();

  std::thread t1([&] {
    for(unsigned i = 0; i < count; ++i) {
      q.push(i + 30 * count);
    }
  });

  std::thread t2([&] {
    for(unsigned i = 0; i < count; ++i) {
      q.push(i + 10 * count);
    }
  });

  std::thread t3([&] {
    for(unsigned i = 0; i < count; ++i) {
      q.push(i + 20 * count);
    }
  });

  std::thread t4([&] {
    for(unsigned i = 0; i < (count * 3); ++i) {
      q.pop();

      // std::cout<<q.pop()<<std::endl;
    }
  });

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  jss::rcu_barrier();

  end = clock::now();
  std::cout
    << "total time: "
    << std::chrono::duration_cast<std::chrono::milliseconds>(
         end - start)
         .count()
    << "ms" << std::endl;
}
