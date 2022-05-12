#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>

using namespace std;

vector<string> openFile(const char* dataset1){
	fstream file;
	file.open(dataset1,ios::in);

	vector<string> data;
	while(!file.eof()){
		string temp;
		file>>temp;
		temp.push_back(';');
        data.push_back(temp);
    }
	file.close();
	cout<<"successful!"<<endl;
    return data;
}

int main()
{
    vector<string>data = openFile("788points.txt");
    fstream clustering;
	clustering.open("788points.txt",ios::out);
    for(int i = 0;i < 788;i ++)
    clustering << data[i] << endl;

    return 0;
}