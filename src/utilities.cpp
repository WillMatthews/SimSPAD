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
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <tuple>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "sipm.hpp"
#include "utilities.hpp"

using namespace std;

// Load experimental run data from a binary file (packaged from MATLAB)
/*
FILE FORMAT:
    double dt = x(0);
    double num_microcell = x(1);
    double vbias = x(2);
    double vbr = x(3);
    double recovery = x(4);
    double pde_Max = x(5);
    double pde_Vchr = x(6);
    double ccell = x(7);
    double pulse_fwhm = x(8);
    double digital_thresholds = x(9);

    vector<double> optical_input = x(10:end);
*/
tuple<vector<double>, SiPM> load_binary(string filename)
{
    vector<double> optical_input = {};
    ifstream fin(filename, ios::binary);
    vector<double> sipmParametersVector = {};
    int i = 0;
    for (double read; fin.read(reinterpret_cast<char *>(&read), sizeof(read));)
    {
        if (i < 10)
        {
            sipmParametersVector.push_back(read);
        }
        else
        {
            optical_input.push_back(read);
        }
        ++i;
    }

    return make_tuple(optical_input, SiPM(sipmParametersVector));
}

// Write binary file from simulation settings and output
void write_binary(string filename, SiPM sipm, vector<double> response)
{
    ofstream fout(filename, ios::out | ios::binary);

    /*
    chrono::system_clock::time_point currentTime = chrono::system_clock::now();
    time_t tt;
    tt = chrono::system_clock::to_time_t(currentTime);
    string timeStr = ctime(tt);
    */

    vector<double> sipmParametersVector = sipm.dump_configuration();

    // Concatenate response on the end of SiPM parameters vector
    sipmParametersVector.insert(sipmParametersVector.end(), response.begin(), response.end());

    // NOTE (sipm-eq-paper): was sizeof(sipmParametersVector) -- the size of the
    // std::vector object (~24 B), not of an element -- so this wrote ~3x past the
    // buffer (heap garbage tail). Must be sizeof(double).
    fout.write((char *)(&sipmParametersVector[0]), sizeof(double) * sipmParametersVector.size());
    fout.close();
}

// Output convolve to pulse shape using a gaussian approximation
vector<double> conv1d(vector<double> inputVec, vector<double> kernel)
{

    if (kernel.size() <= 1)
    {
        return inputVec;
    }

    int kernelSize = (int)kernel.size();

    if (kernelSize == 1)
    {
        return inputVec;
    }

    vector<double> outputVec = {};
    outputVec.reserve(inputVec.size());
    int innerPosition;

    for (int i = 0; i < (int)inputVec.size(); i++)
    {
        outputVec.push_back(0.0);
        for (int j = 0; j < kernelSize; j++)
        {
            innerPosition = i + j - kernelSize / 2;
            if (!(innerPosition < 0 || (innerPosition > (long)inputVec.size())))
            {
                outputVec[i] += kernel[j] * inputVec[innerPosition];
            }
        }
    }
    return outputVec;
}

// Generate Gaussian kernel
// TODO check if the pulse width is correct!
vector<double> get_gaussian(double dt, double tauFwhm)
{
    const double gaussianConstant = (double)(1 / sqrt(2 * M_PI));
    const double fwhmConversionConst = sqrt(2 * log(2)) / 2;
    const double sigma = (tauFwhm / dt) / fwhmConversionConst;
    const double numSigma = 4.0;

    int gaussianNumberOfPoints = (int)ceil(numSigma * sigma);

    vector<double> kernel = {};
    kernel.reserve(gaussianNumberOfPoints * 2);

    if (gaussianNumberOfPoints == 1)
    {
        kernel.push_back(1);
        return kernel;
    }

    double gaussianPower;

    for (int i = -(gaussianNumberOfPoints - 1); i < (gaussianNumberOfPoints - 1); i++)
    {
        gaussianPower = -pow((double)i / sigma, 2) / 2;
        kernel.push_back(gaussianConstant * exp(gaussianPower));
    }
    return kernel;
}

