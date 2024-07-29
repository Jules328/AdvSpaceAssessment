CC       := gcc
CPPFLAGS := 
CFLAGS   := -Wall -g -O0
LDLIBS   := 

EXE := fsw

.PHONY: all clean
all: $(EXE)

$(EXE): src/fsw.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -rf $(EXE)