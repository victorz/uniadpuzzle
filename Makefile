CFLAGS = -Wall

decompressor_broken_fixed:
	g++ $(CFLAGS) $@.cpp -o $@
