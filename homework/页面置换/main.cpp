#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#define P 32
#define F 8
#define RS_MAX_SIZE 32

//p����������ʼλ��
//e��ҳ��
//m���������ƶ���
//t���Ƚ�ֵ

int rs[RS_MAX_SIZE];
int rs_size;

int M[F];
int m_size = 0;

int oldest_FIFO = 0;//used in FIFO

int queue_LRU[F];//used in LRU
int queue_size = 0;

int replace_clock = 0;//used in Clock
int access_bit[F] = { 0 };
int modify_bit[F] = { 0 };

#define MAX 1024
//ҳ�������ܳ��ȡ�����������ʼλ��p����������
int pageNum=32,pageStart=1,blockNum=3;
//�����״̬����
int **blockSta;
//ҳ����������
int *page;
//ҳ������ָ��
int pPage;
//ȱҳ�ж�����
char *interrupt;
//ȱҳ����
int lakePage;
//�����ڴ�N
int memory;
//�������а�����ҳ��e
int everyWorkSet;
//�������ƶ���m
int moveRate;

void CreateRS() {
	rs_size = 0;
	srand((unsigned)time(NULL));
	int p = rand() % P;
	int e = 8;
	int m = 4;
	double t = rand() / (RAND_MAX + 1.0);
	int rs_index = 0;
	while (rs_size + m <= RS_MAX_SIZE) {
		for (int i = 0; i < m; i++) {
			int cur = rand() % e + p;
			rs[rs_index++] = cur;
		}
		double r = rand() / (RAND_MAX + 1.0);
		if (r < t)
			p = rand() % P;
		else
			p = (p + 1) % P;
		rs_size += m;
	}
}

bool Contains(int p) {
	for (int i = 0; i < m_size; i++) {
		if (M[i] == p)
			return true;
	}
	return false;
}

void OPT_Rep(int i) {//i is the current index of rs
	int visited[F] = { 0 };
	for (int j = 0; j < F; j++) {
		for (int m = i + 1; m < rs_size; m++) {
			if (M[j] == rs[m]) {
				visited[j] = m - i;
				break;
			}
		}
	}
	int replace_index;
	int temp = 0;
	for (int j = 0; j < F; j++) {
		if (visited[j] == 0) {
			M[j] = rs[i];
			return;
		}
		if (visited[j] > temp) {
			temp = visited[j];
			replace_index = j;
		}
	}
	M[replace_index] = rs[i];
}

/*
Name:OPT()
Achieve:����û��㷨
*/
void OPT() {
	int replace_num = 0;
	for (int i = 0; i < rs_size; i++) {
		if (Contains(rs[i]))
			continue;
		if (m_size < F) {
			M[m_size++] = rs[i];
		}
		else {
			OPT_Rep(i);
			replace_num++;
		}
	}
	printf("OPTReplace: \n     the page fault rate of replacement is %6.3f%%", 100 * ((float)replace_num / P));
	printf("\n\n");
}

/*
Name:void RAD()
Achieve:����û��㷨��Random permutation��
���ڴ����أ����ѡ��һ��������滻
*/
void RAD() {
	int replace_num = 0;
	for (int i = 0; i < rs_size; i++) {
		if (Contains(rs[i]))
			continue;
		if (m_size < F)
			M[m_size++] = rs[i];
		else {
			srand((unsigned)time(NULL));
			int random = rand() % F;
			M[random] = rs[i];
			replace_num++;
		}
	}
	printf("RandomReplace: \n     the page fault rate of replacement is %6.3f%%", 100 * ((float)replace_num / P));
	printf("\n\n");
}

/*
Name:void FIFO()
Achieve:�Ƚ��ȳ�����Fisrt In First Out��
���һ���������Ƚ��뻺���У���Ӧ�����类��̭
ÿ���滻���Ƚ����ڴ��ҳ��
*/
void FIFO() {
	int replace_num = 0;
	for (int i = 0; i < rs_size; i++) {
		if (Contains(rs[i])) {
			continue;
		}
		if (m_size < F) {
			M[m_size++] = rs[i];
		}
		else {
			M[oldest_FIFO] = rs[i];
			oldest_FIFO = (oldest_FIFO + 1) % F;
			replace_num++;
		}
	}
	printf("FIFOReplace: \n     the page fault rate of replacement is %6.3f%%", 100 * ((float)replace_num / P));
	printf("\n\n");
}

