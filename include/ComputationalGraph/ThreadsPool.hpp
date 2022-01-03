#ifndef COMPUTATIONALGRAPH_CPP_THREADSPOOL_H
#define COMPUTATIONALGRAPH_CPP_THREADSPOOL_H

#include <functional>
#include <ComputationalGraph/DelayQueue.hpp>


template <typename D = std::chrono::system_clock::duration>
class ThreadsPool
{
public:
    using JobType = std::function<void()>;
    enum RepeatableStrategy
    {
        Periodic, Interval
    };

    explicit ThreadsPool(int threadsCount);
    ~ThreadsPool();

    template <class JT>
    void submit(JT &&job);

    template <class JT>
    void submitDelayed(JT &&job, D delay);

    void submitRepeatable(JobType job, D repeatTime, RepeatableStrategy repeatableStrategy, bool delayed);

    template<class JT>
    void submitPeriodic(JT &&job, D repeatTime, bool delayed)
    { submitRepeatable(std::forward<JT>(job), repeatTime, Periodic, delayed); }

    template<class JT>
    void submitInterval(JT &&job, D repeatTime, bool delayed)
    { submitRepeatable(std::forward<JT>(job), repeatTime, Interval, delayed); }

    size_t size() const
    {return jobsQueue.size();}

private:
    void threadFunction();

    DelayQueue<JobType> jobsQueue;
    std::vector<std::thread> threads;
    std::atomic<bool> running;
};


template<typename D>
ThreadsPool<D>::ThreadsPool(int threadsCount):
    running(true)
{
    for(int i = 0; i < threadsCount; ++i)
        threads.emplace_back(&ThreadsPool::threadFunction, this);
}

template <typename D>
template <class JT>
void ThreadsPool<D>::submit(JT &&job)
{
    jobsQueue.push(std::forward<JT>(job), D(0));
}

template <typename D>
template <class JT>
void ThreadsPool<D>::submitDelayed(JT &&job, D delay)
{
    jobsQueue.push(std::forward<JT>(job), delay);
}

template <typename D>
void ThreadsPool<D>::submitRepeatable(JobType job, D repeatTime, RepeatableStrategy repeatableStrategy, bool delayed)
{
    // TODO: perfect lambda capture
    auto repeatableJob = [this, job, repeatTime, repeatableStrategy]{
        submitRepeatable(job, repeatTime, repeatableStrategy, false);
    };

    if(repeatableStrategy == Periodic)
    {
        submitDelayed(repeatableJob, repeatTime);
        if(!delayed)
            job();
    }
    else if(repeatableStrategy == Interval)
    {
        if(!delayed)
            job();
        submitDelayed(repeatableJob, repeatTime);
    }
}

template<typename D>
void ThreadsPool<D>::threadFunction()
{
    while(running)
    {
        auto job = jobsQueue.popWait(std::chrono::milliseconds(1));
        if(job)
            job.value()();
    }
}

template<typename D>
ThreadsPool<D>::~ThreadsPool()
{
    running = false;
    for(auto &t : threads)
        t.join();
}


#endif //COMPUTATIONALGRAPH_CPP_THREADSPOOL_H
