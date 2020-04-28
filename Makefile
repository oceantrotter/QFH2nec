all: helix2nec QFH2nec

helix2nec:
	gcc -o helix2nec helix2nec.c -lm -W -Wall


QFH2nec:
	gcc -o QFH2nec QFH2nec.c -lm -W -Wall

	
clean:
	rm -rf *.o
	rm -rf helix2nec
	rm -rf QFH2nec

test: clean all
	./QFH2nec 137.5 0.5 1 15 5 0.3
	xnec2c QFH\ 137.5_0.5_0.30_1.0_15.0_5.0.nec
