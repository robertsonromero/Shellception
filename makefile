CC=gcc
CFLAGS=-g -c -Wall
SOURCES=minishell.c 
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=minishell

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

$(OBJECTS): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

clean: 
	rm -rf $(OBJECTS) $(EXECUTABLE)
