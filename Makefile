CC = g++
FLAGS = -O2 -std=c++0x 
LDFLAGS = -O2 -std=c++0x 
LIBS = -L/usr/lib -L/usr/local/lib -m64 -lpthread
INCS = -I./

TARGET = CL-threadpool
TOCOMPILE = main.o

all: ${TOCOMPILE}
	${CC} $(LDFLAGS) -o $(TARGET) ${TOCOMPILE} ${LIBS}

.cpp.o:
	$(CC) ${FLAGS} ${INCS} -c $*.cpp

clean:
	rm -f *.o $(TARGET)
	
