
all: sample-1 sample-2 sample-3

sample-1: sample-1.cpp peg.hpp
	g++ -o $@ $< -I/home/kik/include

sample-2: sample-2.cpp peg.hpp
	g++ -o $@ $< -I/home/kik/include

sample-3: sample-3.cpp peg.hpp
	g++ -o $@ $< -I/home/kik/include

