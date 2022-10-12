simspad: simspad.cpp sipm.hpp sipmcsv.hpp
	g++ --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread simspad.cpp -o simspad
