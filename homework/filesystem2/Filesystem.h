#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED





#include <vector>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "Block.h"
#include <iostream>

/*
 * 0-5����:λͼ
 * 6-25����:�ļ�Ŀ¼
 * 26-45����:�ļ���Ϣ
 * 46-2879����:�ļ�����
 * ���404���ļ�
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
