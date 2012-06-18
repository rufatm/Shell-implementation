all: 
	gcc -o myshell myshell.c -lreadline -lncurses 
	
clean:
	rm -f myshell
