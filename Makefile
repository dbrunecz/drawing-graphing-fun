
#apt-get install libx11-dev

CFLAGS+= -Wall -Werror
CFLAGS+= -ggdb
#CFLAGS+= -O3
#CFLAGS+= -ansi -pedantic
LDFLAGS+= -L/usr/X11R6/lib
LDLIBS+= -lX11 -lm

3d3: CFLAGS+=-O3
3d3: LDLIBS+=-lOpenCL
3d3: dbcl.o dbx.o 3d3.o loadfile.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

pong: dbx.o pong.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

graph: dbx.o graph.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

hellox6: dbx.o hellox6.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

hellox5: dbx.o hellox5.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

3d: dbx.o 3d.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

3d2: dbx.o 3d2.o
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	@-rm -f pong hellox5 hellox6 graph 3d 3d2 3d3 *.o 2&>/dev/null
