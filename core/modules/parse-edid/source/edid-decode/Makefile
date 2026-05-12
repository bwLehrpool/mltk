CXX = g++
CXXFLAGS = -O2 -g -Wall -Wextra -std=c++11
VERSION = %VERSION%
GIT_COMMIT_CNT := $(shell git rev-list --count HEAD 2>/dev/null || echo 0)
GIT_SHA := $(shell git rev-parse HEAD 2>/dev/null || echo unknown)
GIT_COMMIT_DATE := $(shell git log -1 --format=%cd --date=format:'%Y-%m-%d %H:%M:%S' 2>/dev/null || echo unknown)

CXXFLAGS += -DPACKAGE_VERSION="\"$(VERSION)\""
CXXFLAGS += -DGIT_COMMIT_CNT=$(GIT_COMMIT_CNT)
CXXFLAGS += -DGIT_SHA="\"$(GIT_SHA)\""
CXXFLAGS += -DGIT_COMMIT_DATE="\"$(GIT_COMMIT_DATE)\""

SOURCES = $(wildcard *.cpp)

# Check for linux/i2c-dev.h
HAS_I2C := $(shell test -f /usr/include/linux/i2c-dev.h && echo yes)

ifeq ($(HAS_I2C),yes)
	SOURCES += ddc.cpp
	CXXFLAGS += -D__HAS_I2C_DEV__
endif

OBJECTS = $(SOURCES:.cpp=.o)
TARGET = edid-decode

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp edid-decode.h oui.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
