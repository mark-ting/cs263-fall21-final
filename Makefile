all: pubsub

# C++-compiler settings
CC = g++

# Optimization level
O = 0

# Create object files
%.o: %.cpp
	$(CC) -O$(O) -o $@ -c $<

# Create executable
pubsub: pubsub.o
	$(CC) -o $@ $^
