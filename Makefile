CC = g++
C17FLAGS = -Wall -Wextra -Werror -Wpedantic -std=c++17
C14FLAGS = -Wall -Wextra -Werror -Wpedantic -std=c++14
C11FLAGS = -Wall -Wextra -Werror -Wpedantic -std=c++11
ALL = simpletest

all: $(ALL)

simpletest : src/SimpleTestCases.cpp Makefile
	$(CC) $(C11FLAGS) -o st1.exe src/SimpleTestCases.cpp

clean:
	$(RM) $(ALL) *.o

test: all
	./st1.exe
