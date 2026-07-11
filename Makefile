CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
TARGET   := main
SRCS :=  src/tiny_ppl_core.cpp src/parser.cpp src/primitives.cpp src/inference.cpp  src/machine.cpp src/main.cpp
.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
