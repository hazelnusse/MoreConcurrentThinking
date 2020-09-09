class petersons_mutex {
  std::atomic<bool> flag[2];
  std::atomic<int> turn;
public:
  petersons_mutex():
    turn(0){
    flag[0]=false;
    flag[1]=false;
  }
  
  void lock(unsigned index) {
    unsigned const other=index?0:1;
    flag[index].store(true, std::memory_order_relaxed);
    turn.exchange(other, std::memory_order_relaxed);
    
    while (flag[other].load(std::memory_order_relaxed) &&
           other == turn.load(std::memory_order_relaxed))
      std::this_thread::yield();
  }
  
  void unlock(unsigned index) {
    flag[index].store(false, std::memory_order_relaxed);
  }
};

