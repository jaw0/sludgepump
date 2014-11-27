
CCC = g++

# be sure to -I and -L the location of inn includes/libs
# NB: some makes use CCFLAGS instead of CFLAGS

CFLAGS = -O2 -g -I. -I../build-netbsd/include -DRCSID
LDFLAGS = -L../build-netbsd/lib -linn


OBJS = article.o buffer.o channel.o main.o misc.o nio.o nntp.o nntpihave.o nntpstream.o \
	slopbucket.o sludgepump.o stats.o version.o

sp: $(OBJS)
	$(CCC) $(CCFLAGS) -o sp $(OBJS) $(LDFLAGS)


clean:
	-rm -f sp $(OBJS)

