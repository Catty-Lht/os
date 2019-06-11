//
// Created by 杜永坤 on 2019/6/11.
//

#include "Block.h"

Block::Block(int C, int H, int B){
    if(C>=DISK_C) {
        printf("柱面超过限制\n");
        exit(1);
    }
    else
        this->C = C;
    if(H>=DISK_H) {
        printf("磁头超过限制\n");
        exit(1);
    }
    else
        this->H = H;
    if(B>=DISK_B) {
        printf("扇区超过限制\n");
        exit(1);
    }
    else
        this->B = B;
    this->offset = this->C*DISK_B*DISK_H*DISK_SIZE+this->H*DISK_B*DISK_SIZE+this->B*DISK_SIZE;
    this->data = readBlock();
}


Block::Block(BlockInfo blockInfo) {
    this->B = blockInfo.B;
    this->H = blockInfo.H;
    this->C = blockInfo.C;
    this->offset = this->C*DISK_B*DISK_H*DISK_SIZE+this->H*DISK_B*DISK_SIZE+this->B*DISK_SIZE;
    this->data = readBlock();
}

char *Block::readBlock() {
    char *ptr = (char *)malloc(sizeof(char)*DISK_SIZE);
    FILE *fdisk = fopen("fdisk","r+");
    fseek(fdisk,this->offset,SEEK_SET);
    for(int i=0;i<DISK_SIZE;i++){
        ptr[i] = (char)fgetc(fdisk);
    }
    fclose(fdisk);
    return ptr;
}

char *Block::getData() {
    return this->data;
}

int Block::writeData(char *data, int offset,int lenth) {
    FILE *fdisk = fopen("fdisk","r+");
    fseek(fdisk,this->offset+offset,SEEK_SET);
    int l;
    int s = 0;
    if(lenth+offset>=DISK_SIZE){
        l = DISK_SIZE-offset;
        s = lenth-l;
    } else{
        l = lenth;
    }
//    int l = lenth+offset>=DISK_SIZE ? DISK_SIZE-offset : lenth;
    for(int i=0;i<l;i++){
        fputc(data[i],fdisk);
    }
    fclose(fdisk);
    return s;
}

Block Block::nextBlock() {
    if(this->B+1<DISK_B){
        return Block(this->C,this->H,this->B+1);
    } else{
        if(this->H+1<DISK_H){
            return Block(this->C,this->H+1,0);
        } else{
            if(this->C<DISK_C){
                return Block(this->C+1,0,0);
            } else{
                printf("超出内存限制\n");
                exit(1);
            }
        }
    }
}

