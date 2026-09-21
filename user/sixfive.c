#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int isseparator(char c) {
  return strchr(" -\r\t\n./,", c) != 0;
}

void finish_number(int value, int has_digits, int valid)
{
  if (valid && has_digits && (value % 5 == 0 || value %
  6 == 0))
    printf("%d\n", value);
}


void sixfive(int fd) {
  char c;
  int n;
  int value = 0;
  int has_digits = 0;
  int valid = 1;

  while((n = read(fd, &c, 1)) > 0) {
    if(isseparator(c)) {
      finish_number(value, has_digits, valid);

      value = 0;
      has_digits = 0;
      valid = 1;
    } else if (c >= '0' && c <= '9') {
      if(valid) {
        value = value * 10 + (c - '0');
        has_digits = 1;
      }
    } else {
      //可能有字母
      valid = 0;
    }
  }
  //EOF也是分隔符，处理最后一个
  finish_number(value, has_digits, valid);
  if(n < 0) {
    fprintf(2, "sixfive: read error\n");
  }
}



int main(int argc, char *argv[]) {
  int fd;
  int i;
  if(argc == 1) {
    sixfive(0);
    exit(0);
  }

  for(i = 1; i < argc; i ++) {
    fd = open(argv[i], O_RDONLY);
    if(fd < 0) {
      fprintf(2, "cannot open %s\n", argv[i]);
      continue;
    }
    sixfive(fd);
    close(fd);
  }
  exit(0);

}