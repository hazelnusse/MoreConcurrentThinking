#include <atomic>
#include <assert.h>
#include <iostream>
#include <type_traits>
#include <thread>
#include <sstream>

constexpr unsigned padding_size=128;

template<typename T,unsigned buffer_size=42>
class queue5{
public:
  void push_back(T t){
    auto my_entry=allocate_entry();
    new(&my_entry->storage) T(t);
    my_entry->next=nullptr;
    my_entry->prev=tail.load();
    while(!tail.compare_exchange_weak(
            my_entry->prev,my_entry)){}
  }
  T pop_front(){
    entry* old_head=head;
    while(!old_head)
      old_head=chase_tail();
    head=old_head->next;
    auto ptr=static_cast<T*>(static_cast<void*>(&old_head->storage));
    auto ret=*ptr;
    ptr->~T();
    recycle_node(old_head);
    return ret;
  }
  ~queue5(){
    while(head || tail.load())
      pop_front();
  }
private:
  typedef typename std::aligned_storage<sizeof(T),alignof(T)>::type storage_type;
  struct entry{
    std::atomic<bool> in_use{false};
    char padding1[padding_size];
    storage_type storage;
    entry* next{nullptr};
    entry* prev{nullptr};
    char padding2[padding_size];
  };

  std::atomic<unsigned> push_hint{0};
  char padding1[padding_size];
  entry* head{nullptr};
  char padding2[padding_size];
  std::atomic<entry*> tail{nullptr};
  char padding3[padding_size];
  entry buffer[buffer_size];

  entry* chase_tail(){
    entry* next=tail.exchange(nullptr);
    if(!next)
      return nullptr;
        
    while(next->prev){
      next->prev->next=next;
      next=next->prev;
    }
    return next;
  }
  entry* allocate_entry(){
    unsigned my_pos=push_hint.load();
    while(!push_hint.compare_exchange_weak(
            my_pos,(my_pos+1)%buffer_size)){}
    while(buffer[my_pos].in_use.exchange(true))
      my_pos=(my_pos+1)%buffer_size;
    return &buffer[my_pos];
  }

  void recycle_node(entry* my_entry){
    my_entry->in_use.store(false);
  }
    
};

int main(){
  queue5<int> q;

  unsigned const count=10000000;

  typedef std::chrono::high_resolution_clock clock;

  clock::time_point start,end;

  start=clock::now();
  
  std::thread t1([&]{
      for(unsigned i=0;i<count;++i){
        q.push_back(i+30*count);
      }
    });

  std::thread t2([&]{
      for(unsigned i=0;i<count;++i){
        q.push_back(i+10*count);
      }
    });
    
  std::thread t3([&]{
      for(unsigned i=0;i<count;++i){
        q.push_back(i+20*count);
      }
    });
    
  std::thread t4([&]{
      for(unsigned i=0;i<(count*3);++i){
        q.pop_front();
        
        // std::cout<<q.pop_front()<<std::endl;
      }
    });

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  end=clock::now();
  std::cout<<"total time: "<<std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<<"ms"<<std::endl;
  
}
