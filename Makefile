CXX ?= g++

CXXFLAGS := -std=c++17 -O2 -g -Wall -Wno-switch -Werror -I . -I include

SRCS := src/format.cc src/os.cc anim.cpp animdata.cpp export.cpp lodepng.cpp sheet.cpp utils.cpp converter/gba.cpp pwaanim.cpp

HEADERS := pwaanim.hpp anim.hpp animdata.hpp export.hpp lodepng.h sheet.hpp converter/gba.hpp

.PHONY: all clean

all: pwaanim
	@:

pwaanim: $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) pwaanim pwaanim.exe
