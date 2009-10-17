
all: sample-1 sample-2 sample-3 sample-4 sample-5 sample-6

sample-%: sample-%.cpp peg.hpp
	g++ -o $@ $< -I/home/kik/include


