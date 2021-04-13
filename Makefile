OBJ = main.o functions.o
all: cp_daemon

cp_daemon: $(OBJ)
	gcc $(OBJ) -o cp_daemon

$(OBJ): functions.h

.PHONY: clean
clean:
	rm -f *.o cp_daemon