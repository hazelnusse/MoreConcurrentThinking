#include <functional>
#include <future>
#include <iostream>

struct basic_executor
{
    void submit(std::function<void()> task) {
      task();
    }
};

struct wrapper_executor
{
    wrapper_executor(basic_executor& ex) : ex_{ex} {};

    std::future<void> submit(std::function<void()> task)
    {
        auto wrapper = std::make_shared<std::packaged_task<void()>>(task);
        ex_.submit([wrapper] { (*wrapper)(); });
        return wrapper->get_future();
    }


private:
    basic_executor& ex_;
};

void print_something() {
    std::cout << "42\n";
}

int main()
{
    basic_executor be;
    wrapper_executor we(be);
    auto future = we.submit(print_something);
    future.wait();
}
