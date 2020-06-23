CC = g++
CXXFLAGS = -D__MACOSX_CORE__ -DRAWWAVE_PATH=\"../stk/rawwaves\" \
	-I../stk/include/ $(shell pkg-config --cflags libmpr)
LDFLAGS = -L../stk/src -lstk $(shell pkg-config --libs libmpr) -lpthread -framework CoreAudio -framework CoreFoundation

stk_mapper_demo: stk_mapper_demo.o

clean:
	$(RM) -f *.o