// Given a number `num` scale to engineering notation (nearest power of three) and return the unit prefix associated with the unit
tuple<wstring, double> exponent_val(double num)
{
    // NOTE (sipm-eq-paper): num <= 0 gave log10 = -inf, floor -> INT_MIN, and the
    // loop below then indexed prefixes[-1] (out of bounds) -> bad_alloc. Guard it.
    if (!(num > 0))
    {
        return make_tuple(wstring(L""), num);
    }
    int floor_prefix = floor(log10(num));
    wstring prefixes[9] = {L"f", L"p", L"n", L"μ", L"m", L"", L"k", L"M", L"G"};
    bool triggered = false;
    int k = 0;
    for (int i = -15; i < 10; i = i + 3)
    {
        if (floor_prefix < i)
        {
            triggered = true;
            k--;
            break;
        };
        k++;
    }
    if (!triggered)
    {
        return make_tuple(prefixes[5], num);
    }
    return make_tuple(prefixes[k], num * pow(10, -(k * 3 - 15)));
}

// Linearly space num_in points between values start_in and end_in
// Template removed as I am stupid and keep getting undefined reference errors.
// template <typename T>
// vector<double> linspace(T start_in, T end_in, int num_in)
vector<double> linspace(double start_in, double end_in, int num_in)
{

    vector<double> linspaced;
    linspaced.reserve(num_in);

    double start = static_cast<double>(start_in);
    double end = static_cast<double>(end_in);
    double num = static_cast<double>(num_in);

    if (num == 0)
    {
        return linspaced;
    }
    if (num == 1)
    {
        linspaced.push_back(start);
        return linspaced;
    }

    double delta = (end - start) / (num - 1);

    for (int i = 0; i < num - 1; ++i)
    {
        linspaced.push_back(start + delta * i);
    }
    linspaced.push_back(end); // I want to ensure that start and end
                              // are exactly the same as the input
    return linspaced;
}

// ===========================================================================
// .npy waveform IO
// ===========================================================================

// Pull descr (dtype string) and the flattened element count out of a .npy
// header dictionary, e.g. "{'descr': '<f8', 'fortran_order': False,
// 'shape': (12345,), }".
static void parse_npy_descr_shape(const string &hdr, string &descr, size_t &count)
{
    size_t d = hdr.find("descr");
    size_t colon = hdr.find(':', d);
    size_t q1 = hdr.find('\'', colon);
    size_t q2 = hdr.find('\'', q1 + 1);
    if (d == string::npos || q1 == string::npos || q2 == string::npos)
        throw runtime_error("malformed .npy header (descr)");
    descr = hdr.substr(q1 + 1, q2 - q1 - 1);

    size_t s = hdr.find("shape");
    size_t op = hdr.find('(', s);
    size_t cp = hdr.find(')', op);
    if (s == string::npos || op == string::npos || cp == string::npos)
        throw runtime_error("malformed .npy header (shape)");
    string inside = hdr.substr(op + 1, cp - op - 1);

    // Multiply every integer in the shape tuple -> flat element count.
    count = 1;
    bool any = false;
    for (size_t p = 0; p < inside.size();)
    {
        if (isdigit((unsigned char)inside[p]))
        {
            char *endp = nullptr;
            unsigned long v = strtoul(inside.c_str() + p, &endp, 10);
            count *= (size_t)v;
            any = true;
            p = (size_t)(endp - inside.c_str());
        }
        else
        {
            ++p;
        }
    }
    if (!any)
        count = 0; // shape () -> scalar / empty
}

// Build a version-1.0 .npy header for a 1-D little-endian float64 array of
// `count` elements, padded so the total preamble is a multiple of 64 bytes.
static string build_npy_header(size_t count)
{
    ostringstream dict;
    dict << "{'descr': '<f8', 'fortran_order': False, 'shape': (" << count << ",), }";
    string d = dict.str();

    size_t unpadded = 10 + d.size() + 1; // 6 magic + 2 version + 2 len + dict + '\n'
    size_t pad = (64 - (unpadded % 64)) % 64;
    d.append(pad, ' ');
    d.push_back('\n');

    uint16_t len = (uint16_t)d.size();
    string out;
    out.push_back((char)0x93);
    out += "NUMPY";
    out.push_back((char)0x01); // major version
    out.push_back((char)0x00); // minor version
    out.push_back((char)(len & 0xff));
    out.push_back((char)((len >> 8) & 0xff));
    out += d;
    return out;
}

