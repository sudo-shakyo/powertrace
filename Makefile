CC=gcc
CFLAGS=-Wall -pthread -O2
OBJS=main.o collector.o sampler.o correlator.o reporter.o
TARGET=powertrace

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET) powertrace_log.csv
