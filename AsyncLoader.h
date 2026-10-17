#pragma once
#include <future>
#include <queue>
#include <mutex>
#include <functional>
#include <string>
#include <vector>
#include <atomic>

class AsyncLoader {
public:
    using Task = std::function<void()>;

    static AsyncLoader& Get();

    // شغّل مهمة في الخلفية
    std::future<void> Submit(Task task);

    // نداء من الـ main thread كل frame
    void Poll();

    // سجّل callback يشتغل في الـ main thread
    void PostToMain(std::function<void()> callback);

    // عدد المهام اللي لسه شغالة
    size_t PendingCount() const;

    // عدد المهام الجاهزة في الـ queue
    size_t QueueSize() const;

    ~AsyncLoader();

private:
    AsyncLoader() = default;

    std::vector<std::future<void>> mPending;
    std::queue<std::function<void()>> mMainQueue;
    mutable std::mutex mMutex;
    std::atomic<int> mActiveTasks{ 0 };
};