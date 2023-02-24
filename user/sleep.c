#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char* argv[]){
    if(argc != 2){
        fprintf(2, "Usage: sleep time...\n");//标准输出到终端
        exit(1);
    }
    int cnt = atoi(argv[1]);
    if(sleep(cnt) == -1){
        fprintf(2, "sleep %d second fail\n", argv[1]);
    }
    exit(0);
}
