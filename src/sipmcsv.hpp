
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <stdlib.h>
#include "sipm.hpp"

using namespace std;



// CSV Handling Functions
int writeCSV(string filename, vector<double> inputVec, SiPM sipm){
    double dt = sipm.dt;
    ofstream csv;
    csv.open(filename);
    csv << "time, qOut\n";
    double t = 0;
    for (int i=0; i<inputVec.size(); i++){
        csv << t << "," << inputVec[i] << "\n";
        //cout << t << "," << inputVec[i] << "\n";
        t += dt;
    }
    csv.close();
    return 0;
}

class CSVRow
{
    public:
        string_view operator[](size_t index) const
        {
            return string_view(&m_line[m_data[index] + 1], m_data[index + 1] -  (m_data[index] + 1));
        }
        size_t size() const
        {
            return m_data.size() - 1;
        }
        void readNextRow(istream& str)
        {
            getline(str, m_line);

            m_data.clear();
            m_data.emplace_back(-1);
            string::size_type pos = 0;
            while((pos = m_line.find(',', pos)) != string::npos)
            {
                m_data.emplace_back(pos);
                ++pos;
            }
            // This checks for a trailing comma with no data after it.
            pos   = m_line.size();
            m_data.emplace_back(pos);
        }
    private:
        string         m_line;
        vector<int>    m_data;
};

istream& operator>>(istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

    class CSVIterator
    {   
        public:
            typedef input_iterator_tag     iterator_category;
            typedef CSVRow                      value_type;
            typedef size_t                 difference_type;
            typedef CSVRow*                     pointer;
            typedef CSVRow&                     reference;

            CSVIterator(istream& str)  :m_str(str.good()?&str:nullptr) { ++(*this); }
            CSVIterator()                   :m_str(nullptr) {}

            // Pre Increment
            CSVIterator& operator++()               {if (m_str) { if (!((*m_str) >> m_row)){m_str = nullptr;}}return *this;}
            // Post increment
            CSVIterator operator++(int)             {CSVIterator    tmp(*this);++(*this);return tmp;}
            CSVRow const& operator*()   const       {return m_row;}
            CSVRow const* operator->()  const       {return &m_row;}

            bool operator==(CSVIterator const& rhs) {return ((this == &rhs) || ((this->m_str == nullptr) && (rhs.m_str == nullptr)));}
            bool operator!=(CSVIterator const& rhs) {return !((*this) == rhs);}
        private:
            istream*       m_str;
            CSVRow              m_row;
    };

    class CSVRange
    {
        istream&   stream;
        public:
            CSVRange(istream& str)
                : stream(str)
            {}
            CSVIterator begin() const {return CSVIterator{stream};}
            CSVIterator end()   const {return CSVIterator{};}
    };


    tuple<vector<double>, double> readCSV(string filename){
        ifstream file(filename);

        vector<double> time = {};
        vector<double> photons_per_dt = {};
        double t;
        double photon;
        long rowcnt = 0;
        for(auto& row: CSVRange(file))
        {
            rowcnt +=1;
            if (rowcnt <= 1){
                continue;
            }
            t = (double) stod((string) row[0]);
            photon = (double) stod((string) row[1]);
            //cout << t << "   \t" << photon << "\t" << endl;
            time.push_back(t);
            photons_per_dt.push_back(photon);
        }
        double dt = (time[20]-time[10])/10;
        return make_tuple(photons_per_dt, dt);
    }
