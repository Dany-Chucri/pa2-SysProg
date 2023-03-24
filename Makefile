mysh: mysh.c
	gcc -g -Wall -Werror -fsanitize=address -o mysh mysh.c

clean:
	rm -rf mysh
