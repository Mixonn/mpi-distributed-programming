CXX := mpic++
CXXFLAGS := -Wall -std=c++11 -g
LDFLAGS := -pthread

SRC_DIR = src
BIN_DIR = bin

SRC := $(wildcard ${SRC_DIR}/*.cpp)
OBJ  := $(SRC:.cpp=.o)
EXEC = ${BIN_DIR}/pyrkon


all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(CXXFLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -rf ${OBJ} ${EXEC}
