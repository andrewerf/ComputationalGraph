#include <iostream>
#include <functional>
#include "ComputationalGraph/DelayQueue.hpp"

using namespace std::chrono_literals;

int main()
{
    DelayQueue<std::function<void()>> q;
    q.push([]{
        std::cout << 1 << std::endl;
    }, 10s);

    std::thread t([&q]{
        std::this_thread::sleep_for(2s);
        q.push([&q]{
            std::cout << 2 << std::endl;
        }, 0s);
    });
    t.detach();

    q.popWait()();
    q.popWait()();

    return 0;
}
