/*
 * 读写文件
 */
#ifndef _CONFIG_UTILS_H_
#define _CONFIG_UTILS_H_
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
using namespace std;
inline void saveConfigString(const char* data,QString filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    file.write(data);
    file.close();
}

inline bool saveConfigQString(QString data,QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append))
    {
        cout<<"file open field!"<<endl;
        return false;
    }
    QTextStream out(&file);
    if(data.size() > 0)
    {

        out<< data+"\n";
    }

    file.close() ;
    return true;
}

inline bool LoadConfigString(double& value, string name)
{
    ifstream ifs(name.c_str());
    if(!ifs){
        ifs.close();
        return false;
    }
    string bufline;
    getline(ifs, bufline, '\n');
    value = atof(bufline.c_str());
    ifs.close();
    return true;
}

inline bool LoadConfigString(int& value, string name)
{
    ifstream ifs(name.c_str());
    if(!ifs){
        ifs.close();
        return false;
    }
    string bufline;
    getline(ifs, bufline, '\n');
    value = atoi(bufline.c_str());
    ifs.close();
    return true;
}

inline bool LoadConfigString(string& value, string name)
{
    ifstream ifs(name.c_str());
    if(!ifs)
    {
        ifs.close();
        return false;
    }
    string bufline;
    getline(ifs, bufline, '\n');
    value = bufline;
    ifs.close();
    return true;
}

inline bool SaveConfigString(const double& value, string name)
{
    if( (access(name.c_str(), 06 )) == -1 )
    {
        stringstream ss;
        ss << "echo 0 > \"" << (name) << "\"";
        system(ss.str().c_str());
    }
    ofstream fs(name.c_str());
    if(!fs){
        fs.close();
        return false;
    }
    fs  << value << endl;
    fs.close();
    return true;
}

inline bool SaveConfigString(const int& value, string name)
{
    if( (access(name.c_str(), 06 )) == -1 )
    {
        stringstream ss;
        ss << "echo 0 > \"" << (name) << "\"";
        system(ss.str().c_str());
    }
    ofstream fs(name.c_str());
    if(!fs){
        fs.close();
        return false;
    }
    fs  << value << endl;
    fs.close();
    return true;
}

inline bool SaveConfigString(const string& value, string name)
{
    if((access(name.c_str(), 06 )) == -1 )
    {
        stringstream ss;
        ss << "echo 0 > \"" << (name) << "\"";
        system(ss.str().c_str());
    }
    ofstream fs(name.c_str());
    if(!fs)
    {
        fs.close();
        return false;
    }
    fs  << value << endl;
    fs.close();
    return true;
}
inline bool saveDateToCsv(QString filename,QStringList lsit,bool isOneLine)
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append))
    {
        cout<<"file open field!"<<endl;
        return  false;
    }
    QTextStream out(&file);
    if(lsit.size())
    {
        for(int i=0;i<lsit.size();i++)
        {
            out<< lsit.at(i)+",";
        }
        if(isOneLine)
        {
            out<<",\n";
        }
    }
    file.close() ;
    return  true;
}

inline bool saveMedDateToCsv(QString filename,QStringList lsit,bool isOneLine)
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append))
    {
        cout<<"file open field!"<<endl;
        return  false;
    }
    //情况1将字符编码设置为ANSI即GB码，因为window下的excel的编码默认为ANSI码
    //避免在window下打开该文件中文乱码
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QTextCodec::setCodecForLocale(codec);
    QTextStream out(&file);
    if(lsit.size())
    {
        for(int i=0;i<lsit.size();i++)
        {
            out<< lsit.at(i)+",";
        }
        if(isOneLine)
        {
            out<<",\n";
        }
    }
    file.close() ;
    //情况2 Linux下编码默认为utf-8编码，即unicode编码，前面设置了ANSI编码(GB码)
    //这里恢复utf-8编码，避免系统中其它地方出现乱码现象
    QTextCodec *utf8 = QTextCodec::codecForName("utf-8");
    QTextCodec::setCodecForLocale(utf8);
    return  true;
}
#endif
