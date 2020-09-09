#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <assert.h>
#include <iostream>
#include <thread>

template<typename T>
class queue1{
public:
  void push_back(T t){
    {
      std::lock_guard<std::mutex> guard(m);
      q.push(t);
    }
    c.notify_one();
  }
  T pop_front(){
    std::unique_lock<std::mutex> guard(m);
    c.wait(guard,[=]{return !q.empty();});
    auto ret=q.front();
    q.pop();
    return ret;
  }
private:
  std::mutex m;
  std::condition_variable c;
  std::queue<T> q;
};

int main(){
  queue1<int> q;

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
