CC = clang++
CFLAGS = -std=c++23 -O2 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti -flto

all: chess chess_perft

chess: src/main.cpp
	$(CC) $(CFLAGS) -o bin/chess src/main.cpp

chess_perft: src/chess_perft.cpp
	$(CC) $(CFLAGS) -o bin/chess_perft src/chess_perft.cpp

clean:
	rm -f bin/*
