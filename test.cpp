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

const int N = 6;            //数据规模
vector<vector<double>> dist(N, vector<double>(N));   //全局变量，存放距离判断结果
vector<vector<double>> ncopyx;
vector<vector<double>> ncopyy;    //存放拷贝


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
    int i;
    while(getline(infile,temp))
    {
       x0.push_back(stod(temp));
       vector<double> xx(N,stod(temp));
       int split = temp.find(';',0) + 1;
       ycut = temp.substr(split);
       y0.push_back(stod(ycut));
       vector<double> yy(N,stod(ycut));
       ncopyx.push_back(xx);
       ncopyy.push_back(yy);
    }
	infile.close();
	cout<<"Reading the dataset successful!!"<<endl;
    return;
}

//返回该Ctxt变量的第i个数的n份拷贝
// Ctxt ncopy(int k,Ctxt c,Context &context) //0 <= i <= n-1
// {
//     Ctxt c1 = c;
//     rotate(c1,-k);//将c左移i个slot (c[0], ..., example:c[n-1]) = (c[1], c[2], ..., c[n-1], c[0])
    
//     vector<double> v(N);
//     v[0] = 1;
//     PtxtArray pp(context,v);
//     c1 *= pp;        //c 0 0 0 0 0 0 0 
//     Ctxt res = c1;

//     for(int j = 1;j < N;j ++)
//     {
//         //cout << "===============" << j << endl;
//         //rotate(c1,1);
//         res += c;
//     }
//     return res;
//     return c;
// }

//密文比较，把它交给数据拥有者来做，只返回大小
void check(Ctxt sed,double e,SecKey secretKey,Context &context,int i)
{
    vector<double> d;
    vector<double> v;
    PtxtArray p(context); 
    p.decrypt(sed,secretKey);
    p.store(v);
    e *= e;

    for(int i = 0;i < N;i ++)
    {
        if(v[i] <= e) d.push_back(1);
        else d.push_back(0);
    }
    dist[i].assign(d.begin(),d.end());
}

//部分并行化距离计算（二维点,n为输入数据规模,xy没有引用不会影响本来的值）  //在通信过程中不会将知晓参数私钥
void ppSEDcalculation(Ctxt x, Ctxt y,double e, Context &context,vector<Ctxt> nx,vector<Ctxt> ny, SecKey secretKey)
{
    cout << "================= starting to calculate!! ====================" << endl;
    //e *= e;//SED e^2
    cout << "dist : " << endl;
    for(int i = 0;i < N;i ++)
    {   
        Ctxt sedx = x; Ctxt sedy = y;
        sedx -= nx[i]; sedx *= sedx;     //(x - xi)^2
        sedy -= ny[i]; sedy *= sedy;;
        Ctxt sed = sedx;
        sed += sedy;   //(x - x0) ^ 2 + (y - y0) ^ 2

        //将得到的sed与e^2进行比较，小于则在dist[i][j]存1
        check(sed,e, secretKey,context,i);

        //查看一下距离计算的结果
        for(int j = 0;j < N;j ++)
        {
            cout << dist[i][j] << ' ';
        }
        cout << endl;
    }
    return ;
}

//用所有的EncryptedElements的各自的属性值来创建SIMD值(分割)
void combineToSIMD(vector<vector <Ctxt>> cEncryptedElements,Ctxt &sClusterIDs,Ctxt &sIsNoise,
                            Ctxt &sNotProcessed,Ctxt &sValidNeighborhoodSize,Context &context,SecKey secretKey)
{
    PtxtArray p(context);
    for(int k = 0;k <N;k ++)
    {
        vector<double> v(N,0);
        v[k] = 1;
        p.load(v);
        cEncryptedElements[k][0] *= p;
        Ctxt temp1 = sClusterIDs;
        temp1 += cEncryptedElements[k][0];
        sClusterIDs = temp1;

        cEncryptedElements[k][1] *= p;
        Ctxt temp2 = sIsNoise;
        temp2 += cEncryptedElements[k][1];
        sIsNoise = temp2;

        cEncryptedElements[k][2] *= p;
        Ctxt temp3 = sNotProcessed;
        temp3 += cEncryptedElements[k][2];
        sNotProcessed = temp3;

        cEncryptedElements[k][4] *= p;
        Ctxt temp4 = sValidNeighborhoodSize;
        temp4 += cEncryptedElements[k][4];
        sValidNeighborhoodSize = temp4;


    }
    return;
}

bool checkCtxtTrueOrFalse(Ctxt a, Ctxt c1, Context &context,SecKey secretKey)
{
    PtxtArray p1(context);
    p1.decrypt(a,secretKey);
    PtxtArray p2(context);
    p2.decrypt(c1,secretKey);
    vector<double> v1, v2;
    p1.store(v1);
    p2.store(v2);
    if(v1[0] > 1 && v2[0] == 1) return 1;
    if(v2[0] - v1[0] > 0) 
    {
        if(v2[0] - v1[0] <= 0.01) 
        {
            cout << "equal!!" << endl; 
            return 1;
        }
        else 
        {
            cout <<"not equal"<<endl;
            return 0;
        }
    }
    else 
    {
        if(v1[0] - v2[0] <= 0.01) 
        {
            cout << "equal!! " << endl;
            return 1;
        }
        else
        {
            cout <<"not equal"<<endl;
            return 0; 
        }
    }
}

//将分割成SIMD的值合并
void updateElements(Ctxt sIsNoise, Ctxt sNotProcessed,vector<vector <Ctxt>> &cEncryptedElements,
            Context &context,SecKey secretKey,Ctxt &sisCoreelmet)
{
    vector<double> v1(N,0);
    PtxtArray p1(context);
    p1.decrypt(sIsNoise,secretKey);
    p1.store(v1);

    vector<double> v2(N,0);
    PtxtArray p2(context);
    p2.decrypt(sNotProcessed,secretKey);
    p2.store(v2);

    vector<double> v5(N,0);
    PtxtArray p5(context);
    p5.decrypt(sisCoreelmet,secretKey);
    p5.store(v5);

    int update = 0;
    for(int i = 0;i < N;i ++)
        {
            if(v1[update] < 0) v1[update] = -v1[update];
            vector<double> v3(N,v1[update]);
            PtxtArray p3(context,v3);
            p3.encrypt(cEncryptedElements[i][1]);

            if(v2[update] < 0) v2[update] = -v2[update];
            vector<double> v4(N,v2[update]);
            PtxtArray p4(context,v4);
            p4.encrypt(cEncryptedElements[i][2]);

            if(v5[update] < 0) v5[update] = -v5[update];
            vector<double> v6(N,v5[update]);
            PtxtArray p6(context,v5);
            p6.encrypt(sisCoreelmet);

            update ++;
        }
    return;
}

//更新clusterid情况  交给用户来做
void updatecluster(vector<vector <Ctxt>> &cEncryptedElements, Ctxt sClusterIDs, Ctxt c1, Ctxt cc1, 
                        SecKey secretKey, Context &context)
{
    //更新ClusterIDs
    vector<double> v(N,0);
    PtxtArray p(context);
    p.decrypt(sClusterIDs,secretKey);
    p.store(v);
    for(int i = 0;i < N;i ++)
        {
            if(checkCtxtTrueOrFalse(cEncryptedElements[i][1],c1,context,secretKey)) 
            {
                cout << "update :sure is a noise!" << endl; 
                cEncryptedElements[i][0] = cc1;
            }
            else 
            {
                cout << "update clusterID " << endl;
                if(v[i] < 0) v[i] = -v[i];
                vector<double> v1(N,v[i]);//update
                PtxtArray p1(context,v1);
                 p1.encrypt(cEncryptedElements[i][0]);
            }
        }
    return ;
}

void operationofline24to28(Ctxt sssValidNeighbor,Ctxt &sClusterIDs, Ctxt &sNotProcessed, Ctxt &sIsNoise, 
           Ctxt ccurrentClusterID, SecKey secretKey,Context &context)
{
    vector<double> v1(N,0);
    PtxtArray p1(context);
    p1.decrypt(sssValidNeighbor,secretKey);
    p1.store(v1);//sValidNeighbor明文

    vector<double> v2(N,0);
    PtxtArray p2(context);
    p2.decrypt(sClusterIDs,secretKey);
    p2.store(v2);//sClusterIDs明文

    vector<double> v3(N,0);
    PtxtArray p3(context);
    p3.decrypt(sNotProcessed,secretKey);
    p3.store(v3);//sNotProcessed明文

    vector<double> v4(N,0);
    PtxtArray p4(context);
    p4.decrypt(sIsNoise,secretKey);
    p4.store(v4);//sIsNoise,明文

    vector<double> v5(N,0);
    PtxtArray p5(context);
    p5.decrypt(ccurrentClusterID,secretKey);
    p5.store(v5);//ccurrentCluster明文

    for(int i = 0;i < N;i ++)
    {
        if(v1[i] < 0) v1[i] = -v1[i];
        if(v1[i] >= 1) 
        {
            if(v5[i] < 0) 
                v2[i] = -v5[i];
            else 
                v2[i] = v5[i];
            v3[i] = 0;
            v4[i] = 0;
        }
    }
    
    p2.load(v2);
    p3.load(v3);
    p4.load(v4);

    p2.encrypt(sClusterIDs);
    p3.encrypt(sNotProcessed);
    p4.encrypt(sIsNoise);

}

//
bool checkORTREE(Ctxt a, Context &context,SecKey secretKey)
{
    PtxtArray p1(context);
    p1.decrypt(a,secretKey);
    // PtxtArray p2(context);
    // p2.decrypt(c1,secretKey);
    vector<double> v1;
    p1.store(v1);
    // p2.store(v2);
    for(int i = 0;i < N;i ++)
    {
        if(v1[i] < 0) v1[i] = -v1[i];
        if(v1[i] >= 1) return true;
        // if(v1[i] - v2[i] > 0)
        // {
        //     if(v1[i] - v2[i] <= 0.01) return true;
        // }
        // else
        //     if(v2[i] - v1[i] <= 0.01) return true;
    }
    return false;
}

//纠正邻居矩阵
void correctneighbor(vector<vector <Ctxt>> &cEncryptedElements,Context &context,SecKey secretKey)
{
    for(int i = 0;i < N;i ++)
    {
        PtxtArray p(context);
        vector<double> v;
        p.decrypt(cEncryptedElements[i][3],secretKey);
        p.store(v);
        for(int j = 0;j < N;j ++)
        {
            if(v[j] < 0) v[j] = -v[j];
            if(v[j] >= 1) v[j] = 1;
        }
        p.load(v);
        p.encrypt(cEncryptedElements[i][3]);
    }
    return;
}

//隐私保护DBSCAN
void Privacy_preserving_DBSCAN(vector<vector <Ctxt>> &cEncryptedElements,
Ctxt ccurrentClusterID, Ctxt cisCoreElement, Ctxt c0, Ctxt c1, Ctxt cc1, SecKey secretKey,Context &context)
{
    cout << "========================= hello dbscan!!! ========================" << endl;
    for(int elem = 0;elem < N;elem ++)     //引用条件稍微斟酌一下
    {   
        cout << endl;
        cout << "hello No. " << elem + 1 << endl;
        //判断该点是否被处理过
        if(checkCtxtTrueOrFalse(cEncryptedElements[elem][2],c1,context,secretKey))
        {
            cout << "The elem is not processed!!!" <<endl;
            Ctxt c00 = c0;
            if(checkCtxtTrueOrFalse(cEncryptedElements[elem][4],c1,context,secretKey))
            {
                cout << "The elem is coreelemt" << endl;
                cEncryptedElements[elem][0] = ccurrentClusterID;//1
            }
            else 
            {
                cEncryptedElements[elem][0] = c00;
                cout << "The elem has not enough validneighbor" << endl;
            }
        }

        //判断是否为核心点
        Ctxt temp1 = cEncryptedElements[elem][4];
        temp1 *= cEncryptedElements[elem][2];

        if(checkCtxtTrueOrFalse(temp1,c1,context,secretKey))
        {Ctxt c11 = c1; cisCoreElement = c11;cout << "It's definatlly  core" << endl;}
        else {Ctxt c00 = c0;cisCoreElement = c00;}

        //判断该点是否为噪声点
        if(checkCtxtTrueOrFalse(cEncryptedElements[elem][2],c1,context,secretKey))
        {
            cout << "Not processed maybe is a noise" << endl;
            if(checkCtxtTrueOrFalse(cEncryptedElements[elem][4],c1,context,secretKey))
            {
                cout << "it's not a noise!" << endl; 
                Ctxt c00 = c0;
                cEncryptedElements[elem][1] = c00;
            }
            else
            {  
                cout << "it's a noise!" << endl;
                Ctxt c11 = c1;
                cEncryptedElements[elem][1]= c11;
            }
        }

        //处理完成
        Ctxt c00 = c0;
        cEncryptedElements[elem][2] = c00;

    //=========================================================================================
       int maxIterations = 1;     //根据论文的实验结果，几个数据集选取的最大迭代值为4，且结果正确

        //用所有的EncryptedElements的各自的属性值来创建SIMD值
        Ctxt sClusterIDs = c0;
        Ctxt sIsNoise = c0;
        Ctxt sNotProcessed = c0;
        Ctxt sValidNeighborhoodSize = c0;
    //=========================================================================================
    //此处应多为SIMD操作    
        for(int i = 0;i < maxIterations;i ++)
        {
            //用所有的EncryptedElements的各自的属性值来创建SIMD值
            combineToSIMD(cEncryptedElements,sClusterIDs,sIsNoise,sNotProcessed,sValidNeighborhoodSize,context,secretKey);
            Ctxt sValidNeighbor = sIsNoise;
            sValidNeighbor += sNotProcessed;    //用加法代替||操作

            Ctxt ssValidNeighbor = sValidNeighbor;
            ssValidNeighbor *= cisCoreElement;//用乘法代替&&操作
            
            Ctxt sssValidNeighbor = ssValidNeighbor;
            sssValidNeighbor *= cEncryptedElements[elem][3];

            Ctxt sNeighborsAreCoreElement = c0;

            if(checkORTREE(sssValidNeighbor,context,secretKey))
            {   
                operationofline24to28(sssValidNeighbor, sClusterIDs, sNotProcessed, sIsNoise, ccurrentClusterID, secretKey,context);
                sNeighborsAreCoreElement = sssValidNeighbor;
                sNeighborsAreCoreElement *= sValidNeighborhoodSize;
            }
            else 
            {
                sNeighborsAreCoreElement = c0;
            }
           
            //如果邻居本身也是核心点，则把邻居的邻居也加入进来
            vector<double> v(N,0);
            for(int k = 0;k < N;k ++)
            {
                Ctxt sUpdateResultDistanceComp = sNeighborsAreCoreElement;
                sUpdateResultDistanceComp *= cEncryptedElements[k][3];
                correctneighbor(cEncryptedElements,context,secretKey);

                vector<double> xxx;
                PtxtArray xxxx(context);
                xxxx.decrypt( sUpdateResultDistanceComp,secretKey);
                xxxx.store(xxx);
                for(int ll = 0;ll < N;ll ++)
                {
                    cout << xxx[ll] << ' ';
                }
                cout << endl;
                
                //ORTREE操作
                //if(sUpdateResultDistanceComp != Approx(0))
                if(checkORTREE(sUpdateResultDistanceComp,context,secretKey))
                        v[k] = 1;
            }
            PtxtArray p(context,v);
            correctneighbor(cEncryptedElements,context,secretKey);

            Ctxt cEnc = cEncryptedElements[elem][3];
            cEnc += p;
            cEncryptedElements[elem][3] = cEnc;

            //将各SIMD值合并并更新所有的EncryptedInputRecord
            updateElements(sIsNoise, sNotProcessed, cEncryptedElements, context, secretKey, cisCoreElement);
        }
        
        correctneighbor(cEncryptedElements,context,secretKey);
        updatecluster(cEncryptedElements, sClusterIDs, c1, cc1, secretKey, context);

        correctneighbor(cEncryptedElements,context,secretKey);
        Ctxt ID = ccurrentClusterID;
        ID += cisCoreElement;
        ccurrentClusterID = ID;

        vector<double> aaaa;
        PtxtArray AAA(context);
        AAA.decrypt(ccurrentClusterID,secretKey);
        AAA.store(aaaa);
        cout<< "ccurrentClusterID : " << aaaa[0] << endl;
        cout << endl;
    }  
    return;
}

int main(int argc, char* argv[])
{
    //初始化，由于slot的数量n是由m给定的，因此不同规模的数据集应选用不同的n，参考推荐参数表
    Context context = ContextBuilder<CKKS>().m(32768).bits(358).precision(30).c(6).build();//选取了最大的m以获得足够的n slots
    //bits的选择上主要是为了同台计算的深度，由于数据规模较大
    SecKey secretKey(context);
    secretKey.GenSecKey();
    const PubKey &publicKey = secretKey;

    //读入文件
    vector<double> x0, y0;
    openFile("points.txt",x0,y0);

    //参数读入
    double minPts = 2;//参数1，邻居个数         以明文的形式和传递给处理方 //注意，包含了其本身应当减去1！！！！
    //cout << "please enter the minPts : ";
    //cin >> minPts;
    double e = 3.000001;//参数2，距离          数据拥有着持有，输入后进行加密操作  //存在误差，本身的邻域距离都是1
    //cout << "please enter the e : ";
    //cin >> e;
    
    //坐标向量x0,y0的密文形式
    PtxtArray x1(context,x0),y1(context,y0);
    Ctxt x(publicKey),y(publicKey);
    x1.encrypt(x);
    y1.encrypt(y);

    //n份拷贝暴力算法
    vector<Ctxt> nx, ny;
    for(int i = 0;i < N;i ++)
    {
        PtxtArray xx(context,ncopyx[i]),yy(context,ncopyy[i]);
        Ctxt nxx(publicKey),nyy(publicKey);
        xx.encrypt(nxx);
        yy.encrypt(nyy);
        nx.push_back(nxx);
        ny.push_back(nyy);
    }

    // //参数e的密文的n个拷贝
    // vector<double> e0(N,e);
    // PtxtArray pe(context,e0);
    // Ctxt ce(publicKey);
    // pe.encrypt(ce);
//=================================================================================================================
    //距离计算
    ppSEDcalculation(x, y, e, context,nx,ny,secretKey);

    //对于每个输入的数据记录创建一个EncryptedInputRecord,并进行初始化
    vector<EncryptedInputRecord> EncryptedElements;
    vector<vector <Ctxt>> cEncryptedElements;

    for(int i = 0;i < N;i ++)
    {
        EncryptedInputRecord Element;
        vector <Ctxt> ck;

        Element.clusterID = 0;
        vector<double> clusterID (N,0);
        PtxtArray pclusterID(context,clusterID);
        Ctxt cclusterID(publicKey);
        pclusterID.encrypt(cclusterID);
        ck.push_back(cclusterID);

        Element.isNoise = 0;
        vector<double> isNoise(N,0);
        PtxtArray pisNoise(context,isNoise);
        Ctxt cisNoise(publicKey);
        pisNoise.encrypt(cisNoise);
        ck.push_back(cisNoise);

        Element.notProcess = 1;
        vector<double> notProcess(N,1);
        PtxtArray pnotProcess(context,notProcess);
        Ctxt cnotProcess(publicKey);
        pnotProcess.encrypt(cnotProcess);
        ck.push_back(cnotProcess);

        
        Element.neighbors.assign(dist[i].begin(),dist[i].end()); //oh my god im gonna be mad
        PtxtArray pneighbors(context,dist[i]);
        Ctxt cneighbors(publicKey);
        pneighbors.encrypt(cneighbors);
        ck.push_back(cneighbors);

        //计算邻居数目
        double neighborcount = accumulate(Element.neighbors.begin(),
                                                Element.neighbors.end(),0);
    
        if(neighborcount >= minPts) {Element.validNeighborhoodSize = 1;ck.push_back(cnotProcess);}
        else {Element.validNeighborhoodSize = 0;ck.push_back(cclusterID);}

        EncryptedElements.push_back(Element);
        cEncryptedElements.push_back(ck);
    }
    
    //开始执行隐私保护DBSCAN协议
    vector<double> currentClusterID(N,0);
    PtxtArray pcurrentClusterID(context,currentClusterID);
    Ctxt ccurrentClusterID(publicKey);
    pcurrentClusterID.encrypt(ccurrentClusterID);
    
    vector<double> isCoreElement(N,0);
    PtxtArray pisCoreElement(context,isCoreElement);
    Ctxt cisCoreElement(publicKey);
    pisCoreElement.encrypt(cisCoreElement);

    Ctxt c0 = cisCoreElement;
    vector<double> cc(N,1);
    PtxtArray pcc(context,cc);
    Ctxt c1(publicKey);
    pcc.encrypt(c1);

    vector<double> ccc(N,-1);
    PtxtArray pccc(context,ccc);
    Ctxt cc1(publicKey);
    pccc.encrypt(cc1);


    Privacy_preserving_DBSCAN(cEncryptedElements,ccurrentClusterID, cisCoreElement, c0, c1, cc1,secretKey,context);

    //归纳结果========================================================================================================
    vector<double> resclusterID;
    // int duibi = EncryptedElements[0].clusterID;
    // resclusterID.push_back(EncryptedElements[0].clusterID);
    // for(int i = 1;i < EncryptedElements.size();i ++)
    // {
    //     if(EncryptedElements[i].clusterID != duibi)
    //     {
    //         duibi = EncryptedElements[i].clusterID;
    //         resclusterID.push_back(EncryptedElements[i].clusterID);
    //     }
    //     else
    //         continue;
    // }
    cout << endl;
    for(int i = 0;i < N;i ++)
    {
        vector<double> resv;
        PtxtArray resp(context);
        resp.decrypt(cEncryptedElements[i][0],secretKey);
        resp.store(resv);
        cout << "ClusterID : " << resv[i] << endl;
    }
    // int numcluster = resclusterID.size();//聚类的数量
    // cout << "the number of the cluster is : " << numcluster << endl;
    
    return 0;
}

//===================== 推荐参数表 =========================    
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
    	262144	2234	2  
*/