# Pruebas en Linux/macOS (no necesitan devkitPro): make -C tests run
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined
SRC := ../src

SOURCES := test_main.cpp \
           $(SRC)/ui/settings.cpp \
           $(SRC)/ui/game_library.cpp \
           $(SRC)/ui/fs_util.cpp \
           $(SRC)/ui/menu.cpp \
           $(SRC)/core/stub_core.cpp

test_runner: $(SOURCES) $(wildcard $(SRC)/*/*.h)
	$(CXX) $(CXXFLAGS) -I$(SRC) $(SOURCES) -o $@

run: test_runner
	./test_runner

clean:
	rm -f test_runner

.PHONY: run clean
