 CXX := g++
 CXXFLAGS := -std=c++17 -Wall -Wextra -O2
 TARGET := bitftk
 SRC := test.cpp

 .PHONY: all run clean

 all: $(TARGET)

 $(TARGET): $(SRC) bitfake2.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

 run: $(TARGET)
	./$(TARGET)

 clean:
	rm -f $(TARGET)
