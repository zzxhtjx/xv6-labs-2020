#include"kernel/types.h"
#include"kernel/stat.h"
#include"user/user.h"
int fd[2];//0->读,1->写

int main(){
    int ret = pipe(fd);
    if(ret == -1){
        printf("pipe fail\n");
        exit(-1);
    }
    int pid = fork();
    if(pid){//父进程
        char buf[1] = {0};
        write(fd[0], "zzx ", 4);//阻塞的函数所以不需要进行操作
        int x;
        wait(&x);//阻塞等待
        read(fd[1], buf, sizeof(buf));
        int pid = getpid();
        printf("%d: received pong\n",pid);
        close(fd[0]);
        close(fd[1]);
    }else {
        char buf[1] = {0};
        read(fd[1], buf, sizeof(buf));
        int pid = getpid();
        printf("%d: received ping\n",pid);
        write(fd[0], "pxm ", 4);
        close(fd[0]);
        close(fd[1]);
    }
    return 0;
}