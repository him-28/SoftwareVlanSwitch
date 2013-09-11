all: slicz slijent

slicz: ./slich_src/*.h ./slich_src/*.c
	gcc -std=gnu99 -pthread ./slich_src/*.h ./slich_src/*.c -o slicz

slijent: ./slijent_src/main.c
	gcc -std=gnu99 -pthread ./slijent_src/main.c -o slijent	
clean:
	rm -f slicz slijent *~ slich_src/*.o slich_src/*~ slijent_src/*.o slijent_src/*~


