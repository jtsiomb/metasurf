src = $(wildcard *.c)
obj = $(src:.c=.o)
bin = simple

CC = gcc
CFLAGS = -pedantic -Wall -g -I../../src
LDFLAGS = -L../.. -lmetasurf $(libgl) -lm

ifeq ($(shell uname -s), Darwin)
	libgl = -framework OpenGL -framework GLUT
else
	libgl = -lGL -lGLU -lglut
endif

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
