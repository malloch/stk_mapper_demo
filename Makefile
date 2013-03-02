CC = g++
CXXFLAGS = -D__MACOSX_CORE__ -DRAWWAVE_PATH=\"../stk-4.4.4/rawwaves\" \
	-I../stk-4.4.4/include/ $(shell pkg-config --cflags libmapper-0)
LDFLAGS = -L../stk-4.4.4/src -lstk $(shell pkg-config --libs libmapper-0) -lpthread -framework CoreAudio -framework CoreFoundation

stk_mapper_demo: stk_mapper_demo.o

clean:
	$(RM) -f *.o