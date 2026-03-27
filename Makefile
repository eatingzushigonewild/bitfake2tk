 CXX := g++
 CXXFLAGS := -std=c++17 -Wall -Wextra -O2
 TARGET := bitftk
 SRC := test.cpp
 CLANG_FORMAT := clang-format
 FORMAT_FILES := $(shell find . -type f \( -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" -o -name "*.h" -o -name "*.hh" -o -name "*.hpp" -o -name "*.hxx" \) -not -path "./.git/*")

 .PHONY: all run clean format format-check

 all: $(TARGET)

 $(TARGET): $(SRC) bitfake2.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

 run: $(TARGET)
	./$(TARGET)

 format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "Error: $(CLANG_FORMAT) is not installed."; echo "Install it, then run 'make format' again."; exit 1; }
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

 format-check:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "Error: $(CLANG_FORMAT) is not installed."; echo "Install it, then run 'make format-check' again."; exit 1; }
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

 clean:
	rm -f $(TARGET)
