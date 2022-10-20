simspad: ./src/main.cpp ./src/sipm.cpp ./src/sipmcsv.cpp ./src/utilities.cpp
	#cd ./src

	#g++ -v --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread -o simspad ./main.cpp ./sipm.cpp ./sipmcsv.cpp ./utilities.cpp
	g++ --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread -iprefix ./src/ -o simspad ./src/main.cpp ./src/sipm.cpp ./src/sipmcsv.cpp ./src/utilities.cpp
	#g++ --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread -o simspad ./src/main.cpp
