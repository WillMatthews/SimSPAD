simspad: ./src/main.cpp ./src/sipm.cpp ./src/utilities.cpp
	g++ --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread -L./lib  -o simspad ./src/main.cpp ./src/sipm.cpp ./src/utilities.cpp
