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

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Light speed in m/s
const double speedOfLight = 2.99792458e8;

// Boltzmann's constant in eV/K
const double kBoltzmann = 8.617333262e-5;

// Boltzmann's constant in J/K
const double kBoltzmannJ = 1.380658e-23;

// Stefan-Boltzmann constant in W/m^2/K^4
const double sigmaStefanBoltzmann = 5.67051e-8;

// Planck's constant in J s
const double hPlanck = 6.6260755e-34;

// eV to Joule conversion constant in J/eV
const double eV2Joule = 1.602176565e-19;

// Joule to eV conversion constant in  eV/J
const double Joule2eV = 1.0 / eV2Joule;

// Elementary Charge (single Electron) in C
const double elementaryCharge = 1.60217663e-19;

#endif
