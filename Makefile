# This is just a convenience Makefile to avoid having to remember
# all the CMake commands and their arguments.

# Set CMAKE_GENERATOR in the environment to select how you build, e.g.:
#   CMAKE_GENERATOR=Ninja

CLANG_FORMAT=clang-format -i

.PHONY: all test clean cclean format

all: ${BUILD_DIR}
	cmake -B build -DCMAKE_BUILD_TYPE=Release .
	cmake --build build --parallel 8

test: ${BUILD_DIR} test/*
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTESTING=ON .
	cmake --build build --parallel 8

clean:
	cmake --build build --target clean

cclean:
	rm -rf build

format:
	find include -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}
	find src -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}
	find cmd -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}
	find test -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}

