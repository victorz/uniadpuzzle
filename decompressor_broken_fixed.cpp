/*******************************************************************************

Start here:

Introduction
------------

JPEG uses a lossy compression scheme that looks loosely like this:
1. Divide image into blocks (8x8 pixels in this case)
2. Apply a discrete cosine transform on each block. This will transform the
   block into a frequency-domain representation, We have still not done any
   compression.
3. Reshuffle the coefficients according to a zig-zag pattern. This will merely
   move all low-frequency components together.
4. Quantize the coefficents. This means scale down the coefficients and round
   them to integers. This is where we lose information, alas, lossy compression.
   The idea is that frequency components with low amplitude can be removed
   without destroying the visual impression of the picture.
5. Come up with an efficient system of storing the coefficients, and keep in
   mind that many may be zeros. 

Suggested reading on wikipedia and other sources: DCT, JPEG (especially 
sections Block splitting, Discrete cosine transform, Quantization, Entropy 
coding.

Again, the actual compression is done in step 4. This is also where the 
compressor has to make good decisions. Quantify much, and we will lose a lot
of information, or quantify only a little, and the compressed image becomes
(too) large. For the decompressor, life is easier though. All that the 
decompressor needs to do is to read the level of quantization per block and 
scale up the matrix.

An implementation of "VPEG"
---------------------------

Below is an attempt to decompress an gray-scale image, that is, each pixel 
has a single value 0-255.

Instead of JPEG (but with inspiration it), we use a very simple coding schema 
with seven different patterns. The DC coefficient ([0][0] in the matrix) is 
stored separately from the other coefficients. The other coefficients are then 
stored in a run-length encoding schema, where consecutive zeros are represented 
with a single number.

Below are the seven different patterns. (We use the non-standard notation 0b 
to denote a binary value. E.g., 0b01011010 is the same as 0x5a which is the
same as 90.)

0b101000yy 0bxxxxxxxx 0bxxxxxxxxx: start of block, quantifization value is yy, 
                                   DC value is xxxxxxxxxxxxxxxx (big-endian)
0b0sxxxxxx:                        coefficient value sxxxxxx (1-complement sign 
                                   s=0 means 1, s=1 means -1)
0b100zzzzs 0bxxxxxxxx:             zzzzz zeros, followed by coefficient value 
                                   sxxxxxxxx
0b10101100:                        end of block
0b10101110:                        next row of blocks
0b10101111:                        end of file
0b11111111 0bxxxxxxxx:             ignore next xxxxxxxx bytes

The reader should again note that this schema is not at all the same as the 
Huffman encoding in the JPEG format. Only the ideas and algorithms are the same.

Problem
-------

However, there are some bugs in the decompression code below :( 
One tiny function is even missing!

To participate in the drawing of great prices, write down the information 
(a timestamp and a symbol) found in the image together with your name and 
e-mail and give it to the people in the Vizrt/Ardendo booth at Uniaden. Also, be 
at the booth on Tuesday at the given time to participate in the drawing.

Even if you do not solve the puzzle, we still want to talk to you. We do not 
only search people who eats bits and bytes for breakfast, but all kind of 
talented persons!

Hints
-----

- Read the compiler warnings/errors.
- Read each line carefully.
- Make sure you and your debugger are a team.
- There are no known bugs in the data, quant_values, or zigzag_order constants.

*******************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

unsigned char data[] = 
    {
        0xa0, 0x07, 0xf8, 0x9e, 0x00, 0x9e, 0x00, 0x9e, 
        0x00, 0xac, 0xa3, 0x01, 0xe0, 0x11, 0x1c, 0x02, 
        0x50, 0x58, 0x4b, 0x42, 0x0e, 0x12, 0x07, 0x0a, 
        0x02, 0x4a, 0x4c, 0x41, 0x46, 0x48, 0x42, 0x06, 
        0x03, 0x82, 0x01, 0x05, 0x06, 0x02, 0x41, 0x41, 
        0x87, 0x03, 0x44, 0x8c, 0x01, 0x01, 0x9e, 0x00, 
        0xac, 0xa2, 0x03, 0xfb, 0x9e, 0x00, 0x9e, 0x00, 
        0x9e, 0x00, 0xac, 0xa3, 0x01, 0xb9, 0x0c, 0x80, 
        0x40, 0x05, 0x4b, 0x76, 0x06, 0x45, 0x09, 0x27, 
        0x02, 0x46, 0x04, 0x45, 0x58, 0x41, 0x42, 0x04, 
        0x42, 0x02, 0x06, 0x82, 0x01, 0x01, 0x42, 0x01, 
        0x83, 0x02, 0x87, 0x01, 0x9e, 0x00, 0x9e, 0x00, 
        0xac, 0xa3, 0x01, 0xee, 0x4e, 0x0e, 0x4b, 0x0d, 
        0x4c, 0x47, 0x0a, 0x4b, 0x09, 0x44, 0x07, 0x49, 
        0x08, 0x46, 0x82, 0x04, 0x45, 0x06, 0x45, 0x01, 
        0x85, 0x03, 0x04, 0x44, 0x01, 0x88, 0x01, 0x42, 
        0x01, 0x9e, 0x00, 0xac, 0xa0, 0x07, 0xf8, 0x9e, 
        0x00, 0x9e, 0x00, 0x9e, 0x00, 0xac, 0xff, 0x0e,
        0x49, 0x73, 0x61, 0x6b, 0x20, 0x77, 0x61, 0x73,
        0x20, 0x68, 0x65, 0x72, 0x65, 0x2e, 0xa0, 0x07, 
        0xf8, 0x9e, 0x00, 0x9e, 0x00, 0x9e, 0x00, 0xac, 
        0xa3, 0x01, 0xfa, 0x03, 0x03, 0x41, 0x43, 0x43, 
        0x82, 0x01, 0x02, 0x03, 0x85, 0x01, 0x42, 0x42, 
        0x86, 0x01, 0x02, 0x01, 0x89, 0x01, 0x9e, 0x00, 
        0x9e, 0x00, 0xac, 0xa3, 0x01, 0xf8, 0x44, 0x05, 
        0x42, 0x04, 0x45, 0x82, 0x02, 0x44, 0x04, 0x85, 
        0x02, 0x03, 0x43, 0x86, 0x02, 0x43, 0x01, 0x89, 
        0x02, 0x01, 0x41, 0x9e, 0x00, 0x9e, 0x00, 0xac, 
        0xae, 0xa3, 0x01, 0xc0, 0x2f, 0x2a, 0x52, 0x5e, 
        0x49, 0x43, 0x06, 0x03, 0x44, 0x07, 0x09, 0x09, 
        0x06, 0x05, 0x41, 0x4b, 0x4f, 0x49, 0x44, 0x41, 
        0x82, 0x02, 0x0c, 0x09, 0x03, 0x01, 0x87, 0x02, 
        0x44, 0x8f, 0x01, 0x8a, 0x01, 0x01, 0x87, 0x01, 
        0xac, 0xa3, 0x01, 0x1f, 0x32, 0x77, 0x63, 0x48, 
        0x0f, 0x07, 0x2b, 0x18, 0x03, 0x12, 0x47, 0x24, 
        0x0b, 0x44, 0x49, 0x0e, 0x03, 0x41, 0x41, 0x03, 
        0x05, 0x43, 0x48, 0x4c, 0x4f, 0x82, 0x03, 0x44, 
        0x82, 0x01, 0x4a, 0x49, 0x42, 0x01, 0x83, 0x01, 
        0x41, 0x83, 0x02, 0x41, 0x82, 0x01, 0x84, 0x02, 
        0x83, 0x01, 0x84, 0x01, 0x86, 0x01, 0xac, 0xa3, 
        0x01, 0xb8, 0x2a, 0x4c, 0x70, 0x17, 0x14, 0x0d, 
        0x49, 0x50, 0x04, 0x4f, 0x0f, 0x0a, 0x46, 0x41, 
        0x42, 0x42, 0x42, 0x03, 0x41, 0x02, 0x82, 0x03, 
        0x41, 0x43, 0x83, 0x01, 0x01, 0x42, 0x82, 0x02, 
        0x83, 0x01, 0x01, 0x87, 0x01, 0x86, 0x01, 0x9e, 
        0x00, 0xac, 0xa3, 0x01, 0xad, 0x55, 0x7c, 0x51, 
        0x55, 0x71, 0x45, 0x43, 0x42, 0x5b, 0x4d, 0x45, 
        0x83, 0x04, 0x56, 0x83, 0x04, 0x05, 0x08, 0x03, 
        0x44, 0x42, 0x84, 0x01, 0x02, 0x83, 0x02, 0x84, 
        0x02, 0x03, 0x03, 0x02, 0x02, 0x86, 0x01, 0x8a, 
        0x01, 0x82, 0x01, 0x9e, 0x00, 0xac, 0xa3, 0x01, 
        0x60, 0x81, 0x5e, 0x0c, 0x62, 0x47, 0x02, 0x28, 
        0x45, 0x49, 0x42, 0x20, 0x52, 0x51, 0x0d, 0x03, 
        0x0a, 0x43, 0x56, 0x01, 0x46, 0x43, 0x85, 0x07, 
        0x0a, 0x03, 0x41, 0x84, 0x02, 0x83, 0x03, 0x44, 
        0x01, 0x02, 0x02, 0x82, 0x02, 0x84, 0x02, 0x41, 
        0x85, 0x02, 0x41, 0x8d, 0x01, 0x41, 0xac, 0xa3, 
        0x01, 0x71, 0x19, 0x21, 0x30, 0x06, 0x0e, 0x81, 
        0x4f, 0x44, 0x01, 0x45, 0x43, 0x0f, 0x44, 0x51, 
        0x10, 0x08, 0x09, 0x07, 0x4a, 0x05, 0x02, 0x45, 
        0x82, 0x04, 0x82, 0x01, 0x41, 0x87, 0x01, 0x49, 
        0x08, 0x41, 0x41, 0x02, 0x85, 0x01, 0x02, 0x01, 
        0x41, 0x89, 0x01, 0x9e, 0x00, 0xac, 0xa3, 0x01, 
        0x95, 0x4e, 0x0b, 0x80, 0x54, 0x05, 0x0c, 0x1e, 
        0x48, 0x01, 0x0b, 0x69, 0x4c, 0x4a, 0x82, 0x07, 
        0x4b, 0x03, 0x43, 0x49, 0x82, 0x02, 0x05, 0x04, 
        0x05, 0x83, 0x05, 0x84, 0x05, 0x82, 0x01, 0x04, 
        0x83, 0x01, 0x85, 0x02, 0x84, 0x03, 0x9e, 0x00, 
        0xac, 0xa3, 0x01, 0x54, 0x42, 0x5b, 0x30, 0x2b, 
        0x48, 0x35, 0x49, 0x1c, 0x12, 0x0d, 0x56, 0x4b, 
        0x04, 0x10, 0x46, 0x49, 0x4f, 0x4d, 0x47, 0x07, 
        0x45, 0x02, 0x44, 0x44, 0x46, 0x45, 0x02, 0x42, 
        0x02, 0x02, 0x02, 0x03, 0x41, 0x42, 0x01, 0x82, 
        0x01, 0x01, 0x03, 0x03, 0x83, 0x01, 0x86, 0x01, 
        0x02, 0x8a, 0x01, 0xac, 0xff, 0x5e, 0x43, 0x6f, 
        0x6e, 0x67, 0x72, 0x61, 0x74, 0x75, 0x6c, 0x61, 
        0x74, 0x69, 0x6f, 0x6e, 0x73, 0x20, 0x66, 0x72, 
        0x6f, 0x6d, 0x20, 0x56, 0x69, 0x7a, 0x72, 0x74, 
        0x21, 0x20, 0x46, 0x6f, 0x72, 0x20, 0x74, 0x68, 
        0x65, 0x20, 0x63, 0x68, 0x61, 0x6e, 0x63, 0x65, 
        0x20, 0x74, 0x6f, 0x20, 0x61, 0x6e, 0x20, 0x65, 
        0x78, 0x74, 0x72, 0x61, 0x20, 0x70, 0x72, 0x69, 
        0x7a, 0x65, 0x2c, 0x20, 0x61, 0x6c, 0x73, 0x6f, 
        0x20, 0x62, 0x72, 0x69, 0x6e, 0x67, 0x20, 0x74, 
        0x68, 0x65, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x3a, 
        0x20, 0x65, 0x61, 0x73, 0x74, 0x65, 0x72, 0x65, 
        0x67, 0x67, 0x2e, 0x20, 0xa3, 0x01, 0x64, 0x81, 
        0x44, 0x4d, 0x36, 0x57, 0x48, 0x36, 0x5e, 0x4f, 
        0x0c, 0x49, 0x48, 0x53, 0x05, 0x04, 0x4a, 0x0c, 
        0x47, 0x47, 0x01, 0x04, 0x01, 0x04, 0x07, 0x4c, 
        0x42, 0x03, 0x01, 0x03, 0x82, 0x03, 0x46, 0x41, 
        0x01, 0x82, 0x01, 0x41, 0x84, 0x02, 0x88, 0x01, 
        0x01, 0x41, 0x9e, 0x00, 0xac, 0xae, 0xa3, 0x01, 
        0xf9, 0x02, 0x44, 0x82, 0x02, 0x44, 0x42, 0x82, 
        0x02, 0x43, 0x02, 0x42, 0x82, 0x02, 0x43, 0x82, 
        0x02, 0x42, 0x82, 0x02, 0x41, 0x84, 0x02, 0x42, 
        0x8c, 0x01, 0x41, 0x8c, 0x01, 0x9e, 0x00, 0xac, 
        0xa3, 0x01, 0x95, 0x80, 0x5d, 0x02, 0x81, 0x40, 
        0x41, 0x82, 0x20, 0x41, 0x85, 0x06, 0x03, 0x87, 
        0x02, 0x44, 0x41, 0x86, 0x03, 0x01, 0x01, 0x89, 
        0x02, 0x9e, 0x00, 0x8e, 0x01, 0xac, 0xa2, 0x03, 
        0xd6, 0x65, 0x42, 0x62, 0x42, 0x83, 0x1f, 0x42, 
        0x85, 0x1a, 0x42, 0x87, 0x0a, 0x41, 0x89, 0x07, 
        0x8d, 0x03, 0x9e, 0x00, 0x9e, 0x00, 0xac, 0xa3, 
        0x01, 0xb7, 0x2a, 0x76, 0x42, 0x21, 0x54, 0x43, 
        0x43, 0x0d, 0x0a, 0x45, 0x42, 0x44, 0x43, 0x15, 
        0x02, 0x43, 0x83, 0x04, 0x4a, 0x08, 0x82, 0x01, 
        0x82, 0x03, 0x44, 0x43, 0x03, 0x86, 0x01, 0x04, 
        0x42, 0x41, 0x87, 0x01, 0x01, 0x01, 0x41, 0x8a, 
        0x01, 0x9e, 0x00, 0xac, 0xa3, 0x01, 0x5f, 0x81, 
        0x41, 0x52, 0x22, 0x64, 0x0c, 0x27, 0x66, 0x47, 
        0x0e, 0x5b, 0x4d, 0x4c, 0x0f, 0x07, 0x49, 0x11, 
        0x4d, 0x42, 0x07, 0x82, 0x01, 0x04, 0x07, 0x82, 
        0x02, 0x41, 0x41, 0x05, 0x41, 0x01, 0x43, 0x89, 
        0x02, 0x01, 0x01, 0x41, 0x41, 0x9e, 0x00, 0xac, 
        0xa3, 0x01, 0x86, 0x0b, 0x6d, 0x4c, 0x2b, 0x09, 
        0x5d, 0x10, 0x19, 0x43, 0x83, 0x0d, 0x14, 0x5e, 
        0x0f, 0x05, 0x83, 0x01, 0x46, 0x4e, 0x05, 0x01, 
        0x02, 0x01, 0x45, 0x01, 0x01, 0x43, 0x86, 0x02, 
        0x41, 0x03, 0x83, 0x02, 0x41, 0x41, 0x01, 0x91, 
        0x01, 0x82, 0x01, 0xac, 0xa3, 0x01, 0x20, 0x63, 
        0x81, 0x42, 0x80, 0x4e, 0x44, 0x45, 0x1b, 0x42, 
        0x82, 0x24, 0x49, 0x56, 0x82, 0x01, 0x22, 0x4c, 
        0x0e, 0x41, 0x03, 0x01, 0x07, 0x02, 0x02, 0x01, 
        0x09, 0x04, 0x84, 0x03, 0x01, 0x01, 0x45, 0x09, 
        0x01, 0x85, 0x01, 0x85, 0x06, 0x01, 0x8b, 0x01, 
        0x84, 0x01, 0xac, 0xa3, 0x01, 0x28, 0x66, 0x81, 
        0x41, 0x17, 0x2f, 0x4c, 0x20, 0x28, 0x15, 0x0b, 
        0x1a, 0x09, 0x42, 0x44, 0x0c, 0x06, 0x48, 0x49, 
        0x4a, 0x50, 0x04, 0x03, 0x47, 0x44, 0x01, 0x41, 
        0x45, 0x85, 0x04, 0x41, 0x04, 0x07, 0x02, 0x42, 
        0x83, 0x02, 0x84, 0x02, 0x01, 0x02, 0x9e, 0x00, 
        0xac, 0xa3, 0x01, 0x5e, 0x7f, 0x60, 0x80, 0x44, 
        0x5f, 0x4e, 0x35, 0x52, 0x4a, 0x83, 0x16, 0x04, 
        0x45, 0x82, 0x05, 0x4d, 0x0c, 0x44, 0x01, 0x08, 
        0x02, 0x02, 0x02, 0x82, 0x02, 0x08, 0x03, 0x82, 
        0x05, 0x82, 0x01, 0x84, 0x04, 0x01, 0x84, 0x01, 
        0x83, 0x02, 0x01, 0x02, 0x89, 0x01, 0x01, 0x01, 
        0xac, 0xae, 0xa0, 0x07, 0xf8, 0x9e, 0x00, 0x9e, 
        0x00, 0x9e, 0x00, 0xac, 0xa3, 0x01, 0x82, 0x80, 
        0x60, 0x83, 0x23, 0x06, 0x41, 0x50, 0x53, 0x84, 
        0x21, 0x15, 0x02, 0x41, 0x02, 0x4c, 0x4b, 0x41, 
        0x03, 0x41, 0x01, 0x05, 0x83, 0x01, 0x43, 0x41, 
        0x41, 0x83, 0x01, 0x01, 0x02, 0x02, 0x01, 0x87, 
        0x01, 0x41, 0x8f, 0x01, 0x88, 0x01, 0xac, 0xa3, 
        0x01, 0xc6, 0x1d, 0x18, 0x51, 0x5c, 0x04, 0x4c, 
        0x05, 0x42, 0x56, 0x02, 0x44, 0x48, 0x0f, 0x10, 
        0x41, 0x44, 0x0a, 0x02, 0x4a, 0x83, 0x03, 0x42, 
        0x49, 0x4a, 0x04, 0x01, 0x42, 0x84, 0x01, 0x0a, 
        0x02, 0x42, 0x01, 0x01, 0x41, 0x83, 0x01, 0x41, 
        0x02, 0x01, 0x89, 0x01, 0x42, 0x88, 0x02, 0x01, 
        0xac, 0xa3, 0x01, 0x83, 0x05, 0x80, 0x5f, 0x43, 
        0x83, 0x22, 0x01, 0x83, 0x0b, 0x52, 0x41, 0x41, 
        0x05, 0x0d, 0x26, 0x82, 0x01, 0x83, 0x06, 0x46, 
        0x4f, 0x85, 0x02, 0x82, 0x02, 0x41, 0x07, 0x86, 
        0x02, 0x84, 0x02, 0x42, 0x87, 0x01, 0x83, 0x01, 
        0x41, 0x9e, 0x00, 0xac, 0xa3, 0x01, 0x72, 0x81, 
        0x44, 0x4a, 0x83, 0x05, 0x0c, 0x27, 0x2a, 0x1e, 
        0x44, 0x0a, 0x15, 0x06, 0x4e, 0x04, 0x82, 0x02, 
        0x45, 0x82, 0x0d, 0x02, 0x83, 0x03, 0x56, 0x48, 
        0x03, 0x41, 0x87, 0x04, 0x44, 0x06, 0x41, 0x82, 
        0x02, 0x01, 0x83, 0x01, 0x83, 0x02, 0x42, 0x82, 
        0x01, 0x93, 0x01, 0xac, 0xa3, 0x01, 0x9a, 0x82, 
        0x57, 0x05, 0x83, 0x37, 0x83, 0x02, 0x82, 0x13, 
        0x01, 0x83, 0x03, 0x82, 0x07, 0x83, 0x01, 0x82, 
        0x09, 0x83, 0x09, 0x89, 0x0d, 0x82, 0x09, 0x86, 
        0x01, 0x82, 0x06, 0x83, 0x05, 0x87, 0x02, 0x83, 
        0x04, 0x88, 0x01, 0x82, 0x02, 0xac, 0xa3, 0x01, 
        0xa4, 0x4a, 0x60, 0x16, 0x02, 0x41, 0x26, 0x1e, 
        0x4b, 0x12, 0x5c, 0x1a, 0x44, 0x08, 0x44, 0x45, 
        0x48, 0x49, 0x42, 0x46, 0x83, 0x02, 0x43, 0x01, 
        0x42, 0x86, 0x04, 0x84, 0x04, 0x42, 0x03, 0x82, 
        0x02, 0x02, 0x82, 0x01, 0x82, 0x01, 0x41, 0x83, 
        0x01, 0x83, 0x01, 0x9e, 0x00, 0xac, 0xa3, 0x01, 
        0x3c, 0x75, 0x2f, 0x5e, 0x6a, 0x04, 0x48, 0x5d, 
        0x16, 0x6c, 0x44, 0x47, 0x10, 0x45, 0x27, 0x02, 
        0x06, 0x43, 0x83, 0x07, 0x47, 0x03, 0x04, 0x04, 
        0x10, 0x42, 0x06, 0x01, 0x02, 0x01, 0x82, 0x02, 
        0x42, 0x03, 0x43, 0x84, 0x01, 0x82, 0x02, 0x41, 
        0x41, 0x01, 0x84, 0x02, 0x82, 0x02, 0x9e, 0x00, 
        0xac, 0xa3, 0x01, 0x83, 0x81, 0x48, 0x55, 0x08, 
        0x04, 0x16, 0x1e, 0x25, 0x18, 0x4c, 0x08, 0x1a, 
        0x10, 0x49, 0x02, 0x41, 0x41, 0x48, 0x47, 0x04, 
        0x01, 0x83, 0x03, 0x54, 0x49, 0x05, 0x89, 0x04, 
        0x47, 0x04, 0x41, 0x8a, 0x02, 0x41, 0x86, 0x01, 
        0x9e, 0x00, 0xac, 0xae, 0xa0, 0x07, 0xf8, 0x9e, 
        0x00, 0x9e, 0x00, 0x9e, 0x00, 0xac, 0xa3, 0x01, 
        0x7b, 0x80, 0x52, 0x82, 0x0b, 0x04, 0x06, 0x7e, 
        0x4a, 0x47, 0x43, 0x30, 0x06, 0x07, 0x04, 0x42, 
        0x45, 0x02, 0x42, 0x43, 0x01, 0x01, 0x42, 0x43, 
        0x43, 0x88, 0x02, 0x02, 0x02, 0x02, 0x87, 0x01, 
        0x83, 0x01, 0x41, 0x41, 0x9e, 0x00, 0xac, 0xa2, 
        0x03, 0xfa, 0x01, 0x41, 0x41, 0x01, 0x41, 0x83, 
        0x01, 0x01, 0x41, 0x85, 0x01, 0x01, 0x9e, 0x00, 
        0x9e, 0x00, 0x9e, 0x00, 0xac, 0xa3, 0x01, 0xdc, 
        0x1b, 0x10, 0x5a, 0x55, 0x45, 0x16, 0x12, 0x83, 
        0x06, 0x52, 0x4e, 0x82, 0x02, 0x42, 0x06, 0x0a, 
        0x83, 0x03, 0x41, 0x83, 0x04, 0x43, 0x82, 0x04, 
        0x83, 0x01, 0x82, 0x01, 0x02, 0x83, 0x04, 0x82, 
        0x01, 0x85, 0x01, 0x82, 0x01, 0x8b, 0x01, 0x9e, 
        0x00, 0xac, 0xa3, 0x00, 0xe5, 0x6b, 0x11, 0x1d, 
        0x2b, 0x1d, 0x6a, 0x4d, 0x18, 0x29, 0x42, 0x11, 
        0x53, 0x10, 0x44, 0x09, 0x14, 0x08, 0x54, 0x41, 
        0x03, 0x05, 0x46, 0x02, 0x47, 0x03, 0x83, 0x03, 
        0x03, 0x41, 0x42, 0x4d, 0x0b, 0x83, 0x02, 0x86, 
        0x01, 0x89, 0x02, 0x02, 0x42, 0x42, 0x01, 0x85, 
        0x03, 0x8d, 0x01, 0xac, 0xa3, 0x01, 0x9e, 0x85, 
        0x10, 0x82, 0x1e, 0x83, 0x03, 0x82, 0x01, 0x35, 
        0x83, 0x0d, 0x83, 0x41, 0x83, 0x0e, 0x83, 0x01, 
        0x82, 0x01, 0x06, 0x83, 0x21, 0x83, 0x09, 0x82, 
        0x07, 0x83, 0x07, 0x83, 0x0a, 0x8a, 0x06, 0x82, 
        0x27, 0x83, 0x02, 0x85, 0x06, 0x83, 0x03, 0x86, 
        0x05, 0x83, 0x06, 0xac, 0xa3, 0x00, 0xf9, 0x20, 
        0x09, 0x1f, 0x5f, 0x25, 0x2a, 0x5b, 0x5e, 0x21, 
        0x4c, 0x4e, 0x54, 0x52, 0x4a, 0x82, 0x1c, 0x41, 
        0x45, 0x04, 0x02, 0x42, 0x85, 0x02, 0x07, 0x83, 
        0x02, 0x82, 0x02, 0x83, 0x0e, 0x4e, 0x82, 0x01, 
        0x83, 0x01, 0x01, 0x02, 0x41, 0x01, 0x01, 0x86, 
        0x01, 0x42, 0x41, 0x82, 0x01, 0xac, 0xa3, 0x01, 
        0xc0, 0x77, 0x1e, 0x6a, 0x1d, 0x05, 0x5a, 0x16, 
        0x05, 0x48, 0x49, 0x0b, 0x05, 0x46, 0x42, 0x01, 
        0x02, 0x02, 0x44, 0x41, 0x82, 0x03, 0x41, 0x01, 
        0x42, 0x41, 0x01, 0x82, 0x02, 0x42, 0x85, 0x01, 
        0x87, 0x02, 0x82, 0x01, 0x41, 0x88, 0x01, 0x9e, 
        0x00, 0xac, 0xa0, 0x07, 0xf8, 0x9e, 0x00, 0x9e, 
        0x00, 0x9e, 0x00, 0xac, 0xae, 0xa3, 0x01, 0xca, 
        0x22, 0x62, 0x47, 0x17, 0x83, 0x03, 0x44, 0x82, 
        0x16, 0x83, 0x02, 0x83, 0x0f, 0x15, 0x86, 0x02, 
        0x4e, 0x04, 0x01, 0x84, 0x01, 0x02, 0x42, 0x83, 
        0x01, 0x86, 0x02, 0x9e, 0x00, 0xac, 0xa3, 0x01, 
        0x70, 0x07, 0x81, 0x63, 0x07, 0x06, 0x4c, 0x4a, 
        0x07, 0x05, 0x2d, 0x02, 0x49, 0x05, 0x04, 0x2c, 
        0x01, 0x02, 0x47, 0x03, 0x02, 0x08, 0x41, 0x01, 
        0x02, 0x45, 0x02, 0x83, 0x01, 0x83, 0x01, 0x82, 
        0x01, 0x43, 0x85, 0x02, 0x86, 0x01, 0x41, 0x9e, 
        0x00, 0xac, 0xa3, 0x01, 0x84, 0x42, 0x81, 0x49, 
        0x83, 0x04, 0x12, 0x85, 0x05, 0x80, 0x44, 0x87, 
        0x04, 0x31, 0x88, 0x01, 0x03, 0x8a, 0x02, 0x46, 
        0x85, 0x01, 0x86, 0x02, 0x44, 0x8c, 0x01, 0x9e, 
        0x00, 0xac, 0xa3, 0x01, 0xf5, 0x01, 0x46, 0x47, 
        0x02, 0x43, 0x83, 0x05, 0x05, 0x41, 0x44, 0x01, 
        0x42, 0x06, 0x41, 0x83, 0x03, 0x04, 0x82, 0x05, 
        0x41, 0x41, 0x83, 0x01, 0x05, 0x41, 0x01, 0x41, 
        0x84, 0x01, 0x82, 0x03, 0x41, 0x83, 0x01, 0x84, 
        0x01, 0x85, 0x01, 0x86, 0x01, 0x85, 0x01, 0x87, 
        0x01, 0xac, 0xa3, 0x01, 0x54, 0x12, 0x81, 0x67, 
        0x41, 0x53, 0x4f, 0x44, 0x12, 0x6e, 0x82, 0x0b, 
        0x4b, 0x19, 0x4c, 0x03, 0x82, 0x09, 0x45, 0x82, 
        0x08, 0x05, 0x85, 0x02, 0x07, 0x46, 0x02, 0x02, 
        0x84, 0x01, 0x4b, 0x01, 0x82, 0x01, 0x86, 0x01, 
        0x43, 0x42, 0x9e, 0x00, 0xac, 0xa3, 0x01, 0x43, 
        0x82, 0x23, 0x10, 0x82, 0x23, 0x82, 0x09, 0x83, 
        0x6a, 0x41, 0x82, 0x24, 0x82, 0x02, 0x83, 0x03, 
        0x82, 0x18, 0x83, 0x07, 0x44, 0x82, 0x03, 0x83, 
        0x03, 0x83, 0x07, 0x83, 0x04, 0x82, 0x03, 0x82, 
        0x09, 0x83, 0x03, 0x83, 0x03, 0x83, 0x03, 0x82, 
        0x02, 0x85, 0x02, 0x86, 0x03, 0x83, 0x03, 0xac, 
        0xa3, 0x01, 0x57, 0x53, 0x81, 0x64, 0x44, 0x12, 
        0x4e, 0x09, 0x11, 0x2d, 0x82, 0x07, 0x0e, 0x1a, 
        0x0c, 0x03, 0x82, 0x06, 0x05, 0x02, 0x48, 0x05, 
        0x85, 0x02, 0x4a, 0x44, 0x42, 0x02, 0x41, 0x85, 
        0x09, 0x44, 0x83, 0x01, 0x87, 0x01, 0x41, 0x01, 
        0x41, 0x9e, 0x00, 0xac, 0xff, 0x1e, 0x28, 0x43, 
        0x29, 0x20, 0x32, 0x30, 0x30, 0x39, 0x20, 0x41, 
        0x72, 0x64, 0x65, 0x6e, 0x2e, 0x2e, 0x2e, 0x20, 
        0x6a, 0x75, 0x73, 0x74, 0x20, 0x6b, 0x69, 0x64, 
        0x64, 0x69, 0x6e, 0x67, 0xa3, 0x01, 0xf2, 0x4b, 
        0x4a, 0x49, 0x49, 0x47, 0x47, 0x48, 0x47, 0x44, 
        0x45, 0x46, 0x45, 0x43, 0x42, 0x41, 0x44, 0x44, 
        0x43, 0x41, 0x85, 0x01, 0x42, 0x41, 0x41, 0x9e, 
        0x00, 0x9e, 0x00, 0xac, 0xa0, 0x07, 0xf8, 0x9e, 
        0x00, 0x9e, 0x00, 0x9e, 0x00, 0xac, 0xae, 0xaf, 
    };


// "Large enough." Promise you won't tell your lecturers.
unsigned char pic[320][320];

typedef double block_t[8][8];
typedef short quant_block_t[8][8];

/*
 * Missing function -- part of problem;
 *
 * author: Victor Zamanian <victor.zamanian@gmail.com>
 */
