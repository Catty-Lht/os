//
// Created by 杜永坤 on 2019/6/11.
//

#include <afxres.h>
#include "Filesystem.h"

//获取从地址addr开始长度为lenth的数据
char *Filesystem::readBlock(int addr, int lenth) {
    char *a = (char *) malloc(sizeof(char) * lenth);
    int tmp = addr;
    int C, H, B;
    C = tmp / (DISK_H * DISK_B * DISK_SIZE);
    tmp %= (DISK_H * DISK_B * DISK_SIZE);
    H = tmp / (DISK_B * DISK_SIZE);
    tmp %= (DISK_B * DISK_SIZE);
    B = tmp / (DISK_SIZE);
    tmp %= (DISK_SIZE);
    BlockInfo blockInfo;
    blockInfo.C = C;
    blockInfo.H = H;
    blockInfo.B = B;
    Block block = Block(blockInfo);
    int offset = tmp;
//    int size = DISK_SIZE-tmp;
    for (int i = 0; i < lenth; ++i, offset++) {
        if (offset == DISK_SIZE) {
            offset = 0;
            block = block.nextBlock();
        }
        a[i] = block.getData()[offset];
    }
    return a;
}

//写入从addr开始长度为lenth的数据
void Filesystem::writeBlock(int addr, int lenth, char *data) {
    int tmp = addr;
    int C, H, B;
    C = tmp / (DISK_H * DISK_B * DISK_SIZE);
    tmp %= (DISK_H * DISK_B * DISK_SIZE);
    H = tmp / (DISK_B * DISK_SIZE);
    tmp %= (DISK_B * DISK_SIZE);
    B = tmp / (DISK_SIZE);
    tmp %= (DISK_SIZE);
    BlockInfo blockInfo;
    blockInfo.C = C;
    blockInfo.H = H;
    blockInfo.B = B;
    Block block = Block(blockInfo);
    int offset = tmp;
    for (int i = 0; i < lenth; ++i, offset++) {
        if (offset == DISK_SIZE) {
            offset = 0;
            block = block.nextBlock();
        }
        block.writeData(&data[i], offset, 1);
    }
}

Filesystem::Filesystem() {
    this->openedFile = NULL;
    for (int i = 0; i < 2834; i++) {
        this->bitmap[i] = *(this->readBlock(i, 1));
    }
    char *a;
    int gap = 32;
    int start = 6*DISK_SIZE;
    int tmp = start;
    pFile file=NULL;
    while (1) {
        a = (this->readBlock(tmp,1));
        if(a[0]=='\0'){
            break;
        } else {
            start = tmp;
            start += 1;
            file = new File;
            a = this->readBlock(start,27);
            file->menu.filename=a;
            start+=26;
//            file->menu.index=this->convertToINT(this->readBlock(start,4));
            a = this->readBlock(start,5);
            file->menu.index = (a[0]-'0')*10000+(a[1]-'0')*1000+(a[2]-'0')*100+(a[3]-'0')*10+(a[4]-'0');
            start = file->menu.index+5;
            a = this->readBlock(start,4);
            file->fileInfo.index = file->menu.index;
            file->fileInfo.block[0] = a[0]-'0';
            file->Filedata="";
            a = this->readBlock((46+file->fileInfo.block[0])*DISK_SIZE,100);
            file->Filedata = a;
            this->files.push_back(*file);
            tmp = tmp + gap;
        }
    }
}

