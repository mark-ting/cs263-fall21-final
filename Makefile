all: broker publisher subscriber

# C++-compiler settings
CC = g++ -ggdb3

# Optimization level
O = 0

# Create object files
%.o: %.cpp
	$(CC) -O$(O) -o $@ -c $<

# Create executable
broker: broker.o pubsub.o
	$(CC) -o $@ $^

publisher: publisher.o pubsub.o
	$(CC) -o $@ $^

subscriber: subscriber.o pubsub.o
	$(CC) -o $@ $^
