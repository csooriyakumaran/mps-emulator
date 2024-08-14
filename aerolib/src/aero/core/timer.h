#ifndef _AERO_CORE_TIMER_H_
#define _AERO_CORE_TIMER_H_

#include <chrono>
#include <iostream>
#include <string>

namespace aero
{

class Timer
{
public:
    Timer() { Reset(); }

    void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

    float Elapsed()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
    }

    float ElapsedMilliseconds() { return Elapsed() * 1000.0f; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};

class ScopedTimer
{
public:
    ScopedTimer(const std::string& name)
        : m_Name(name)
    {
    }
    ~ScopedTimer()
    {
        float time = m_Timer.ElapsedMilliseconds();
        std::cout << "[TIMER] " << m_Name << " - " << time << " ms\n";
    }

private:
    std::string m_Name;
    Timer m_Timer;
};

} // namespace aero

#endif // _AERO_CORE_TIMER_H_
