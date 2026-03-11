MPICXX = mpic++
CXXFLAGS = -Wall

SRC = main.cpp custom_collectives.cpp
TEST_SRC = test.cpp custom_collectives.cpp
HDR = custom_collectives.h
EXEC = primitives
TEST_EXEC = test_runner

NP ?= 8
TEST_ARGS ?= -g 50000000 0

.PHONY: all autograder test clean

all: autograder

autograder: $(EXEC)

$(EXEC): $(SRC) $(HDR)
	$(MPICXX) $(CXXFLAGS) -o $@ $(SRC)

$(TEST_EXEC): $(TEST_SRC) $(HDR)
	$(MPICXX) $(CXXFLAGS) -o $@ $(TEST_SRC)

test: $(TEST_EXEC)
	mpirun -n $(NP) ./$(TEST_EXEC) $(TEST_ARGS)

clean:
	rm -f $(EXEC) $(TEST_EXEC)