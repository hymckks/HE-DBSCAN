#include <cstdio>
#include <cstring>
#include <helib/helib.h>

using namespace helib;
using namespace std;

int main(int argc, char* argv[])
{
    Context context = ContextBuilder<CKKS>()
            .m(131072)   //m增加，n位数增加，安全性增加，且m必须是2的幂，但m越大性能越低
            .bits(1445)          //bits增加，安全性降低，但可以进行更深入的同态计算（与计算深度有关），密文大小也会增加
            .precision(30)        //2{-precision}精度，precison增加，同态计算深度减少，安全性与性能不变,建议(precison <= 40)
            .c(8)                 //c增加，安全性增加但性能会下降，公钥的内存需求也会增加，建议(2 <= c <= 8)
            .build();    
        //强烈建议参数按照表格推荐取值

    //测试安全等级（LWE）
    //cout << "security level : " << context.securityLevel() << endl;

    long n = 400;   //n = m / 4

    SecKey secretKey(context);  //构建一个私钥
    secretKey.GenSecKey();      //将构建的私钥调用GenSecKey方法，使其真正成为可用的私钥

    const PubKey &publicKey = secretKey;  //通过私钥初始化公钥对象

    //初始化完成，可以开始加密了

    //===================================================================================
    vector<double> v0(n);
    for(int i = 0;i < n;i ++)
    {
        v0[i] = i;
    }

    PtxtArray p0(context,v0);//将v0传入PtxtArray容器中，该容器与context关联（）

    Ctxt c0(publicKey);      //密文对象与公钥相关联

    p0.encrypt(c0);       //利用c0将p0进行加密（c0即为密文，用c开头变量进行数学操作）

    //在来俩密文


    PtxtArray p1(context,v0);
    Ctxt c1(publicKey);
    p1.encrypt(c1);

    PtxtArray p2(context,v0);
    Ctxt c2(publicKey);
    p2.encrypt(c2);

    Ctxt c3 = c0;
    c3 *= c1;
    Ctxt c4 = c2;
    c4 *= 1.5;
    c3 += c4;

    PtxtArray pp3(context);
    pp3.decrypt(c3,secretKey);
    vector<double> v3;
    pp3.store(v3);

    cout << "success !" << endl;
    
    return 0;
}
