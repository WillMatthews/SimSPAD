#ifndef RAM_LOG_H
#define RAM_LOG_H

#include <string>
#include <deque>
#include <mutex>

class RamLog
{
public:
    static RamLog &getInstance();

    void log(const std::string &message);

    std::string getLog() const;

    void clear();

    RamLog(RamLog const &) = delete;
    void operator=(RamLog const &) = delete;

private:
    RamLog() = default;

    std::deque<std::string> logMessages_;
    mutable std::mutex mutex_;
    const size_t MAX_SIZE = 512 * 1024; // 512K

    void trimLog();
};

#endif // RAM_LOG_H
