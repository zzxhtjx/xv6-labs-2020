#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
int
main(int argc, char *argv[])
{
  //记得函数是会阻塞的
  int fd = 0;//从标准输入进行读数据啊
  char buf[1024] = {0};
  int len = read(fd, buf, sizeof (buf));//最终的长度啊
  int p = 0, cnt = 0;
  for(int i = 0; i < len;i++){
    if(buf[i] == '\n'){
      //表示的是换行，拷贝
      buf[i] = 0;
      int pid = fork();
      if(pid){
        //可能等待的时间比较久
        int x;
        wait(&x);
        p = i + 1;
        cnt = 0;
        continue;
      }else {
        //子进程
        char* argv2[cnt + argc];//表示的参数,第一个是名字
        for(int i = 0; i < cnt + argc; i++){
          argv2[i] = (char*)malloc(50);//分配长度
        }
        for(int i = 1; i < argc; i++){//第一个参数是xargs,第二个参数是运行的名字
          strcpy(argv2[i - 1], argv[i]);
        }
        for(int i = argc - 1; i < cnt + argc; i++){
          strcpy(argv2[i], buf + p);
          p = p + strlen(buf + p) + 1;
        }
        exec(argv[1], argv2);
        exit(0);//直接进行结束了
      }
    }
    if(buf[i] == ' '){
      cnt++;
      buf[i] = 0;
      continue;
    }
  }
  exit(0);
}
//使用的是c和c++对字符串的处理进行判断标准输入
