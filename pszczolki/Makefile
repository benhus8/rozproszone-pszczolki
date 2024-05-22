SOURCES=$(wildcard *.c)
HEADERS=$(SOURCES:.c=.h)

NUM_BEES = 3
NUM_REEDS = 2
NUM_FLOWERS = 2

FLAGS=-DDEBUG -g
# FLAGS=-g

all: main tags

main: $(SOURCES) $(HEADERS) Makefile
	mpicc -Wall $(SOURCES) $(FLAGS) -DNUM_BEES=${NUM_BEES} -DNUM_REEDS=$(NUM_REEDS) -DNUM_FLOWERS=$(NUM_FLOWERS) -o main

clear: clean

clean:
	rm main a.out

tags: ${SOURCES} ${HEADERS}
	ctags -R .

run: main Makefile tags
	mpirun -np $(NUM_BEES) ./main