/*
Name:  void LRU ()
Achieve: ������δʹ�ã�Least Recently Used��
���һ�����������һ��ʱ��û�б����ʵ�����ô�ڽ��������ʵĿ�����Ҳ��С
��̭�ʱ��δ��ʹ�õ�ҳ��
*/
void LRU() {
	int replace_num = 0;
	for (int i = 0; i < rs_size; i++) {
		if (Contains(rs[i])) {
			for (int j = 0; j < queue_size; j++) {
				if (queue_LRU[j] == rs[i]) {
					for (int m = j; m < queue_size; m++)
						queue_LRU[m] = queue_LRU[m + 1];
					queue_LRU[queue_size - 1] = rs[i];
				}
				break;
			}
			continue;
		}
		if (m_size < F) {
			queue_LRU[queue_size++] = rs[i];
			M[m_size++] = rs[i];
		}
		else {
			for (int j = 0; j < F; j++) {
				if (M[j] == queue_LRU[0]) {
					for (int m = 0; m < queue_size; m++) {
						queue_LRU[m] = queue_LRU[m + 1];
					}
					queue_LRU[queue_size - 1] = M[j];
					M[j] = rs[i];
					break;
				}
			}
			replace_num++;
		}
	}
	printf("LRUReplace: \n     the page fault rate of replacement is %6.3f%%", 100 * ((float)replace_num / P));
	printf("\n\n");
}

/*
Name:  void Clock ()
Achieve: ��clock�㷨
ҳ�汻���ʺ�access_bit[j]��1������ǰ����λaccess_bit[j]Ϊ0�������ó�������ǰ����λaccess_bit[j]Ϊ1������Ϊ0
*/
void Clock() {
	int replace_num = 0;
	for (int i = 0; i < rs_size; i++) {
		if (Contains(rs[i])) {
			for (int j = 0; j < m_size; j++) {
				if (M[j] == rs[i]) {
					access_bit[j] = 1;
				}
			}
			continue;
		}
		if (m_size < F) {
			access_bit[m_size] = 1;
			M[m_size++] = rs[i];
		}
		else {
			while (true) {
				if (access_bit[replace_clock] == 0) {
					M[replace_clock] = rs[i];
					access_bit[replace_clock] = 1;
					break;
				}
				else {
					access_bit[replace_clock] = 0;
					replace_clock = (replace_clock + 1) % F;
				}
			}
			replace_num++;
		}
	}
	printf("ClockReplace: \n     the page fault rate of replacement is %6.3f%%", 100 * ((float)replace_num / P));
	printf("\n\n");
}

/*
Name:  void MoreClock ()
Achieve: �Ľ�Clock�㷨
����λaccess���޸�λmodify��ͨ����������ɨ���ҳ����ʺ���̭��ҳ��
*/
void MoreClock() {
	int replace_num = 0;
	int flag = 0;
	int temp = 1;
	for (int i = 0; i < rs_size; i++) {
		if (Contains(rs[i])) {
			for (int j = 0; j < m_size; j++) {
				if (M[j] == rs[i]) {
					access_bit[j] = 1;
				}
			}
			//continue;
		}
		else {
			for (int j = 0; j < m_size; j++) {
				if (M[j] == rs[i]) {
					modify_bit[j] = 1;
				}
			}
			if (m_size < F) {
				access_bit[m_size] = 1;
				M[m_size++] = rs[i];
			}
			else {
				while (temp == 1) {
				oncemore:	if (access_bit[replace_clock] == 0 && modify_bit[replace_clock] == 0) {//ɨ��ѭ�����У�Ѱ��A=0��M=0�ĵ�һ��ҳ��
						M[replace_clock] = rs[i];
						access_bit[replace_clock] = 0;
						temp = 0;
					}
					else {
						if (flag) {
							if (access_bit[replace_clock] == 0 && modify_bit[replace_clock] == 1) {//ɨ��ѭ�����У�Ѱ��A=0��M=0�ĵ�һ��ҳ��
								M[replace_clock] = rs[i];
								access_bit[replace_clock] = 0;
								temp = 0;
							}
							else {
								access_bit[replace_clock] = 0;
								if (replace_clock + 1 == F) {//����ڶ���Ҳʧ�ܣ���δ�ҵ��ڶ���ҳ�棬��ָ�뷵�ص���ʼ��λ�ã��������еķ���λ��0
									replace_clock = (replace_clock + 1) % F;
									access_bit[F] = { 0 };
									flag = 0;
									goto oncemore;
								}
								replace_clock = (replace_clock + 1) % F;
							}
							//flag = 0;
						}
						else {
							if (replace_clock + 1 == F)//�����û��ɨ��һ��
								flag = 1;
							//access_bit[replace_clock] = 0;
							replace_clock = (replace_clock + 1) % F;
						}
					}
				}
				replace_num++;
			}
		}
	}
	printf("MoreClockReplace: \n     the page fault rate of replacement is %6.3f%%", 100 * ((float)replace_num / P));
	printf("\n\n");
}

