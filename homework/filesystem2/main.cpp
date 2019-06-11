#include <iostream>
#include "Filesystem.h"

using namespace std;

void help() {
    cout << " help                 //list commands" << endl;
    cout << " directory            //list Menu" << endl;
    cout << " create  [filename]   //create file" << endl;
    cout << " destroy [filename]   //destroy file" << endl;
    cout << " open    [filename]   //open file" << endl;
    cout << " read                 //read data" << endl;
    cout << " write   [data]       // write data" << endl;
    cout << " close                //close file" << endl;
    cout << " fr                   //refresh disk" << endl;
    cout << " exit                 //exit the program" << endl;
}

int main() {
    Filesystem *filesystem = new Filesystem();
    help();
    while (1) {
        cout << " /" << filesystem->getOpenedFile() << " > ";
        string cmd;
//        cin >> cmd;
        getline(cin,cmd);
        char a[3];
        a[0] = cmd[0];
        a[1] = cmd[1];
        a[2] = '\0';
        int fl=0;
        for(int i=0;cmd.length();i++)
        {
            if(cmd[i]==' ')
            {
                fl=i+1;
             break;
            }

        }
        string cmdHead = a;
        if (cmdHead == "di") {
            filesystem->ls();
        } else if (cmdHead == "cr") {
            string filename = cmd.substr(fl, cmd.length());
            filesystem->create(filename);
        } else if (cmdHead == "de") {
            string filename = cmd.substr(fl, cmd.length());
            filesystem->destroy(filename);
        } else if (cmdHead == "op") {
            string filename = cmd.substr(fl, cmd.length());
            filesystem->open(filename);
        } else if (cmdHead == "re") {
            filesystem->read();
        } else if (cmdHead == "wr") {
            string data = cmd.substr(fl, cmd.length());
            filesystem->write(data);
        } else if (cmdHead == "cl") {
            filesystem->close();
        } else if (cmdHead == "fr") {
            filesystem->refresh();
        } else if (cmdHead == "ex") {
            break;
        } else if (cmdHead == "he") {
            help();
        } else if (cmdHead == "cc"){
            filesystem->renewDisk();
        } else {
            cout << "error input" << endl;
        }
    }
//    char *a= const_cast<char *>("aaaaaaaaa");
//    filesystem->writeBlock(0,10,a);
//    filesystem->renewDisk();
    return 0;
}
