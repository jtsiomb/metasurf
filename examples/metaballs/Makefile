src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = metaballs

CC = gcc
CFLAGS = -pedantic -Wall -g -I../../src
LDFLAGS = -L../.. -lmetasurf $(libgl)

ifeq ($(shell uname -s), Darwin)
	libgl = -framework OpenGL -framework GLUT -lGLEW
else
	libgl = -lGL -lGLU -lglut -lGLEW
endif

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)


.PHONY: cleandep
cleandep:
	rm -f $(dep)
