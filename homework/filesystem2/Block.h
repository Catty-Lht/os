#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED
//
// Created by 杜永坤 on 2019/6/10.
//



#include <stdlib.h>
#include <stdio.h>

//柱面
#define DISK_C 2
//磁头
#define DISK_H 80
//扇区
#define DISK_B 18
//扇区大小
#define DISK_SIZE 512


typedef struct {
    int C;
    int H;
    int B;
}BlockInfo;

class Block {
private:
    int C;
    int H;
    int B;
    int offset;
    char *data;
    char *readBlock();
public:
    Block(int C, int H, int B);
    Block(BlockInfo blockInfo);
    char *getData();
    //返回值表示剩余未写入的数据
    int writeData(char *data,int offset,int lenth);
    Block nextBlock();
};


#endif // BLOCK_H_INCLUDED