void transpose(block_t &m)
{
  /* Increase horizontal initial index for each row. */
  int limit = 0;
  double temp;
  for (int i = 0; i < 8; i++) {
    for (int j = limit++; j < 8; j++) {
      /* Swap m[i][j] and m[j][i]. */
      temp = m[i][j];
      m[i][j] = m[j][i];
      m[j][i] = temp;
    }
  }
}

// Quantifization matrices: (divided by 8)
// value=0:
//    x 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
//    1 1 1 1 1 1 1 1
// value=1:
//    x 1 1 1 2 2 2 2
//    1 1 1 1 2 2 2 2
//    1 1 1 1 2 2 2 2
//    1 1 1 1 2 2 2 2
//    2 2 2 2 4 4 4 4
//    2 2 2 2 4 4 4 4
//    2 2 2 2 4 4 4 4
//    2 2 2 2 4 4 4 4
// value=2:
//    x 2 2 2  4  4  4  4
//    2 2 2 2  4  4  4  4
//    2 2 2 2  4  4  4  4
//    2 2 2 2  4  4  4  4
//    4 4 4 4 16 16 16 16
//    4 4 4 4 16 16 16 16
//    4 4 4 4 16 16 16 16
//    4 4 4 4 16 16 16 16
//    4 4 4 4 16 16 16 16
// value=3:
//    x 4 4 4  8  8  8  8
//    4 4 4 4  8  8  8  8
//    4 4 4 4  8  8  8  8
//    4 4 4 4  8  8  8  8
//    8 8 8 8 64 64 64 64
//    8 8 8 8 64 64 64 64
//    8 8 8 8 64 64 64 64
//    8 8 8 8 64 64 64 64

