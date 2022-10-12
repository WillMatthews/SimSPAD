simspad: ./src/simspad.cpp ./src/sipm.hpp ./src/sipmcsv.hpp
	g++ --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread ./src/simspad.cpp -o simspad
