CXX		 := g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror --std=c++17 -O3 -ffast-math -funsafe-math-optimizations -msse4.2 -pthread
LDFLAGS	 := -L/usr/lib -lstdc++ -lm
BUILD	 := ./build
OBJ_DIR	 := $(BUILD)/objects
APP_DIR	 := $(BUILD)/apps
LIB_DIR	 := ./lib
TARGET	 := simspad
TARGET_SERVER	 := server
INCLUDE	 := -Ilib/
SRC_ALL	 := $(wildcard src/*.cpp)
SRC		 := $(filter-out src/server.cpp, $(SRC_ALL))

OBJECTS	 := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
		 := $(OBJECTS:.o=.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all build clean debug release info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

configure:
	@mkdir -p $(LIB_DIR)
	git clone https://github.com/yhirose/cpp-httplib ./lib/cpp-httplib

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

info:
	@echo "[*] Application dir: ${APP_DIR}	   "
	@echo "[*] Object dir:		${OBJ_DIR}	   "
	@echo "[*] Sources:			${SRC}		   "
	@echo "[*] Objects:			${OBJECTS}	   "
	@echo "[*] Dependencies:	${DEPENDENCIES}"

test: ./test/test.cpp ./test/performance.hpp ./test/current_accuracy.hpp ./src/sipm.cpp ./src/utilities.cpp
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/test ./test/test.cpp ./src/sipm.cpp ./src/utilities.cpp

server: ./src/server.cpp ./src/sipm.cpp ./src/utilities.cpp
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET_SERVER) ./src/server.cpp ./src/sipm.cpp ./src/utilities.cpp

simspad: ./src/main.cpp ./src/sipm.cpp ./src/utilities.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) ./src/main.cpp ./src/sipm.cpp ./src/utilities.cpp