Filesystem *Filesystem::refresh() {
    this->renewDisk();
    this->writeBlock(0,2834,this->bitmap);
    int menustart=6*DISK_SIZE;
    int filestart=26*DISK_SIZE;
    int gap = 32;
    char *a;
    vector<File>::iterator iter;
    int filesize=0;
    for (iter=this->files.begin();iter!=this->files.end();iter++)
    {
        filesize++;
    }
    for (int i = 0; i < filesize; ++i) {
        a = (char *)malloc(sizeof(char));
        a[0] = '\001';
        this->writeBlock(menustart,1,a);
        a = (char *)malloc(sizeof(char)*27);
        strcpy(a,this->files[i].menu.filename.c_str());
        this->writeBlock(menustart+1,this->files[i].menu.filename.length(),a);
//        this->writeBlock(menustart+28,1,(char *)&filestart);
//        this->writeBlock(menustart+29,1,(char *)&filestart+1);
//        this->writeBlock(menustart+30,1,(char *)&filestart+2);
//        this->writeBlock(menustart+31,1,(char *)&filestart+3);
        FILE *fp = fopen("fdisk","r+");
        fseek(fp,menustart+27,SEEK_SET);
        fprintf(fp,"%d",filestart);
//        this->writeBlock(filestart,1,(char *)&filestart);
//        this->writeBlock(filestart+1,1,(char *)&filestart+1);
//        this->writeBlock(filestart+2,1,(char *)&filestart+2);
//        this->writeBlock(filestart+3,1,(char *)&filestart+3);
        fseek(fp,filestart,SEEK_SET);
        fprintf(fp,"%d",filestart);
//        this->writeBlock(filestart+4,1,(char *)&(this->files[i].fileInfo.block[0]));
//        this->writeBlock(filestart+5,1,(char *)&(this->files[i].fileInfo.block[0])+1);
//        this->writeBlock(filestart+6,1,(char *)&(this->files[i].fileInfo.block[0])+2);
//        this->writeBlock(filestart+7,1,(char *)&(this->files[i].fileInfo.block[0])+3);
        fprintf(fp,"%d",this->files[i].fileInfo.block[0]);
        fclose(fp);
        a = (char *)malloc(sizeof(char)*this->files[i].Filedata.length());
        strcpy(a,this->files[i].Filedata.c_str());
        this->writeBlock((46+this->files[i].fileInfo.block[0])*DISK_SIZE,this->files[i].Filedata.length(),a);
        menustart += gap;
        filestart += gap;
    }
    return new Filesystem();
}

void Filesystem::ls() {
    for (int i = 0; i < this->files.size(); i++) {
        cout << "name:" << this->files[i].menu.filename << "\tsize:" << this->files[i].Filedata.length() << endl;
    }
}

void Filesystem::create(string filename) {
    pFile fp = new File;
    fp->Filedata = "";
    fp->menu.filename = filename;
    fp->menu.index = 0;
    fp->fileInfo.index = 0;
    for (int i = 0; i < 2834; ++i) {
        if (this->bitmap[i] == '\0') {
            fp->fileInfo.block[0] = i;
            this->bitmap[i] = '\001';
            break;
        }
    }
    this->files.push_back(*fp);
}

void Filesystem::open(string filename) {
    if (this->openedFile != NULL) {
        printf("a file already opened\n");
    } else {
        for (int i = 0; i < this->files.size(); ++i) {
            if (this->files[i].menu.filename == filename) {
                this->openedFile = &this->files[i];
                break;
            }
        }
    }
}

void Filesystem::close() {
    if (this->openedFile == NULL) {
        printf("no file opened\n");
    } else {
        this->openedFile = NULL;
    }
}

void Filesystem::destroy(string filename) {
//    for(vector<File>::iterator itr;itr != this->files.end();itr++){
//        if((*itr).menu.filename==filename){
//            this->files.erase(itr);
//            break;
//        }
//    }
    for (int i = 0; i < this->files.size(); ++i) {
        if (this->files[i].menu.filename == filename) {
            this->files.erase(this->files.begin() + i);
            break;
        }
    }
}

void Filesystem::read() {
    if (this->openedFile==NULL){
        cout << "haven't open file" << endl;
    } else {
        cout << this->openedFile->Filedata << endl;
    }
}

void Filesystem::write(string data) {
    this->openedFile->Filedata = data;
}

bool Filesystem::isOpened() {
    return this->openedFile != NULL;
}

string Filesystem::getOpenedFile() {
    return this->isOpened() ? this->openedFile->menu.filename : "";
}

int Filesystem::convertToINT(const char *char_4bit_value) {
    size_t len = strlen(char_4bit_value);
    if (4 != len) {
        return -1;
    }
    int v1 = 0;
    int v2 = 0;
    int v3 = 0;
    int v4 = 0;
    char a[4] = {0};
    try {
        v1 = (int) char_4bit_value[0];
        v2 = (int) char_4bit_value[1];
        v3 = (int) char_4bit_value[2];
        v4 = (int) char_4bit_value[3];
        a[0] = static_cast<char>(v1);
        a[1] = static_cast<char>(v2);
        a[2] = static_cast<char>(v3);
        a[3] = static_cast<char>(v4);
    }
    catch (runtime_error &) {
        printf("Failed!,char 4 bit convert to int error! (For : char not 4 bit)");
    }
    int result_value = 0;
//    result_value = htons(static_cast<u_short>(*((short *) a + 1)));
    memcpy(&result_value,char_4bit_value,4);
    return result_value;
}

void Filesystem::renewDisk() {
    FILE *fp = fopen("fdisk","w");
    //磁盘文件初始化
    char a = '\0';
    //创建磁盘
    fseek(fp,0,SEEK_SET);
    for(int i=0;i<DISK_C*DISK_B*DISK_H*DISK_SIZE;i++){
        fprintf(fp,"%c",a);
    }
    fclose(fp);
}
