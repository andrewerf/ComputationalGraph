#ifndef COMPUTATIONALGRAPH_CPP_DELAYQUEUE_H
#define COMPUTATIONALGRAPH_CPP_DELAYQUEUE_H

#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>


template <typename T, typename D = std::chrono::system_clock::duration, typename P = std::chrono::system_clock::time_point>
class DelayQueue
{
public:
    DelayQueue();

    template <typename TT>
    void push(TT &&val, D delay);

    std::optional<T> pop();

    T popWait();

    size_t size() const;
    bool empty() const;

private:
    using delayedElem = std::tuple<T, D, P>;
    static bool compare(const delayedElem &p1, const delayedElem &p2);
    static D getDelay(const delayedElem &p);

    std::priority_queue<delayedElem, std::vector<delayedElem>, decltype(&DelayQueue<T, D, P>::compare)> priorityQueue;
    std::mutex queueMutex;
    std::condition_variable available;
};

template<typename T, typename D, typename P>
DelayQueue<T, D, P>::DelayQueue():
    priorityQueue(compare)
{}

template<typename T, typename D, typename P>
template<typename TT>
void DelayQueue<T, D, P>::push(TT &&val, D delay)
{
    auto elem = std::tuple{std::forward<TT>(val), delay, std::chrono::system_clock::now()};
    std::unique_lock lock(queueMutex);
    bool notify = priorityQueue.empty() || compare(priorityQueue.top(), elem);
    priorityQueue.push(elem);

    // if there were no elements before pushing
    // or pushed element's delay is less than last head
    if(notify)
        available.notify_one();
}

template<typename T, typename D, typename P>
std::optional<T> DelayQueue<T, D, P>::pop()
{
    if(priorityQueue.empty())
        return {};

    std::unique_lock lock(queueMutex);
    auto head = priorityQueue.top();

    if(getDelay(head).count() > 0)
        return {};

    priorityQueue.pop();
    if(!priorityQueue.empty())
        available.notify_one();

    return std::get<0>(head);
}

template<typename T, typename D, typename P>
T DelayQueue<T, D, P>::popWait()
{
    std::unique_lock lock(queueMutex);
    available.wait(lock, [this]{
        return !priorityQueue.empty();
    });

    auto head = priorityQueue.top();
    auto delay = getDelay(head);
    available.wait_for(lock, delay, [&head, this]{return getDelay(priorityQueue.top()).count() <= 0;});

    head = priorityQueue.top();
    priorityQueue.pop();
    if(!priorityQueue.empty())
        available.notify_one();
    return std::get<0>(head);
}

template<typename T, typename D, typename P>
bool DelayQueue<T, D, P>::compare(const delayedElem &p1, const delayedElem &p2)
{
    return (std::get<2>(p1) + std::get<1>(p1)) > (std::get<2>(p2) + std::get<1>(p2));
}

template<typename T, typename D, typename P>
D DelayQueue<T, D, P>::getDelay(const delayedElem &p)
{
    auto now = std::chrono::system_clock::now();
    auto duration = std::get<1>(p);
    auto time_point = std::get<2>(p);

    return duration - (now - time_point);
}

template<typename T, typename D, typename P>
size_t DelayQueue<T, D, P>::size() const
{
    return priorityQueue.size();
}

template<typename T, typename D, typename P>
bool DelayQueue<T, D, P>::empty() const
{
    return size() == 0;
}


#endif //COMPUTATIONALGRAPH_CPP_DELAYQUEUE_H
