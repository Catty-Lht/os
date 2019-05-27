#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#define P 32
#define F 8
#define RS_MAX_SIZE 32

//p：工作集起始位置
//e：页数
//m：工作集移动率
//t：比较值

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
//页面序列总长度、工作集的起始位置p、物理块个数
int pageNum=32,pageStart=1,blockNum=3;
//物理块状态数组
int **blockSta;
//页面序列数组
int *page;
//页面序列指针
int pPage;
//缺页中断数组
char *interrupt;
//缺页数量
int lakePage;
//虚拟内存N
int memory;
//工作集中包含的页数e
int everyWorkSet;
//工作集移动率m
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
Achieve:最佳置换算法
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
Achieve:随机置换算法（Random permutation）
当内存满载，随机选择一个物理块替换
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
Achieve:先进先出法（Fisrt In First Out）
如果一个数据最先进入缓存中，则应该最早被淘汰
每次替换最先进入内存的页面
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
Achieve: 最近最久未使用（Least Recently Used）
如果一个数据在最近一段时间没有被访问到，那么在将来被访问的可能性也很小
淘汰最长时间未被使用的页面
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
Achieve: 简单clock算法
页面被访问后access_bit[j]置1；若当前访问位access_bit[j]为0，则将其置出；若当前访问位access_bit[j]为1，则置为0
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
Achieve: 改进Clock算法
访问位access，修改位modify，通过至多四轮扫描找出最适合淘汰的页面
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
				oncemore:	if (access_bit[replace_clock] == 0 && modify_bit[replace_clock] == 0) {//扫描循环队列，寻找A=0且M=0的第一类页面
						M[replace_clock] = rs[i];
						access_bit[replace_clock] = 0;
						temp = 0;
					}
					else {
						if (flag) {
							if (access_bit[replace_clock] == 0 && modify_bit[replace_clock] == 1) {//扫描循环队列，寻找A=0且M=0的第一类页面
								M[replace_clock] = rs[i];
								access_bit[replace_clock] = 0;
								temp = 0;
							}
							else {
								access_bit[replace_clock] = 0;
								if (replace_clock + 1 == F) {//如果第二步也失败，即未找到第二类页面，则将指针返回到开始的位置，并将所有的访问位复0
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
							if (replace_clock + 1 == F)//如果还没有扫描一周
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
//使用位
bool *useBit;
//修改位
bool *modifiedBit;
//修改页面序列
bool *modifiedPage;
//空闲物理块数量
int freeBlock=2;
//空闲物理块状态数组
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
    //虚拟内存大小设置为64(K)
    memory=64;
    //页面序列指针
    pPage=0;
    //缺页数量初始化
    lakePage=0;
    //页面序列指针初始化
    pPage=0;
    //缺页中断记录初始化
    for(int i=0;i<pageNum;i++){
        interrupt[i]='N';
    }
    //将物理块填充满
    fillFront();
    //设置替换块指针：此处的更替指针的原理与FIFO是相同的
    int pReplaceBlock=0;
    //使用位初始化（此处用作缺页数列的填充）
    for(int i=0;i<blockNum;i++){
        useBit[i]=false;
    }
    //将物理块填充过程中的缺页补全，替换块指针不动（填充完后又回到第一个位置）
    //从第1页到第pPage页共产生blockNum个缺页中断
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
    //从能填充满物理块的那一个页面的下一个页面起开始遍历
    for(int i=pPage;i<pageNum;i++){
        //
        for(int j=0;j<freeBlock;j++){
            freeBlockSta[j][i]=freeBlockSta[j][i-1];
        }
        //寻找所有的物理块内是否存储了当前所查找的页面i
        bool findPage = false;
        for(int j=0; j<blockNum; j++){
            if(page[i]==blockSta[j][i-1]){
                findPage=true;
                break;
            }
        }
        //将前一个页面所对应的物理块状态复制到当前页面所对应的物理块状态
        for(int j=0;j<blockNum;j++){
            blockSta[j][i]=blockSta[j][i-1];
        }
        //物理块内已存在相同页面
        if(findPage){
            //上一页面的物理块状态就是当前页面的物理块状态
            //上一页面的物理块状态已复制，直接进行下一页面即可
            continue;
        }
        //物理块内不存在相同页面
        else{
            //是否在缓冲物理块内
            int inFreeBLockLocation=-1;
            for(int j=0;j<freeBlock;j++){
                if(page[i]==freeBlockSta[j][i]){
                    inFreeBLockLocation=j;
                    break;
                }
            }
            //在缓冲物理块内
            if(inFreeBLockLocation!=-1){
                //产生缺页
                lakePage+=1;
                interrupt[i]='Y';
                //将两块的内容交换
                int temp=blockSta[pReplaceBlock][i];
                blockSta[pReplaceBlock][i]=freeBlockSta[inFreeBLockLocation][i];
                freeBlockSta[inFreeBLockLocation][i]=temp;
                //将替换指针后移
                pReplaceBlock=(pReplaceBlock+1)%blockNum;
                //将这一块的内容放到最后
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
                //有空白的缓冲物理块：
                //当前物理块填入空白物理块
                //当前页面填入当前物理块
                if(spaceFreeBlock!=-1){
                    //产生缺页
                    lakePage+=1;
                    interrupt[i]='Y';
                    //将替换指针所指向的物理块进行替换
                    freeBlockSta[spaceFreeBlock][i]=blockSta[pReplaceBlock][i];
                    blockSta[pReplaceBlock][i]=page[i];
                    //将替换指针后移
                    pReplaceBlock=(pReplaceBlock+1)%blockNum;
                }
                //不存在空白的缓冲物理块
                //将空闲物理块的最后一个位置的内容替换成当前的页面，缺页操作同上
                else{
                    //产生缺页
                    lakePage+=1;
                    interrupt[i]='Y';
                    //temp保留当前物理块内容
                    //当前工作物理块填入当前页面作为新内容
                    int temp=blockSta[pReplaceBlock][i];
                    blockSta[pReplaceBlock][i]=page[i];
                    //抛弃空闲物理块第一块的内容，temp放入最后一块的内容
                    for(int j=0;j<freeBlock-1;j++){
                        freeBlockSta[j][i]=freeBlockSta[j+1][i];
                    }
                    freeBlockSta[freeBlock-1][i]=temp;
                    //将替换指针后移
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

