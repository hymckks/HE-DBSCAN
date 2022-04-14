#include <iostream>
#include <vector>
#include <cstdio>
#include <string>
#include <cstring>
#include <cmath>
#include <ctime>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <helib/helib.h>
using namespace std;
using namespace helib;

//-----------------------------=优化空间：本项目仅仅展示了二维点数据，其维度可以拓展--------------------------------------------------

const int N = 400;            //数据规模
vector<vector<double>> dist;   //全局变量，存放距离判断结果

class EncryptedInputRecord      //为每个数据记录创建一个对象来存储其属性值
{
    public:
        int clusterID;
        bool isNoise;
        bool notProcess;
        vector<double> neighbors;   
        bool validNeighborhoodSize;
};

//读文件，分别将数据的x坐标与y坐标读入两个向量中
void openFile(const char* dataset, vector<double> &x0, vector<double> &y0){
	fstream infile;
	infile.open(dataset,ios::in);    //dataset在main函数中指定文件名
	if(!infile.is_open()) 
    {
        cout <<"Open File Failed!" <<endl;
    } 
    
    string temp,ycut;
    while(getline(infile,temp))
    {
       x0.push_back(stod(temp));
       int split = temp.find(';',0) + 1;
       ycut = temp.substr(split);
       y0.push_back(stod(ycut));
    }
	infile.close();
	cout<<"successful!"<<endl;
    return;
}

//返回该Ctxt变量的第i个数的n份拷贝
Ctxt ncopy(int i,Ctxt c) //0 <= i <= n-1
{
    rotate(c,-i);//将c左移i个slot (c[0], ..., example:c[n-1]) = (c[1], c[2], ..., c[n-1], c[0])
    shift(c,N - 1 - i);              //   (0,0,0,0,0,0,....0,c[i])
    Ctxt res = c;
    for(int j = 1;j < N;j ++)
    {
        shift(c,-1);
        res += c;
    }
    return res;
}

//部分并行化距离计算（二维点,n为输入数据规模,xy没有引用不会影响本来的值）
void ppSEDcalculation(Ctxt x, Ctxt y, Ctxt e)
{
    e *= e;//SED
    for(int i = 0;i < N;i ++)
    {
        Ctxt nx = ncopy(i,x);
        Ctxt ny = ncopy(i,y);
        x -= nx; x *= x;     //(x - xi)^2
        y -= ny; y *= y;

        Ctxt sed = x;
        sed += y;   //(x - x0) ^ 2 + (y - y0) ^ 2

        //将得到的sed与e^2进行比较，小于则在dist[i][j]存1
        Ctxt result = sed;
        result -= e;

        for(int j = 0;j < N;j ++)//if result[i] > 0 ;dist[i][j] = 0;else dist[i][j] = 1;
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

//隐私保护DBSCAN
void Privacy_preserving_DBSCAN(vector<EncryptedInputRecord> &EncryptedElements)
{
    int currentClusterID = 0;
    for(EncryptedInputRecord &elem : EncryptedElements)     //引用条件稍微斟酌一下
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
    vector<double> x0, y0;
    openFile("Lsun.txt",x0,y0);
    
    //参数读入
    int minPts;//参数1，邻居个数         以明文的形式和攒底给处理方
    cout << "please enter the minPts : ";
    cin >> minPts;
    int e;//参数2，距离          数据拥有着持有，输入后进行加密操作
    cout << "please enter the minPts : ";
    cin >> e;
    
    //坐标向量x0,y0的密文形式
    PtxtArray x1(context,x0),y1(context,y0);
    Ctxt x(publicKey),y(publicKey);
    x1.encrypt(x);
    y1.encrypt(y);

    //参数e的密文的n个拷贝
    vector<double> e0(N,e);
    PtxtArray pe(context,e0);
    Ctxt ce(publicKey);
    pe.encrypt(ce);
//==========================================================================================
    //距离计算
    ppSEDcalculation(x, y, ce);
    
    //对于每个输入的数据记录创建一个EncryptedInputRecord,并进行初始化
    vector<EncryptedInputRecord> EncryptedElements;
    for(int i = 0;i < N;i ++)
    {
        EncryptedElements[i].clusterID = 0;
        EncryptedElements[i].isNoise = 0;
        EncryptedElements[i].notProcess = 1;

        for(int j = 0;i < N;j ++) EncryptedElements[i].neighbors.push_back(dist[i][j]);

        double neighborcount = accumulate(EncryptedElements[i].neighbors.begin(),
                                                    EncryptedElements[i].neighbors.end(),0);
        if(neighborcount >= minPts) EncryptedElements[i].validNeighborhoodSize = 1;
        else EncryptedElements[i].validNeighborhoodSize = 0;
    }

    //开始执行隐私保护DBSCAN协议
    Privacy_preserving_DBSCAN(EncryptedElements);

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