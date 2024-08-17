#include <stdint.h>

#include "aero/core/threads.h"
#include "aero/core/log.h"

aero::ThreadPool::ThreadPool(size_t workers)
{
    for (size_t i = 0; i < workers; ++i)
    {
        m_Threads.emplace_back(
            [this](std::stop_token stop)
            {
                while (!stop.stop_requested())
                {
                    ThreadFn task;
                    uint64_t  thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
                    {
                        std::unique_lock<std::mutex> lock(m_QueueMutex);

                        LOG_DEBUG_TAG("ThreadPool", "Waiting for work on Thread {0}", thread_id);
                        m_Condition.wait(lock, [this]() { return !m_Tasks.empty() || m_Stop; });

                        if (m_Stop && m_Tasks.empty())
                            return;

                        task = std::move(m_Tasks.front());
                        m_Tasks.pop();
                    }
                    LOG_DEBUG_TAG("ThreadPool", "Job running on Thread {0}", thread_id);
                    task();
                    LOG_DEBUG_TAG("ThreadPool", "Job done on Thread {0}", thread_id);
                }
            }
        );
    }
}

aero::ThreadPool::~ThreadPool()
{
    KillAll();
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Stop = true;
    }

    m_Condition.notify_all();
        /*t.request_stop();*/
}

void aero::ThreadPool::Enqueue(ThreadFn fn, std::string name)
{
    {
        LOG_DEBUG_TAG("ThreadPool", "Queueing `{0}` function in thread pool", name);
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Tasks.emplace(std::move(fn));
    }
    m_Condition.notify_one();
}

void aero::ThreadPool::KillAll()
{
    
    for (auto& t : m_Threads)
    {   
        LOG_DEBUG_TAG("ThreadPool", "Reqesting stop on {}", std::hash<std::thread::id>{}(t.get_id()));
        // LOG_DEBUG_TAG("ThreadPool", "Reqesting stop on {}", t.get_id());
        bool to_stop = t.request_stop();
    }
}
