CFITSIO = $(FITSIOROOT)
CPP = g++
CC = gcc
CFLAGS = -Wall -I$(CFITSIO) 
LIBS = -L$(CFITSIO) -lcfitsio -lm
GLIBS = 
GLIBS += 
OBJECTS = writeDamicHeader.o 
HEADERS = 

ALL : writeDamicHeader.exe
	echo "Listo!"

writeDamicHeader.exe : $(OBJECTS)
	$(CPP) $(OBJECTS) -o writeDamicHeader.exe $(LIBS) $(GLIBS) $(CFLAGS)

writeDamicHeader.o : writeDamicHeader.cc $(HEADERS)
	$(CPP) -c writeDamicHeader.cc -o writeDamicHeader.o $(CFLAGS)

clean:
	rm -f *~ *.o *.exe
