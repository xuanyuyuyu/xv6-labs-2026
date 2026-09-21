#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define MAXARGS 10

char *basename(char *path) {
  char *name = path;
  char *p;

  for(p = path; *p != '\0'; p ++) {
    if(*p == '/')
      name = p + 1;
  }
  return name;
}


void run_command(char **cmd, char *path) {
  char *args[MAXARGS];
  int i;

  if(fork() == 0) {
    //子进程
    for(i = 0; cmd[i] != 0 && i < MAXARGS - 2; i ++) 
      args[i] = cmd[i];
    
    args[i ++] = path;
    args[i] = 0;

    exec(args[0], args);
    fprintf(2, "find: exec %s failed\n", args[0]);
    exit(1);
  }
  //父进程回收子进程
  wait(0);
}

void find(char *path, char *target, char **cmd) {
  char buf[512];
  char *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  //当前path本身名称匹配时：打印，或者执行-exec命令
  if(strcmp(basename(path), target) == 0) {
    if(cmd != 0) {
      run_command(cmd, path);
    } else {
      printf("%s\n", path);
    }
  }
  
  // 普通文件不能继续遍历
  if (st.type != T_DIR) {
    close(fd);
    return;
  }

  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }

  strcpy(buf, path);
  p = buf + strlen(buf);
  *p ++ = '/';

  //每次读取一个目录项
  while(read(fd, &de, sizeof(de)) == sizeof(de)) {
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = '\0';
    //跳过 . 和 ..
    if(strcmp(p, ".") == 0 || strcmp(p, "..") == 0) {
      continue;
    }
    find(buf, target, cmd);
  }

  close(fd);
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
    fprintf(2, "Usage: find path name [-exec command ...]\n");
    exit(1);
  }
  if(argc == 3) {
    find(argv[1], argv[2], 0);
  } else {
    if(strcmp(argv[3], "-exec") != 0) {
      fprintf(2, "Usage: find path name [-exec command ...]\n");
      exit(1);
    }
    if(argc < 5) {
      fprintf(2, "find: missing command after -exec\n");
      exit(1);
    }
    find(argv[1], argv[2], &argv[4]);
  }

  exit(0);
}