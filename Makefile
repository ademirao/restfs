CC = g++
# CFLAGS = -D_FILE_OFFSET_BITS=64 -O3 -std=c++11
CFLAGS = -D_FILE_OFFSET_BITS=64 -g3 -std=c++17
LIBS = -lfuse3 -ljsoncpp -lcurl 

restfs:
	$(CC) *.cc -o $@ $(CFLAGS) $(LIBS) -I ./ 
