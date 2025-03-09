# I'm still learning Makefile

executable = bmp

$(executable): bmp.c bmpLib.c bmpLib.h
	gcc bmp.c bmpLib.c -o $(executable) -lm -Wall

clean:
	rm -f $(executable)