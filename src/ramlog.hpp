/*
 * This file is part of the SimSPAD distribution (http://github.com/WillMatthews/SimSPAD).
 * Copyright (c) 2022 William Matthews.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
