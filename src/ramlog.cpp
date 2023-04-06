#include "ramlog.hpp"

RamLog &RamLog::getInstance()
{
    static RamLog instance;
    return instance;
}

void RamLog::log(const std::string &message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    logMessages_.push_back(message);
    trimLog();
}

std::string RamLog::getLog() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string log;
    for (const auto &message : logMessages_)
    {
        log += message + '\n';
    }
    return log;
}

void RamLog::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    logMessages_.clear();
}

void RamLog::trimLog()
{
    size_t totalSize = 0;
    for (const auto &message : logMessages_)
    {
        totalSize += message.size();
    }
    while (totalSize > MAX_SIZE && !logMessages_.empty())
    {
        totalSize -= logMessages_.front().size();
        logMessages_.pop_front();
    }
}
