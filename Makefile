CXX = g++
CXXFLAGS = -Wall -std=c++17
LDFLAGS = -lncurses

SRC_DIR = src
BUILD_DIR = build
LOG_DIR = logs

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/gitncurses

.PHONY: all clean build

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LOG_DIR):
	mkdir -p $(LOG_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

build: $(LOG_DIR)
	@echo "Building project..."
	@make all 2>&1 | tee $(LOG_DIR)/build_$(shell date +%Y%m%d_%H%M%S).log

clean:
	rm -rf $(BUILD_DIR) $(LOG_DIR) 