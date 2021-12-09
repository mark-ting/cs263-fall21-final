all: broker publisher

# C++-compiler settings
CC = g++ -ggdb3

# Optimization level
O = 0

# Create object files
%.o: %.cpp
	$(CC) -O$(O) -o $@ -c $<

# Create executable
broker: broker.o
	$(CC) -o $@ $^

publisher: publisher.o
	$(CC) -o $@ $^
