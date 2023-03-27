CXX := g++
CXXFLAGS := -O2 -march=native -fopenmp
all: iter

rec: mcsp_rec.cpp graph.cpp graph.h
	$(CXX) $(CXXFLAGS) -Wall -std=c++2a -o recur graph.cpp mcsp_rec.cpp test_utility.cpp -pthread

iter: mcsp_iter.cpp graph.cpp graph.h
	$(CXX) $(CXXFLAGS) -Wall -std=c++2a -o iter graph.cpp mcsp_iter.cpp test_utility.cpp -pthread
	
clean:
	rm -f iter
	rm -f recur
