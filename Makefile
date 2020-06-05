cgdb: linenoise.c debugger.c
	$(CC) -Wall -W -Os -g -o cgdb linenoise.c debugger.c

clean:
	rm -f cgdb
