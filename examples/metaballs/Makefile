src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = metaballs

# uncomment the following line to disable shaders (for old systems like SGI or whatever)
#nosdr = -DNO_SHADERS

CC = gcc
CFLAGS = -pedantic -Wall -g -I../../src $(inc) $(nosdr)
LDFLAGS = -L../.. -lmetasurf $(libgl) -lm

ifeq ($(shell uname -s), Darwin)
	inc = -I/opt/local/include
	libgl = -L/opt/local/lib -framework OpenGL -framework GLUT -lGLEW
else
	libgl = -L/usr/local/lib -lGL -lGLU -lglut -lGLEW
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
