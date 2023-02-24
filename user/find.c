#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{//memcpy与memmove的区别 对于重叠部分的话memcpy会出错
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)//太长了直接进行返回
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));//按照字节进行设置
  return buf;
}
char* x = ".";
char* y = "..";

void
find(char *path,char *filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  //dubug 2
//   printf("bug :  %s %s\n", path,filename);
  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){//查看程序的状态
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    //check the file the same as T_FILE
    if(strcmp(filename, fmtname(path)) == 0){
        //debug 1
        printf("%s\n", path);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){//读的话是会一个文件一个文件的读吗
      if(de.inum == 0)
        continue;
      //add the end flag '\0';
      if((strcmp(x, de.name) == 0) || (strcmp(y, de.name) == 0)){
        continue;
      }   
      //avoid the recursion
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      //maybe overflow the stack
      find(buf, filename);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc != 3){//argument error 
    printf("find diratory filename!!!argument fail");
    exit(-1);
  }
  static char tmp[DIRSIZ + 1];
  memcpy(tmp, argv[2],strlen(argv[2]));
  memset(tmp + strlen(argv[2]), ' ', DIRSIZ-strlen(argv[2]));
  find(argv[1], tmp);
  exit(0);
}
