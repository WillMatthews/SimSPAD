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

#include <iostream>
#include <string>
#include "performance.hpp"
#include "current_accuracy.hpp"

using namespace std;

int main()
{
    unsigned int time_ui = static_cast<unsigned int>(time(NULL));
    srand(time_ui);

    ios::sync_with_stdio(false);
    cin.tie(NULL);
    wcin.tie(NULL);

    locale::global(locale("en_US.utf8"));
    cout.imbue(locale());
    wcout.imbue(locale());

    bool passed = true;

    passed = passed && TEST_currents();
    passed = passed && TEST_performance();

    if (passed)
    {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
