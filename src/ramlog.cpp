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
