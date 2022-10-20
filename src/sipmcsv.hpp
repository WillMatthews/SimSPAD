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

#ifndef SIPM_CSV_H
#define SIPM_CSV_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <stdlib.h>
#include "sipm.hpp"

int writeCSV(string filename, vector<double> inputVec, SiPM sipm);

class CSVRow
{
public:
    string_view operator[](size_t index) const;
    size_t size() const;
    void readNextRow(istream &str);

private:
    string m_line;
    vector<int> m_data;
};

istream &operator>>(istream &str, CSVRow &data);

class CSVIterator
{
public:
    typedef input_iterator_tag iterator_category;
    typedef CSVRow value_type;
    typedef size_t difference_type;
    typedef CSVRow *pointer;
    typedef CSVRow &reference;

    CSVIterator(istream &str);
    CSVIterator();

    CSVIterator &operator++();
    CSVIterator operator++(int);
    CSVRow const &operator*() const;
    CSVRow const *operator->() const;

    bool operator==(CSVIterator const &rhs);
    bool operator!=(CSVIterator const &rhs);

private:
    istream *m_str;
    CSVRow m_row;
};

class CSVRange
{
    istream &stream;

public:
    CSVRange(istream &str)
        : stream(str)
    {
    }
    CSVIterator begin() const { return CSVIterator{stream}; }
    CSVIterator end() const { return CSVIterator{}; }
};

tuple<vector<double>, double> readCSV(string filename);

#endif // SIPM_CSV_H
