#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED





#include <vector>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "Block.h"
#include <iostream>

/*
 * 0-5扇区:位图
 * 6-25扇区:文件目录
 * 26-45扇区:文件信息
 * 46-2879扇区:文件内容
 * 最多404个文件
 * */

using namespace std;

typedef struct {
    string filename;
    int index;
}Menu,*pMenu;

typedef struct {
    int index;
    int block[7];
}FileInfo,*pFileInfo;

typedef struct {
    Menu menu;
    FileInfo fileInfo;
    string Filedata;
}File,*pFile;

class Filesystem {
private:
    char bitmap[2834];
    vector<File> files;
    File *openedFile;
    bool isOpened();
    int convertToINT(const char *char_4bit_value);
public:
    Filesystem();
    Filesystem *refresh();
    char *readBlock(int addr,int lenth);
    void writeBlock(int addr,int lenth,char *data);
    void ls();
    void create(string filename);
    void destroy(string filename);
    void open(string filename);
    void read();
    void write(string data);
    void close();
    string getOpenedFile();
    void renewDisk();
};




#endif // FILESYSTEM_H_INCLUDED
