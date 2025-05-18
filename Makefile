CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -lncurses

SRC = src/main.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = gitNCurses

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean 