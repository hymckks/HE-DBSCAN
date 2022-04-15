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

const int N = 5;            //数据规模
vector<vector<double>> dist;   //全局变量，存放距离判断结果

class EncryptedInputRecord      //为每个数据记录创建一个对象来存储其属性值
{
    public:
        double clusterID;
        double isNoise;
        double notProcess;
        vector<double> neighbors;   
        double validNeighborhoodSize;    //后期需要并行化转化为vector,bool不建议进行vector操作
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

//密文比较，把它交给数据拥有者来做，只返回大小
bool check(Ctxt result)
{
   return true;
}

//返回该Ctxt变量的第i个数的n份拷贝
Ctxt ncopy(int k,Ctxt c,Context &context) //0 <= i <= n-1
{
    // Ctxt c1 = c;
    // rotate(c1,-k);//将c左移i个slot (c[0], ..., example:c[n-1]) = (c[1], c[2], ..., c[n-1], c[0])
    
    // vector<double> v(N);
    // v[0] = 1;
    // PtxtArray pp(context,v);
    // c1 *= pp;        //c 0 0 0 0 0 0 0 
    // Ctxt res = c1;

    // for(int j = 1;j < N;j ++)
    // {
    //     //cout << "===============" << j << endl;
    //     //rotate(c1,1);
    //     res += c;
    // }
    // return res;
    return c;
}

//部分并行化距离计算（二维点,n为输入数据规模,xy没有引用不会影响本来的值）
void ppSEDcalculation(Ctxt x, Ctxt y, Ctxt e,Context &context)
{
    e *= e;//SED
    for(int i = 0;i < N;i ++)
    {
        Ctxt nx = ncopy(i,x,context);
        Ctxt ny = ncopy(i,y,context);
        
        Ctxt sedx = x; Ctxt sedy = y;
        sedx -= nx; sedx *= sedx;     //(x - xi)^2
        sedy -= ny; sedy *= sedy;;
        Ctxt sed = sedx;
        sed += sedy;   //(x - x0) ^ 2 + (y - y0) ^ 2

        //将得到的sed与e^2进行比较，小于则在dist[i][j]存1
        Ctxt result = sed;
        result -= e;

        //密文比较
        check(result);
        cout << "we are now in" << i << endl;
    }
    return ;
}

//用所有的EncryptedElements的各自的属性值来创建SIMD值(分割)
void combineToSIMD(vector<EncryptedInputRecord> EncryptedElements,vector<double> &sClusterIDs,vector<double> &sIsNoise,
                            vector<double> &sNotProcessed,vector<double> &sValidNeighborhoodSize)
{
    for(EncryptedInputRecord elem : EncryptedElements)
    {
        sClusterIDs.push_back(elem.clusterID);
        sIsNoise.push_back(elem.isNoise);
        sNotProcessed.push_back(elem.notProcess);
        sValidNeighborhoodSize.push_back(elem.validNeighborhoodSize);
    }
    return;
}

//将分割成SIMD的值合并
void updateElements(vector<double> &sIsNoise, vector<double> &sNotProcessed,vector<EncryptedInputRecord> EncryptedElements)
{
    int update = 0;
    for(EncryptedInputRecord elem : EncryptedElements)
    {
        elem.isNoise = sIsNoise[update];
        elem.notProcess = sNotProcessed[update];
    }
    return;
}

//隐私保护DBSCAN
void Privacy_preserving_DBSCAN(vector<EncryptedInputRecord> &EncryptedElements,Context &context)
{
    cout << "hello dbscan!!!" << endl;
    int currentClusterID = 0;
    int isCoreElement;
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
        if(elem.validNeighborhoodSize && elem.notProcess) isCoreElement = 1;
        else isCoreElement = 0;

        //判断该点是否为噪声点
        if(elem.notProcess)
        {
            elem.isNoise = !elem.validNeighborhoodSize;
        }
        //处理完成
        elem.notProcess = 0;
    //=========================================================================================
        vector<double> sIsCoreElement(N,isCoreElement);
        PtxtArray psIsCoreElement(context,sIsCoreElement);

        int maxIterations = 4;     //根据论文的实验结果，几个数据集选取的最大迭代值为4，且结果正确

        //用所有的EncryptedElements的各自的属性值来创建SIMD值
        vector<double> sClusterIDs;
        vector<double> sIsNoise;
        vector<double> sNotProcessed;
        vector<double> sValidNeighborhoodSize;
    //=========================================================================================
    //此处应多为SIMD操作    
        for(int i = 0;i < maxIterations;i ++)
        {
            //用所有的EncryptedElements的各自的属性值来创建SIMD值
            combineToSIMD(EncryptedElements,sClusterIDs,sIsNoise,sNotProcessed,sValidNeighborhoodSize);
            
            //PtxtArray具有并行化操作,所以将SIMD的值转化为怪形式
            PtxtArray psIsNoise(context,sIsNoise);
            PtxtArray psNotProcessed(context,sNotProcessed);
            PtxtArray psValidNeighborhoodSize(context,sValidNeighborhoodSize);   //不变的
            PtxtArray pneighbors(context,elem.neighbors);                        //不变的                               
            PtxtArray psNeighborsAreCoreElement(context);
            PtxtArray pkneighbors(context);

            //并行化判断，若新的cluster被创建，则未加入cluster的邻居也被加入cluster
            PtxtArray psValidNeighbor = psIsNoise;
            psValidNeighbor += sNotProcessed;    //用加法代替||操作
            psValidNeighbor *= psIsCoreElement; //用乘法代替&&操作
            psValidNeighbor *= pneighbors;

            vector<double> sValidNeighbor;
            psValidNeighbor.store(sValidNeighbor);

            if(accumulate(sValidNeighbor.begin(),sValidNeighbor.end(),0) != 0)
            {   
                //存在满足条件的邻居，将满足条件的邻居的notprocessed 与isnoise置为0（将点的validNeighbor为1的置为0）,以及对应ID
                psNotProcessed.store(sNotProcessed);
                psIsNoise.store(sIsNoise);
                for(int ii = 0;ii < N;ii ++)
                {
                    if(sValidNeighbor[ii] != 0) 
                    {
                        sNotProcessed[ii] = 0;
                        sIsNoise[ii] = 0;
                        sClusterIDs[ii] = currentClusterID;                    
                    }
                }  
                psNotProcessed.load(sNotProcessed);
                psIsNoise.load(sIsNoise);

                //找出既是邻居又同时是核心点的点
                psNeighborsAreCoreElement = psValidNeighbor;
                psNeighborsAreCoreElement *= psValidNeighborhoodSize;
            }
            else 
            {
                psNeighborsAreCoreElement.load(0);
            }

            //如果邻居本身也是核心点，则把邻居的邻居也加入进来
            for(int k = 0;k < N;k ++)
            {

                    pkneighbors.load(EncryptedElements[k].neighbors);//如果有旧数据会不会影响结果
                    PtxtArray psUpdateResultDistanceComp = psNeighborsAreCoreElement;
                    psUpdateResultDistanceComp *= pkneighbors;

                    //ORTREE操作
                    vector <double>sUpdateResultDistanceComp;
                    psUpdateResultDistanceComp.store(sUpdateResultDistanceComp);
                    if(accumulate(sValidNeighbor.begin(),sValidNeighbor.end(),0) != 0)
                    {
                        pkneighbors += psUpdateResultDistanceComp;
                    }
                    pkneighbors.store(EncryptedElements[k].neighbors);        //改变了邻居表的值
            }

            //将各SIMD值合并并更新所有的EncryptedInputRecord
            psIsNoise.store(sIsNoise);
            psNotProcessed.store(sNotProcessed);
            updateElements(sIsNoise, sNotProcessed, EncryptedElements);
        }
        //更新ClusterIDs
        int update = 0;
        for(EncryptedInputRecord elem : EncryptedElements)
        {
            elem.clusterID = sClusterIDs[update];
            update++;
        }
    }
    currentClusterID += isCoreElement;

    return;
}

int main(int argc, char* argv[])
{
    //初始化，由于slot的数量n是由m给定的，因此不同规模的数据集应选用不同的n，参考推荐参数表
    Context context = ContextBuilder<CKKS>().m(262144).bits(2234).precision(30).c(2).build();//选取了最大的m以获得足够的n slots
    //bits的选择上主要是为了同台计算的深度，由于数据规模较大
    SecKey secretKey(context);
    secretKey.GenSecKey();
    const PubKey &publicKey = secretKey;

    //读入文件
    vector<double> x0, y0;
    openFile("Lsun.txt",x0,y0);
    
    //参数读入
    double minPts = 3;//参数1，邻居个数         以明文的形式和攒底给处理方
    //cout << "please enter the minPts : ";
    //cin >> minPts;
    double e = 0.2;//参数2，距离          数据拥有着持有，输入后进行加密操作
    //cout << "please enter the e : ";
    //cin >> e;
    
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
//=================================================================================================================
    //距离计算
    ppSEDcalculation(x, y, ce, context);
    
    cout << "done!==========" << endl;
    //对于每个输入的数据记录创建一个EncryptedInputRecord,并进行初始化
    vector<EncryptedInputRecord> EncryptedElements;

    cout << "begin!!" << endl;
    for(int i = 0;i < N;i ++)
    {
        EncryptedElements[i].clusterID = 0;
        EncryptedElements[i].isNoise = 0;
        EncryptedElements[i].notProcess = 1;
        cout << "start create : " << i << endl;
        for(int j = 0;i < N;j ++) EncryptedElements[i].neighbors.push_back(dist[i][j]);
        
        //计算邻居数目
        double neighborcount = accumulate(EncryptedElements[i].neighbors.begin(),
                                                    EncryptedElements[i].neighbors.end(),0);
        if(neighborcount >= minPts) EncryptedElements[i].validNeighborhoodSize = 1;
        else EncryptedElements[i].validNeighborhoodSize = 0;
    }

    cout << "create success !!!!!!!!!!!" << endl;
    //开始执行隐私保护DBSCAN协议
    Privacy_preserving_DBSCAN(EncryptedElements,context);//可能有点问题

    //归纳结果========================================================================================================
    vector<int> resclusterID;
    int duibi = EncryptedElements[0].clusterID;
    resclusterID.push_back(EncryptedElements[0].clusterID);
    for(int i = 1;i < EncryptedElements.size();i ++)
    {
        if(EncryptedElements[i].clusterID != duibi)
        {
            duibi = EncryptedElements[i].clusterID;
            resclusterID.push_back(EncryptedElements[i].clusterID);
        }
        else
            continue;
    }
    int numcluster = resclusterID.size();//聚类的数量
    cout << "the number of the cluster is : " << numcluster << endl;
    
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