/*
    Copyright (C) 2020 by Pawel Soja <kernel32.pl@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;

template <typename T>
class Queue
{
public:
    void push(T data);

    T pop(uint msecs);
    T popNoWait();
    T peek(uint msecs);  

    void clear();

    // void terminate();

protected:
    std::queue<T> queue;
    std::mutex    mutex;
    std::condition_variable    cond;
};

// implementation

template <typename T>
inline void Queue<T>::push(T val)
{
    if (val == nullptr)
        return;

    std::lock_guard<std::mutex> lock(mutex);
    queue.push(std::move(val));
    cond.notify_all();
}

template <typename T>
inline T Queue<T>::pop(uint msecs)
{
    T result = nullptr;
    std::unique_lock<std::mutex> lock(mutex);
    if (cond.wait_for(lock, msecs * 1ms, [&](){ return !queue.empty(); }))
    {
        result = std::move(queue.front());
        queue.pop();
    }
    return result;
}

template <typename T>
inline T Queue<T>::popNoWait()
{
    T result = nullptr;
    std::unique_lock<std::mutex> lock(mutex);
    if (!queue.empty())
    {
        result = std::move(queue.front());
        queue.pop();
    }
    return result;
}

template <typename T>
inline T Queue<T>::peek(uint msecs)
{
    T result = nullptr;
    std::unique_lock<std::mutex> lock(mutex);
    if (cond.wait_for(lock, msecs * 1ms, [&](){ return !queue.empty(); }))
    {
        result = queue.front();
    }
    return result;
}

template <typename T>
inline void Queue<T>::clear()
{
    std::queue<T> empty;
    std::swap(queue, empty);
}
