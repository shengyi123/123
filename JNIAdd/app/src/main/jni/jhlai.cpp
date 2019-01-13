//
// Created by rabbi on 2018/10/18.
//

#include "shengyi.h"
#include "LOG.h"
#include <cstdio>
#include <time.h>


int computation(int d, int d1) {
int i;
int mul=0;
for(i=5;i>0;i--){
    mul = mul+d*d1;
    LOGD("mul =%d ",mul);
}
    return 0;
}



int write(){
    /*FILE *fp;
    char str[1024] = "test xxxx";
    fp = fopen("/storage/emulated/0/test.txt ", "w");
    fwrite (str , sizeof(char), sizeof(str), fp);
    fclose(fp);*/
    float float1,float2;
    long long1,long2;
    int int1 = 0x12;
    int int2 = 0x16;
    short short1=0x12;
    short short2=0x16;

    char char1='A';
    char char2='B';
    int begintime,endtime;
    begintime = clock();
    computation(char1,char2);
    endtime = clock();
    LOGD("result = %d",char1*char2);
    LOGD("running time = %d",endtime-begintime);

    return 0;
    }
