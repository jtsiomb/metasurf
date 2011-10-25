src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = volume

# uncomment the following line to disable shaders (for old systems like SGI or whatever)
#nosdr = -DNO_SHADERS

CC = gcc
CFLAGS = -pedantic -Wall -g -I../../src $(nosdr)
LDFLAGS = -L../.. -lmetasurf $(libgl) -limago

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