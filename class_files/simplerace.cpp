#include <thread>
#include <iostream>
unsigned i = 0;
void func() {
  for(unsigned c = 0; c < 2000000; ++c)
    ++i;
  for(unsigned c = 0; c < 2000000; ++c)
    --i;
}
int main() {
  std::thread t1(func), t2(func);
  t1.join();
  t2.join();
  std::cout << "Final i=" << i << std::endl;
}