NpyReader::NpyReader(const string &filename)
    : fin(filename, ios::binary), nElems(0)
{
    if (!fin)
        throw runtime_error("cannot open .npy file: " + filename);

    char magic[6];
    fin.read(magic, 6);
    if (fin.gcount() != 6 || memcmp(magic, "\x93NUMPY", 6) != 0)
        throw runtime_error("not a .npy file: " + filename);

    unsigned char ver[2];
    fin.read(reinterpret_cast<char *>(ver), 2);

    size_t hlen;
    if (ver[0] == 1)
    {
        unsigned char b[2];
        fin.read(reinterpret_cast<char *>(b), 2);
        hlen = (size_t)b[0] | ((size_t)b[1] << 8);
    }
    else
    {
        unsigned char b[4];
        fin.read(reinterpret_cast<char *>(b), 4);
        hlen = (size_t)b[0] | ((size_t)b[1] << 8) | ((size_t)b[2] << 16) | ((size_t)b[3] << 24);
    }

    string hdr(hlen, '\0');
    fin.read(&hdr[0], (streamsize)hlen);

    string descr;
    size_t cnt;
    parse_npy_descr_shape(hdr, descr, cnt);
    if (descr.find("f8") == string::npos)
        throw runtime_error("unsupported .npy dtype (need float64): " + descr);
    if (!descr.empty() && descr[0] == '>')
        throw runtime_error("unsupported .npy byte order (need little-endian): " + descr);

    nElems = cnt;
    dataStart = fin.tellg();
}

size_t NpyReader::read(double *buf, size_t n)
{
    fin.read(reinterpret_cast<char *>(buf), (streamsize)(n * sizeof(double)));
    return (size_t)(fin.gcount() / (streamsize)sizeof(double));
}

void NpyReader::rewind()
{
    fin.clear();
    fin.seekg(dataStart);
}

NpyWriter::NpyWriter(const string &filename, size_t count)
    : fout(filename, ios::binary)
{
    if (!fout)
        throw runtime_error("cannot open .npy file for writing: " + filename);
    string hdr = build_npy_header(count);
    fout.write(hdr.data(), (streamsize)hdr.size());
}

void NpyWriter::write(const double *buf, size_t n)
{
    fout.write(reinterpret_cast<const char *>(buf), (streamsize)(n * sizeof(double)));
}

void NpyWriter::close()
{
    fout.close();
}

// ===========================================================================
// Flat-JSON device parameters
// ===========================================================================

// Minimal parser for a flat JSON object of "name": number pairs. Sufficient
// for the fixed device-parameter schema; not a general JSON parser.
map<string, double> parse_flat_json(const string &s)
{
    map<string, double> m;
    size_t i = 0;
    while (true)
    {
        size_t q1 = s.find('"', i);
        if (q1 == string::npos)
            break;
        size_t q2 = s.find('"', q1 + 1);
        if (q2 == string::npos)
            break;
        string key = s.substr(q1 + 1, q2 - q1 - 1);

        size_t colon = s.find(':', q2 + 1);
        if (colon == string::npos)
            break;
        size_t p = colon + 1;
        while (p < s.size() && isspace((unsigned char)s[p]))
            ++p;

        const char *start = s.c_str() + p;
        char *endp = nullptr;
        double val = strtod(start, &endp);
        if (endp != start)
        {
            m[key] = val;
            i = p + (size_t)(endp - start);
        }
        else
        {
            i = colon + 1; // non-numeric value: skip (not expected in our schema)
        }
    }
    return m;
}

// Parameter key order matches SiPM::dump_configuration().
static const char *kParamKeys[10] = {
    "dt", "numMicrocell", "vBias", "vBr", "tauRecovery",
    "pdeMax", "vChr", "cCell", "tauFwhm", "digitalThreshold"};

SiPM load_params_json(const string &filename)
{
    ifstream f(filename, ios::binary);
    if (!f)
        throw runtime_error("cannot open params file: " + filename);
    stringstream ss;
    ss << f.rdbuf();
    map<string, double> m = parse_flat_json(ss.str());

    vector<double> svars(10);
    for (int i = 0; i < 10; i++)
    {
        auto it = m.find(kParamKeys[i]);
        if (it == m.end())
            throw runtime_error(string("params JSON missing key: ") + kParamKeys[i]);
        svars[i] = it->second;
    }
    return SiPM(svars);
}

string sipm_to_json(SiPM &sipm)
{
    vector<double> c = sipm.dump_configuration();
    ostringstream o;
    o << setprecision(17);
    o << "{\n";
    for (int i = 0; i < 10; i++)
    {
        o << "  \"" << kParamKeys[i] << "\": ";
        if (i == 1)
            o << (unsigned long)c[i]; // numMicrocell as an integer
        else
            o << c[i];
        o << (i < 9 ? ",\n" : "\n");
    }
    o << "}\n";
    return o.str();
}

void save_params_json(const string &filename, SiPM &sipm)
{
    ofstream f(filename, ios::binary);
    if (!f)
        throw runtime_error("cannot open params file for writing: " + filename);
    f << sipm_to_json(sipm);
}
