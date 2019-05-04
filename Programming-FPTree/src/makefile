CXX = g++

HEADER_DIR = ../include

FPTREE_HEADER_DIR = $(HEADER_DIR)/fptree

CXXFLAGS = -std=c++11

CFLAGS = -fPIC -std=c99 -O3 -msse4.2 -mpclmul -march=native -funroll-loops -Wstrict-overflow -Wstrict-aliasing -Wall -Wextra -pedantic -Wshadow

MAIN = ./bin/main

YCSB = ./bin/ycsb

LYCSB = ./bin/lycsb

BIN = bin

FPTREE = fptree.o

UTILITY = utility.o

PALLOCATOR = pallocator.o

CLHASH = clhash.o

ALL := $(LYCSB) $(YCSB) $(MAIN) $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)

#============================TODO: all below============================

$(MAIN) :main.cpp $(HEADER_DIR)/fptree/fptree.h $(FPTREE) $(PALLOCATOR)
	$(CXX) $(CXXFLAGS) -c main.cpp -I $(HEADER_DIR) -g -o main.o
	$(CXX) $(CXXFLAGS) -o $@ main.o $(FPTREE) $(PALLOCATOR)

$(FPTREE) : fptree.cpp $(HEADER_DIR)/fptree/fptree.h 
	$(CXX) $(CXXFLAGS) -c fptree.cpp -I $(HEADER_DIR) -g -o $@

$(UTILITY) : utility.cpp $(HEADER_DIR)/utility/utility.h
	$(CXX) $(CXXFLAGS) -c fptree.cpp -I $(HEADER_DIR) -g -o $@

$(PALLOCATOR) : p_allocator.cpp $(HEADER_DIR)/utility/p_allocator.h
	$(CXX) $(CXXFLAGS) -c p_allocator.cpp -I $(HEADER_DIR) -g -o $@ -lpmem

$(CLHASH) : clhash.c $(HEADER_DIR)/utility/clhash.h
	$(CC) $(CFLAGS) -c clhash.c -I $(HEADER_DIR) -g -o $@

$(LYCSB) : lycsb.cpp 
	$(CXX) $(CXXFLAGS) -c lycsb.cpp -g -o lycsb.o 
	$(CXX) $(CXXFLAGS) lycsb.o -g -o $@ -pthread -lleveldb

$(YCSB) : ycsb.cpp $(HEADER_DIR)/fptree/fptree.h $(FPTREE) $(PALLOCATOR)
	$(CXX) $(CXXFLAGS) -c ycsb.cpp -I $(HEADER_DIR) -g -o ycsb.o
	$(CXX) $(CXXFLAGS) -o $@ ycsb.o $(FPTREE) $(PALLOCATOR) -pthread -lleveldb

$(BIN):
	mkdir -p bin

all : $(ALL)

clean :
	rm -rf *.o ./bin/* ../data/*

cleand : 
	rm -rf ../data/*