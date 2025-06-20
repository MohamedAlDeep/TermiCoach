CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS = -lcurl

termicoach: main.cpp
    $(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
    rm -f termicoach

.PHONY: clean