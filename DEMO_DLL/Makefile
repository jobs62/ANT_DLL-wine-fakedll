SRC= demo.c libant.c
OBJ= $(SRC:.c=.o)


CC=x86_64-w64-mingw32-gcc
CPPFLAGS=-I../ANT_LIB/inc


all: demo

%.o: %.c
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

demo: $(OBJ)
	$(CC) ${LDFLAGS} $^ -o $@

clean:
	rm -f ${OBJ}
