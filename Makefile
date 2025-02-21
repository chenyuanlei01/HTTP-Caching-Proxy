# Compiler settings
CXX = g++
CXXFLAGS = -Wall -g

# Target and source files
TARGET = proxy
SRCS = main.cpp socket.cpp handler.cpp cache.cpp log.cpp request.cpp response.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Compilation rule
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# Generate object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# Declare phony targets
.PHONY: all run clean