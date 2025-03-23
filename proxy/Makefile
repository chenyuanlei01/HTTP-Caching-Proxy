# Compiler settings
CXX = g++
CXXFLAGS = -Wall -g -std=c++17 -pthread

# Target and source files
TARGET = proxy
SRCS = main.cpp socket.cpp handler.cpp cache.cpp log.cpp request.cpp response.cpp
OBJS = $(SRCS:.cpp=.o)

# Include directories
INCLUDES = -I/usr/include/boost

# Libraries
LIBS = -lboost_system -lboost_thread -pthread

# Default target
all: $(TARGET)

# Compilation rule
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Generate object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the program
run: $(TARGET)
	sudo mkdir -p /var/log/erss
	sudo chmod 777 /var/log/erss
	./$(TARGET)

# Clean compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# Declare phony targets
.PHONY: all run clean # tests