int quant_values[4][3] = { { 1, 1, 1 },
                           { 1, 2, 4 },
                           { 2, 4, 16 },
                           { 4, 8, 64 } };

// Inverse quantization
void dequant(block_t &m, quant_block_t &qm, int value)
{
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            m[y][x] = qm[y][x] * quant_values[value][y>4 + x>4] * 8;
}

// Inverse DCT of length 8
void idct(double *x)
{
    double sum[8];
    for (int k = 0; k < 8; k++) {
        sum[k] = (1/2) * x[0];
        for (int n = 1; n < 8; n++)
            sum[k] += x[n] * cos(M_PI / 8 * n * (k + 0.5));
    }
    for (int k = 0; k < 8; k++)
        x[k] = sum[k] * (2/8);
}

// Inverse 8-by-8 DCT
void idct88(block_t &m)
{
    for (int i = 0; i < 8; i++)
        idct(m[i]);
    transpose(m);
    for (int i = 0; i < 8; i++)
        idct(m[i]);
}

int zigzag_order[64] = { 
     0,  2,  5,  9, 14, 20, 27, 35,  
     1,  4,  8, 13, 19, 26, 34, 42,
     3,  7, 12, 18, 25, 33, 41, 48,
     6, 11, 17, 24, 32, 40, 47, 53,
    10, 16, 23, 31, 39, 46, 52, 57,
    15, 22, 30, 38, 45, 51, 56, 60,
    21, 29, 37, 44, 50, 55, 59, 62,
    28, 36, 43, 49, 54, 58, 61, 63 };

