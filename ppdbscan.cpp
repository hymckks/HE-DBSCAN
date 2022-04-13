#include <iostream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <helib/helib.h>
using namespace std;
using namespace helib;

//-----------------------------=优化空间：本项目仅仅展示了二维点数据，其维度可以拓展--------------------------------------------------

int N = 400;            //数据规模
vector<vector<double>> dist;   //全局变量，存放距离判断结果

class EncryptedInputRecord      //为每个数据记录创建一个对象来存储其属性值
{
    public:
        int clusterID;
        bool isNoise;
        bool notProcess;
        vector<double> neighbors;   
        bool validNeighborhoodSize;//需要根据dist处理一下

};

//读文件
vector<EncryptedInputRecord> openFile(const char* dataset){
	fstream file;
	file.open(dataset,ios::in);    //dataset在main函数中指定文件名
	if(!file) 
    {
        cout <<"Open File Failed!" <<endl;
    } 

	file.close();
	cout<<"successful!"<<endl;
}

//部分并行化距离计算（二维点,n为输入数据规模,xy没有引用不会影响本来的值）
void ppSEDcalculation(Ctxt x, Ctxt y, Ctxt e, int n,PubKey publicKey,vector<Ctxt> nx,vector<Ctxt> ny)
{
    e *= e;
    for(int i = 0;i < N;i ++)
    {
        //写一个数组在单独输入的时候就已经把n份拷贝完成了
        x -= nx[i]; x *= x;     //(x - xi)^2
        y -= ny[i]; y *= y;

        Ctxt sed = x;
        sed += y;   //(x - x0) ^ 2 + (y - y0) ^ 2

        //将得到的sed与e^2进行比较，小于则在dist[i][j]存1
        Ctxt result = sed;
        result -= e;
        for(int j = 0;j < n;j ++)//if result[i] > 0 l;dist[i][j] = 0;else dist[i][j] = 1;
        {
            if(check())            //如何判断
            {
                dist[i][j] = 0;
            }                  
            else dist[i][j] = 1;              
        }        
    }
    return ;
}

void Privacy_preserving_DBSCAN(vector<EncryptedInputRecord> &EncryptedElements)
{
    int currentClusterID = 0;
    for(EncryptedInputRecord elem : EncryptedElements)
    {
        //判断该点是否被处理过
        if(elem.notProcess)
        {
            if(elem.validNeighborhoodSize)
            {
                elem.clusterID = currentClusterID;
            }
            else elem.clusterID = 0;
        }

        //判断是否为核心点
        bool isCoreElement;
        if(elem.validNeighborhoodSize && elem.notProcess) isCoreElement = 1;

        //判断该点是否为噪声点
        if(elem.notProcess)
        {
            elem.isNoise = !elem.validNeighborhoodSize;
        }
        //处理完成
        elem.notProcess = 0;
    //=========================================================================================

    }
    
}

int main(int argc, char* argv[])
{
  //初始化，由于slot的数量n是由m给定的，因此不同规模的数据集应选用不同的n，参考推荐参数表
    Context context = ContextBuilder<CKKS>().m(16384).bits(119).precision(30).c(2).build();//选取了最大的m以获得足够的n slots
    SecKey secretKey(context);
    secretKey.GenSecKey();
    const PubKey &publicKey = secretKey;
    

    //读入文件
     = openFile("Lsun.txt");
    vector<Ctxt> nx(N);  //n copies of x[i]
    vector<Ctxt> ny(N);

    //坐标向量的密文形式
    vector<double> x0(N), y0(N);
    PtxtArray x1(context,x0),y1(context,y0);
    Ctxt x(publicKey),y(publicKey);
    x1.encrypt(x);
    y1.encrypt(y);

    //参数e的密文的n个拷贝
    int e;
    vector<double> e0(N,e);
    PtxtArray pe(context,e0);
    Ctxt ce(publicKey);
    pe.encrypt(ce);

    return 0;
}

//=====================推荐参数表=========================    
/*       m	   bits	c
    	16384	119	2
    	32768	358	6
    	32768	299	3
    	32768	239	2
    	65536	725	8
    	65536	717	6
    	65536	669	4
    	65536	613	3
    	65536	558	2
    	131072	1445	8
    	131072	1435	6
    	131072	1387	5
    	131072	1329	4
    	131072	1255	3
    	131072	1098	2
    	262144	2940	8
    	262144	2870	6
    	262144	2763	5
    	262144	2646	4
    	262144	2511	3
    	262144	2234	2  */
