#ifndef _AERO_CORE_THREADS_H_
#define _AERO_CORE_THREADS_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <string>


namespace aero
{

typedef std::function<void()> ThreadFn;

class ThreadPool
{
public:
    /*using ThreadFn = std::function<void()>;*/


public:
    ThreadPool(size_t workers = std::thread::hardware_concurrency());
    ~ThreadPool();

    void Enqueue(ThreadFn fn, std::string name = "");
    void KillAll();
    void KillThread(); // need a way to keep track of threads

private:
    std::vector<std::jthread> m_Threads;
    std::queue<ThreadFn> m_Tasks;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;

    bool m_Stop = false;
};

} // namespace aero
#endif // _AERO_CORE_THREADS_H_
