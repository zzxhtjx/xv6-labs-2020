#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
/*
Your goal is to use pipe and fork to set up the pipeline. The first process feeds the numbers 2 through 35 into the pipeline. For each prime number, you will arrange to create one process that reads from its left neighbor over a pipe and writes to its right neighbor over another pipe. Since xv6 has limited number of file descriptors and processes, the first process can stop at 35.

Some hints:

Be careful to close file descriptors that a process doesn't need, because otherwise your program will run xv6 out of resources before the first process reaches 35.
Once the first process reaches 35, it should wait until the entire pipeline terminates, including all children, grandchildren, &c. Thus the main primes process should only exit after all the output has been printed, and after all the other primes processes have exited.
Hint: read returns zero when the write-side of a pipe is closed.
It's simplest to directly write 32-bit (4-byte) ints to the pipes, rather than using formatted ASCII I/O.
You should create the processes in the pipeline only as they are needed.
Add the program to UPROGS in Makefile.
*/

//实现一次性读完,否则边写边读都会有问题
//第二个管道的返回值的问题
//第三个是处理标记(自己的个人问题)
//文件描述符的复用
int main(){
    int fd[2];
    pipe(fd); 
    for(int i = 2;i <= 35; i++){
        write(fd[1], &i, 4);
    }//写阻塞
    while(1){
        int p;
        if(read(fd[0], &p, 4)){
            printf("prime %d\n", p);
        }else {
            close(fd[0]);
            close(fd[1]);
            break;//已经结束了
        }
        int x;
        int arr[40] = {0};
        close(fd[1]);//这一步没有的话会T,利用的是所有的管道写端都关闭了所以会会有这个现象
        int s = 0;
        while(read(fd[0], &x, 4) > 0){
            if(x % p){
                arr[x] = 1;//是本身啊
                s++;
            }
        }
        close(fd[0]);//把读的也关闭了
        if(!s) break;
        pipe(fd);
        for(int i = 2; i <= 35; i++){
            if(arr[i]){
                write(fd[1], &i, 4);
            }
        }
        int pid = fork();
        if(pid > 0){//父进程
            close(fd[0]);
            close(fd[1]);
            wait(&x);
            break;//记得要退出
        }else {
            //子进程
            continue;
        }
    }
    exit(0);
}