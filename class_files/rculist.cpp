#include <jss/rcu.hpp>
#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <vector>


template<typename T>
struct concurrent_list{
    struct node{
        T value;
        std::atomic<node*> next;
    };

    std::mutex writer_mutex;
    std::atomic<node*> head;
    
    concurrent_list():
        head(nullptr){}

    void add(T value){
        auto *new_node=new node{value,nullptr};
        std::lock_guard guard(writer_mutex);

        new_node->next=head.load();
        head.store(new_node);
    }

    template<typename Func>
    void remove_if(Func f){
        std::vector<node*> del_list;
        {   std::lock_guard guard(writer_mutex);
            auto *prev=&head; auto *p=head.load();
            while(p){
                if(f(p->value)){
                    del_list.push_back(p);
                    *prev=p->next.load();
                } else prev=&p->next;
                p=p->next;
            }
        }
        jss::rcu_synchronize();
        for(auto& p:del_list) delete p;
    }

    template<typename Func>
    void for_each(Func f){
        jss::rcu_reader guard;
        auto p=head.load();
        while(p){
            f(p->value);
            p=p->next;
        }
    }
    
};


int main(){
    concurrent_list<int> list;
    list.add(42);

    auto reader=[&]{
        for(unsigned c=0;c<1000;++c){
            std::ostringstream os;
            os<<std::this_thread::get_id()<<": ";
            
            list.for_each([&](int i){
                os<<i<<",";
            });
            os<<"\n";
            std::cout<<os.str();
            std::cout.flush();
        }
    };

    std::thread r1{reader};
    std::thread r2{reader};
    std::thread r3{reader};
    
    std::thread w1([&]{
        for(unsigned c=0;c<1000;++c){
            list.add(rand());
        }
    });
    
    std::thread w2([&]{
        for(unsigned c=0;c<1000;++c){
            unsigned const low=rand()&0xf;
            list.remove_if([&](int i){
                return (i&0xf)==low;
            });
        }
    });


    w2.join();
    w1.join();
    r3.join();
    r2.join();
    r1.join();
}
