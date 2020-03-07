all : hidtest hidtest.exe

hidtest.exe : hidtest.c hidapi.c os_generic.c CNFGFunctions.c CNFGWinDriver.c
	i686-w64-mingw32-gcc -o $@ $^ $(CFLAGS) -lsetupapi -lgdi32

hidtest : hidtest.o hidapi.o os_generic.o CNFGFunctions.o CNFGXDriver.o
	g++ -o $@ $^ -lusb-1.0 -ludev -lpthread -lX11

clean :
	rm -rf *.o *~ hidtest