void Clear() {
	for (int i = 0; i < F; i++) {
		M[i] = -1;
	}
	m_size = 0;
}
//ʹ��λ
bool *useBit;
//�޸�λ
bool *modifiedBit;
//�޸�ҳ������
bool *modifiedPage;
//�������������
int freeBlock=2;
//���������״̬����
int **freeBlockSta;
void fillFront(){
    pPage=0;
    int a=0;
    int b[blockNum];
    for(int i=0;a<blockNum;i++){
        if(i==0){
            blockSta[0][0]=page[pPage];
            b[a]=page[pPage];
            a+=1;
        }else{
            for(int j=0;j<blockNum;j++){
                blockSta[j][pPage]=blockSta[j][pPage-1];
            }
            bool ifFindCommon=false;
            for(int j=0;j<a;j++){
                if(page[pPage]==b[j]){
                    ifFindCommon=true;
                    break;
                }
            }
            if(!ifFindCommon){
                blockSta[a][pPage]=page[pPage];
                b[a]=page[pPage];
                a+=1;
            }
        }
        pPage+=1;
    }
}
void PBA(){
     page=(int*)malloc(pageNum*sizeof(int));
    modifiedPage=(bool*)malloc(sizeof(bool)*pageNum);
    interrupt=(char*)malloc(pageNum*sizeof(char));
    blockSta=(int**)malloc(blockNum*sizeof(int));
    useBit=(bool*)malloc(sizeof(bool)*blockNum);
    modifiedBit=(bool*)malloc(sizeof(bool)*blockNum);
    for(int i=0;i<blockNum;i++){
        blockSta[i]=(int*)malloc(pageNum*sizeof(int));
        useBit[i]=false;
        modifiedBit[i]=false;
    }
    for(int i=0;i<pageNum;i++){
        page[i]=-1;
        interrupt[i]='N';
        modifiedPage[i]=false;
        for(int j=0;j<blockNum;j++){
            blockSta[j][i]=0;
        }
    }
    freeBlockSta=(int**)malloc(sizeof(int)*freeBlock);
    for(int i=0;i<freeBlock;i++){
        freeBlockSta[i]=(int*)malloc(sizeof(int)*pageNum);
    }
    for(int i=0;i<freeBlock;i++){
        for(int j=0;j<pageNum;j++){
            freeBlockSta[i][j]=0;
        }
    }
    //�����ڴ��С����Ϊ64(K)
    memory=64;
    //ҳ������ָ��
    pPage=0;
    //ȱҳ������ʼ��
    lakePage=0;
    //ҳ������ָ���ʼ��
    pPage=0;
    //ȱҳ�жϼ�¼��ʼ��
    for(int i=0;i<pageNum;i++){
        interrupt[i]='N';
    }
    //������������
    fillFront();
    //�����滻��ָ�룺�˴��ĸ���ָ���ԭ����FIFO����ͬ��
    int pReplaceBlock=0;
    //ʹ��λ��ʼ�����˴�����ȱҳ���е���䣩
    for(int i=0;i<blockNum;i++){
        useBit[i]=false;
    }
    //��������������е�ȱҳ��ȫ���滻��ָ�벻�����������ֻص���һ��λ�ã�
    //�ӵ�1ҳ����pPageҳ������blockNum��ȱҳ�ж�
    for(int i=0;i<pPage;i++){
        for(int j=0;j<blockNum;j++){
            if(page[i]==blockSta[j][pPage-1]){
                if(!useBit[j]){
                    useBit[j]=true;
                    interrupt[i]='Y';
                    lakePage+=1;
                }
                break;
            }
        }
    }
    //�����������������һ��ҳ�����һ��ҳ����ʼ����
    for(int i=pPage;i<pageNum;i++){
        //
        for(int j=0;j<freeBlock;j++){
            freeBlockSta[j][i]=freeBlockSta[j][i-1];
        }
        //Ѱ�����е���������Ƿ�洢�˵�ǰ�����ҵ�ҳ��i
        bool findPage = false;
        for(int j=0; j<blockNum; j++){
            if(page[i]==blockSta[j][i-1]){
                findPage=true;
                break;
            }
        }
        //��ǰһ��ҳ������Ӧ�������״̬���Ƶ���ǰҳ������Ӧ�������״̬
        for(int j=0;j<blockNum;j++){
            blockSta[j][i]=blockSta[j][i-1];
        }
        //��������Ѵ�����ͬҳ��
        if(findPage){
            //��һҳ��������״̬���ǵ�ǰҳ��������״̬
            //��һҳ��������״̬�Ѹ��ƣ�ֱ�ӽ�����һҳ�漴��
            continue;
        }
        //������ڲ�������ͬҳ��
        else{
            //�Ƿ��ڻ����������
            int inFreeBLockLocation=-1;
            for(int j=0;j<freeBlock;j++){
                if(page[i]==freeBlockSta[j][i]){
                    inFreeBLockLocation=j;
                    break;
                }
            }
            //�ڻ����������
            if(inFreeBLockLocation!=-1){
                //����ȱҳ
                lakePage+=1;
                interrupt[i]='Y';
                //����������ݽ���
                int temp=blockSta[pReplaceBlock][i];
                blockSta[pReplaceBlock][i]=freeBlockSta[inFreeBLockLocation][i];
                freeBlockSta[inFreeBLockLocation][i]=temp;
                //���滻ָ�����
                pReplaceBlock=(pReplaceBlock+1)%blockNum;
                //����һ������ݷŵ����
                for(int j=inFreeBLockLocation;j<freeBlock-1;j++){
                    freeBlockSta[j][i]=freeBlockSta[j+1][i];
                }
                freeBlockSta[freeBlock-1][i]=temp;
            }
            else{
                int spaceFreeBlock=-1;
                for(int j=0;j<freeBlock;j++){
                    if(freeBlockSta[j][i]==0){
                        spaceFreeBlock=j;
                        break;
                    }
                }
                //�пհ׵Ļ�������飺
                //��ǰ���������հ������
                //��ǰҳ�����뵱ǰ�����
                if(spaceFreeBlock!=-1){
                    //����ȱҳ
                    lakePage+=1;
                    interrupt[i]='Y';
                    //���滻ָ����ָ������������滻
                    freeBlockSta[spaceFreeBlock][i]=blockSta[pReplaceBlock][i];
                    blockSta[pReplaceBlock][i]=page[i];
                    //���滻ָ�����
                    pReplaceBlock=(pReplaceBlock+1)%blockNum;
                }
                //�����ڿհ׵Ļ��������
                //���������������һ��λ�õ������滻�ɵ�ǰ��ҳ�棬ȱҳ����ͬ��
                else{
                    //����ȱҳ
                    lakePage+=1;
                    interrupt[i]='Y';
                    //temp������ǰ���������
                    //��ǰ������������뵱ǰҳ����Ϊ������
                    int temp=blockSta[pReplaceBlock][i];
                    blockSta[pReplaceBlock][i]=page[i];
                    //��������������һ������ݣ�temp�������һ�������
                    for(int j=0;j<freeBlock-1;j++){
                        freeBlockSta[j][i]=freeBlockSta[j+1][i];
                    }
                    freeBlockSta[freeBlock-1][i]=temp;
                    //���滻ָ�����
                    pReplaceBlock=(pReplaceBlock+1)%blockNum;
                }
            }
        }
    }
    printf("PBA(): \n     the page fault rate of replacement is %6.3f%%", 100 * (double)lakePage/pageNum);
	printf("\n\n");
}

int main() {
	CreateRS();
	printf("RS:\n");
	for (int i = 0; i < rs_size; i++)
		printf("%d ", rs[i]);
	printf("\n");
	printf("The size of RS:  %d\n", rs_size);
	printf("\n\n");

	OPT();
	Clear();


	FIFO();
	Clear();

	LRU();
	Clear();

	MoreClock();
	Clear();

	PBA();
	Clear();

	while (1);
}

