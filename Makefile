CC = g++
# CFLAGS = -D_FILE_OFFSET_BITS=64 -O3 -std=c++11
CFLAGS = -std=c++17
LIBS = -lfuse3 -ljsoncpp -lcurl 
REST_FS_SRCS=$(shell ls *.cc | grep -v _test.cc)
PATH_TEST_SRCS=$(shell ls *.cc | grep -v main.cc)

restfs:
	$(CC) $(REST_FS_SRCS) -o $@ $(CFLAGS) $(LIBS) -I ./ 


path_test:
	$(CC) $(PATH_TEST_SRCS) -o $@ $(CFLAGS) $(LIBS) -I ./ 
