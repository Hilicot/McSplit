CXX := g++
CXXFLAGS := -O2 -march=native -fopenmp
all: dal

rec: mcsp_rec.cpp graph.cpp graph.h
	$(CXX) $(CXXFLAGS) -Wall -std=c++2a -o recur graph.cpp mcsp_rec.cpp test_utility.cpp -pthread

iter: mcsp_iter.cpp graph.cpp graph.h
	$(CXX) $(CXXFLAGS) -Wall -std=c++2a -o iter graph.cpp mcsp_iter.cpp test_utility.cpp -pthread
	
dal: mcsplit+DAL.cpp graph.cpp graph.h mcs.h mcs.cpp stats.h args.h test_utility.cpp
	$(CXX) $(CXXFLAGS) -Wall -std=c++2a -o mcsplit-dal mcsplit+DAL.cpp graph.cpp mcs.h mcs.cpp test_utility.cpp -pthread

clean:
	rm -f iter
	rm -f recur
