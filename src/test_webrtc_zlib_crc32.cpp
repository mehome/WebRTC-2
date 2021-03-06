#include <stdio.h>
#include <stdint.h>
#include <zlib.h>

int main() {

  printf("\n\ntest_zlib_crc32\n\n");

 const unsigned char req[] =
   "\x00\x01\x00\x58"
   "\x21\x12\xa4\x42"
   "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
   "\x80\x22\x00\x10"
   "STUN test client"
   "\x00\x24\x00\x04"
   "\x6e\x00\x01\xff"
   "\x80\x29\x00\x08"
   "\x93\x2f\xf9\xb1\x51\x26\x3b\x36"
   "\x00\x06\x00\x09"
   "\x65\x76\x74\x6a\x3a\x68\x36\x76\x59\x20\x20\x20"
   "\x00\x08\x00\x14"
   "\x9a\xea\xa7\x0c\xbf\xd8\xcb\x56\x78\x1e\xf2\xb5"
   "\xb2\xd3\xf2\x49\xc1\xb5\x71\xa2"
   "\x80\x28\x00\x04"
   "\xe5\x7a\x3b\xcf";

 uint32_t checksum = crc32(0L, Z_NULL, 0);
 checksum = crc32(checksum, req, sizeof(req));

 printf("Checksum: %u\n\n", checksum);

  return 0;
}
