#include <atomic>
#include <assert.h>
#include <iostream>
#include <type_traits>
#include <thread>

template<typename T,unsigned buffer_size=42>
class queue3{
public:
  void push_back(T t){
    unsigned my_pos=push_pos.load();
    while(!push_pos.compare_exchange_weak(
            my_pos,(my_pos+1)%buffer_size)){}
    auto& my_entry=buffer[my_pos];
    while(my_entry.initialized.load()){}
    new(&my_entry.storage) T(t);
    my_entry.initialized.store(true);
  }
  T pop_front(){
    while(!buffer[pop_pos].initialized.load()){}
    auto ptr=static_cast<T*>(
      static_cast<void*>(&buffer[pop_pos].storage));
    auto ret=*ptr;
    ptr->~T();
    buffer[pop_pos].initialized.store(false);
    pop_pos=(pop_pos+1)%buffer_size;
    return ret;
  }
  ~queue3(){
    while(buffer[pop_pos].initialized.load()){
      pop_front();
    }
  }
private:
  std::atomic<unsigned> push_pos{0};
  unsigned pop_pos{0};
  typedef typename std::aligned_storage<
    sizeof(T),alignof(T)>::type storage_type;
  struct entry{
    std::atomic<bool> initialized{false};
    storage_type storage;
  };
  entry buffer[buffer_size];
};

int main(){
  queue3<int> q;

  unsigned const count=1000;
    
  std::thread t1([&]{
      for(unsigned i=0;i<count;++i){
        q.push_back(i);
      }
    });

  std::thread t2([&]{
      for(unsigned i=0;i<count;++i){
        q.push_back(i+10000);
      }
    });
    
  std::thread t3([&]{
      for(unsigned i=0;i<count;++i){
        q.push_back(i+20000);
      }
    });
    
  std::thread t4([&]{
      for(unsigned i=0;i<(count*3);++i){
        std::cout<<q.pop_front()<<std::endl;
      }
    });

  t1.join();
  t2.join();
  t3.join();
  t4.join();
}
