all: helix2nec QFH2nec

helix2nec:
	gcc -o helix2nec helix2nec.c -lm -W -Wall


QFH2NEC:
	gcc -o QFH2nec QFH2nec.c -lm -W -Wall

	
clean:
	rm -rf *.o
	rm -rf helix2nec
	rm -rf QFH2nec
