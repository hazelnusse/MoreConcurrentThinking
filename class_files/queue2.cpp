#include <atomic>
#include <assert.h>
#include <iostream>
#include <type_traits>
#include <thread>

template <typename T, unsigned buffer_size = 42>
class queue2 {
public:
  void push_back(T t) {
    unsigned my_pos = push_pos;
    auto &my_entry = buffer[my_pos];
    if(my_entry.initialized.load())
      throw std::runtime_error("Full");
    push_pos = (my_pos + 1) % buffer_size;
    new(&my_entry.storage) T(std::move(t));
    my_entry.initialized.store(true);
  }
  T pop_front() {
    if(!buffer[pop_pos].initialized.load())
      throw std::runtime_error("empty");
    auto ptr = static_cast<T *>(
      static_cast<void *>(&buffer[pop_pos].storage));
    auto ret = std::move(*ptr);
    ptr->~T();
    buffer[pop_pos].initialized.store(false);
    pop_pos = (pop_pos + 1) % buffer_size;
    return ret;
  }
  ~queue2() {
    while(buffer[pop_pos].initialized.load()) {
      pop_front();
    }
  }

private:
  unsigned push_pos{0};
  unsigned pop_pos{0};
  typedef typename std::aligned_storage<sizeof(T),
    alignof(T)>::type storage_type;
  struct entry {
    std::atomic<bool> initialized{false};
    storage_type storage;
  };
  entry buffer[buffer_size];
};

int main() {
  queue2<int> q;

  unsigned const count = 100;

  std::thread t1([&] {
    for(unsigned i = 0; i < count; ++i) {
      bool success = false;
      do {
        try {
          q.push_back(i);
          success = true;
        } catch(std::exception &) {
        }
      } while(!success);
    }
  });

  std::thread t2([&] {
    for(unsigned i = 0; i < count; ++i) {
      bool success = false;
      do {
        try {
          std::cout << q.pop_front() << std::endl;
          success = true;
        } catch(std::exception &) {
        }
      } while(!success);
    }
  });

  t1.join();
  t2.join();
}

// Local Variables:
// mode: c++
// fill-column: 80
// c-basic-offset: 2
// End:
