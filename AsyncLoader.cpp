#include "AsyncLoader.h"
#include <iostream>

AsyncLoader& AsyncLoader::Get()
{
    static AsyncLoader instance;
    return instance;
}

std::future<void> AsyncLoader::Submit(Task task)
{
    mActiveTasks++;

    auto future = std::async(std::launch::async,
        [this, task = std::move(task)]() {
            try {
                task();
            }
            catch (const std::exception& e) {
                std::cerr << "[AsyncLoader] Task exception: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "[AsyncLoader] Unknown task exception" << std::endl;
            }
            mActiveTasks--;
        });

    // خزّنها عشان نتأكد إنها خلصت لما البرنامج يقفل
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPending.push_back(std::move(future));
    }

    return {};
}

void AsyncLoader::PostToMain(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mMainQueue.push(std::move(callback));
}

void AsyncLoader::Poll()
{
    std::queue<std::function<void()>> localQueue;

    {
        std::lock_guard<std::mutex> lock(mMutex);
        std::swap(localQueue, mMainQueue);
    }

    while (!localQueue.empty()) {
        auto cb = std::move(localQueue.front());
        localQueue.pop();
        cb();
    }
}

size_t AsyncLoader::PendingCount() const
{
    return mActiveTasks.load();
}

size_t AsyncLoader::QueueSize() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mMainQueue.size();
}

AsyncLoader::~AsyncLoader()
{
    for (auto& f : mPending)
        if (f.valid()) f.wait();
}