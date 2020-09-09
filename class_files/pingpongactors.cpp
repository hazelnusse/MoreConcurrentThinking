#include "actors.hpp"
#include <iostream>
#include <chrono>

struct pingpong_message {
  messaging::sender other;
};

class pingpong_actor : public messaging::actor {
  std::string message;

public:
  explicit pingpong_actor(std::string const &message_) :
    message(message_) {}
  void run();
};

void pingpong_actor::run() {
  while(true) {
    incoming.wait().handle<pingpong_message>(
      [&](pingpong_message &msg) {
        std::cout << message << std::endl;
        msg.other.send(pingpong_message{get_sender()});
      });
  }
}

int main() {
  pingpong_actor ping("ping");
  pingpong_actor pong("pong");

  ping.start();
  pong.start();
  ping.send(pingpong_message{pong.get_sender()});

  std::this_thread::sleep_for(std::chrono::seconds(1));
}
