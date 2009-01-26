CFLAGS = -Wall

decompressor:
	g++ $(CFLAGS) $@.cpp -o $@