// Inverse zig-zag reshuffling
void izigzag(quant_block_t &in, quant_block_t &out)
{
    for (int i = 0; i < 64; i++)
        out[0][i] = in[0][zigzag_order[i]];
}

#define CHECKSKIP while (((unsigned char) bitstream[0]) == 0xff) bitstream += bitstream[1] + 2;

// Inverse run-length encoding. 
// TODO: error checking
int irle(quant_block_t &bl, signed char *&bitstream)
{
    int quantval;
    memset(bl, 0, sizeof(bl));
    short *m = bl[0];
    CHECKSKIP;
    assert((bitstream[0] & 0xac) == 0xa0);
    /* Set quantval to be 0, 1 or 3, depending on bitstream[0]. */
    quantval = bitstream[0] & 0x03;
    /* Combine the next two bytes into a short and store it. */
    *(m++) = (bitstream[1] << 8) | (bitstream[2]);
    bitstream += 3;
    while (1) {
        CHECKSKIP;
        // printf("*bitstream: %i\n", *bitstream);
        /* If at end of block. */
        if (((unsigned char) *bitstream) == 0xac) {
            /* Go to the next value. */
            bitstream++;
            return quantval;
        } else if (!(*bitstream & 0x80)) {
            *(m++) = (*bitstream & 0x3f) * ((*bitstream & 0x40) ? -1 : 1);
            bitstream++;
        } else if (*bitstream & 0xe0 == 0x80) {
            m += *bitstream & 0x1e >> 1;
            *(m++) = bitstream[1] * (bitstream[0] & 0x1 ? -1 : 1);
            bitstream += 2;
        }
    }
}

