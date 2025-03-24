# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -lcurl

# Executable name
TARGET = downloader

# Source file
SRC = HW01.c

# Default rule: Compile the program
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Run the program with a test URL and 4 threads
run:
	./$(TARGET) "https://sample-videos.com/video321/mp4/720/big_buck_bunny_720p_30mb.mp4" 4

# Clean up the compiled binary
clean:
	rm -f $(TARGET)