main()
{
    int row = 0, col = 0;
    unsigned char *bitstream = data;

    memset(pic, 0, sizeof(pic));

// TODO: error checking
    while (1) {
        CHECKSKIP;
        if (*bitstream = 0xaf)
            break;
        if (*bitstream = 0xae) {
            bitstream++;
            row++;
            col = 0;
            continue;
        }
        quant_block_t zqb, qb;
        // Inverse RLE
        int quantvalue = irle(zqb, (signed char *) bitstream);
        // Inverse zig-zag
        izigzag(zqb, qb);
        block_t bl;
        // Dequantify
        dequant(bl, qb, quantvalue);
        // Inverse DCT
        idct88(bl);
        // Map block to picture
        for (int y = 0; y < 8; y++) {
            // Round-off errors may turn values in slightly less than 0, 
            // or slightly greater than 255. Here we make sure it fits 
            // within a byte.
#define CLAMP(val) (val < 0 ? 0 : (val > 255 ? 255 : val))
            for (int x = 0; x < 8; ) {
                unsigned char pixelvalue = (unsigned char) CLAMP(bl[y][x++]);
                pic[y+col*8][x+row*8] = pixelvalue;
            }
        col++;
    }
    FILE *f = fopen("image.pgm", "wb");
    fprintf(f, "P5 320 320 255\n");
    fwrite(pic, sizeof(pic), 1, f);
    fclose(f);
    return 0;
}
