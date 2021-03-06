/*************************************************************************
	> File Name: client.cpp
	> Author: yanyuchen
	> Mail: 794990923@qq.com
	> Created Time: 2017年08月01日 星期二 08时21分29秒
 ************************************************************************/
#include<iostream>
#include<string.h>
#include<math.h>
#include<sys/signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include <termios.h>
#include<assert.h>

#include <json/json.h>
#include"protocol.h"

#define PORT 6666   //服务器端口


//网络操作码flags:
    //定义在protocol头文件中

using namespace std;


class TCPClient
{
    public:

    TCPClient(int argc ,char** argv);
    ~TCPClient();

    char data_buffer[10000];    //存放发送和接收数据的buffer


    //向服务器发送数据
    bool send_to_serv(int datasize,uint16_t wOpcode);
    //从服务器接收数据
    bool recv_from_serv();
    void run(TCPClient & client); //主运行函数


    private:

    int conn_fd; //创建连接套接字
    struct sockaddr_in serv_addr; //储存服务器地址

};




TCPClient::TCPClient(int argc,char **argv)  //构造函数
{

    if(argc!=3)    //检测输入参数个数是否正确
    {
        cout<<"Usage: [-a] [serv_address]"<<endl;
        exit(1);
    }

    memset(data_buffer,0,sizeof(data_buffer));  //初始化buffer

    //初始化服务器地址结构
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(PORT);
    
    //从命令行服务器地址
    for(int i=0;i<argc;i++)
    {
        if(strcmp("-a",argv[i])==0)
        {

            if(inet_aton(argv[i+1],&serv_addr.sin_addr)==0)
            {
                cout<<"invaild server ip address"<<endl;
                exit(1);
            }
            break;
        }
    }

    //检查是否少输入了某项参数
    if(serv_addr.sin_addr.s_addr==0)
    {
        cout<<"Usage: [-a] [serv_address]"<<endl;
        exit(1);
    }

    //创建一个TCP套接字
    conn_fd=socket(AF_INET,SOCK_STREAM,0);


    if(conn_fd<0)
    {
        my_err("connect",__LINE__);
    }
    

    //向服务器发送连接请求
    if(connect(conn_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))<0)
    {
        my_err("connect",__LINE__);
    }

}

TCPClient::~TCPClient()  //析构函数
{
    close(conn_fd);

}

bool TCPClient::send_to_serv(int datasize,uint16_t wOpcode) //向服务器发送数据
{
    NetPacket send_packet;   //数据包
    send_packet.Header.wDataSize=datasize+sizeof(NetPacketHeader);  //数据包大小
    send_packet.Header.wOpcode=wOpcode;


    memcpy(send_packet.Data,data_buffer,datasize);  //数据拷贝


    if(send(conn_fd,&send_packet,send_packet.Header.wDataSize,0))
    return true;
    else
    return false;

}

bool TCPClient::recv_from_serv()   //从服务器接收数据
{
    int nrecvsize=0; //一次接收到的数据大小
    int sum_recvsize=0; //总共收到的数据大小
    int packersize;   //数据包总大小
    int datasize;     //数据总大小


    memset(data_buffer,0,sizeof(data_buffer));   ///初始化buffer

      while(sum_recvsize!=sizeof(NetPacketHeader))
    {
        nrecvsize=recv(conn_fd,data_buffer+sum_recvsize,sizeof(NetPacketHeader)-sum_recvsize,0);
        if(nrecvsize==0)
        {
            //服务器退出;
            return false;
        }
        if(nrecvsize<0)
        {
            cout<<"从客户端接收数据失败"<<endl;
            return false;
        }
        sum_recvsize+=nrecvsize;

    }



    NetPacketHeader *phead=(NetPacketHeader*)data_buffer;
    packersize=phead->wDataSize;  //数据包大小
    datasize=packersize-sizeof(NetPacketHeader);     //数据总大小




    while(sum_recvsize!=packersize)
    {
        nrecvsize=recv(conn_fd,data_buffer+sum_recvsize,packersize-sum_recvsize,0);
        if(nrecvsize==0)
        {
            cout<<"从客户端接收数据失败"<<endl;
            return false;
        }
        sum_recvsize+=nrecvsize;
    }

    return true;

}

//函数声明:
bool inputpasswd(string &passwd);   //无回显输入密码
void Login_Register(TCPClient client);    //登录注册函数
void menu(TCPClient client);  //菜单函数


bool inputpasswd(string &passwd)   //无回显输入密码
{
    struct termios tm,tm_old;
    int fd = STDIN_FILENO, c,d;
    if(tcgetattr(fd, &tm) < 0)
    {
        return false;
    }
    tm_old = tm;
    cfmakeraw(&tm);
    if(tcsetattr(fd, TCSANOW, &tm) < 0)
    {
        return false;
    }
    cin>>passwd;
    if(tcsetattr(fd, TCSANOW, &tm_old) < 0)
    {
        return false;
    }
    return true;
}

void Login_Register(TCPClient client)   //登录注册函数
{

    string choice;           //记录选择
    int choice_right=1;     //判断输入选项是否正确
    string number,nickname,sex,address,birthdate, phone;               
    string passwd,passwd1;
    Json::Value accounts;



    cout<<"\t\t欢迎使用图书借阅系统:"<<endl;
    cout<<"\t\t\t1.登录"<<endl;
    cout<<"\t\t\t2.注册"<<endl;
    cout<<"\t\t\t3.退出"<<endl;
    cout<<"\n请输入选择序号:";

    while(choice_right)
    {
        cin>>choice;
        if(choice=="1")  //登录
        {
            choice_right=0;
            cout<<"请输入帐号:";
            cin>>number;
            cout<<"请输入密码:"; 
            inputpasswd(passwd);
            cout<<endl;
            accounts["number"]=number.c_str();           //加入json对象中
            accounts["passwd"]=passwd.c_str();
            string out=accounts.toStyledString();
            memcpy(client.data_buffer,out.c_str(),out.size());   //拷贝到数据buffer
            if(client.send_to_serv(out.size(),LOGIN)==false)  //向服务器发送数据
            {
                cout<<"向服务器发送数据失败"<<endl;
                return;
            }
            if(client.recv_from_serv()==false)   //从服务器接收数据
            {
                cout<<"从服务器接收数据失败"<<endl;
                return;
            }

            NetPacketHeader *phead=(NetPacketHeader*)client.data_buffer;
            if(phead->wOpcode==LOGIN_YES) //登录成功
            {
                cout<<"登录成功"<<endl;
                menu(client);
            }
            else
            {
                cout<<"登录失败，帐号或密码错误"<<endl;
            }


        }
        else if(choice=="2")  //注册
        {
            choice_right=0;
            cout<<"请输入要注册的帐号:";
            cin>>number;
            cout<<"请输入昵称:";
            cin>>nickname;
            cout<<"请输入性别:";
            cin>>sex;
            cout<<"请输入地址:";
            cin>>address;
            cout<<"请输入生日:";
            cin>>birthdate;
            cout<<"请输入手机号码:";
            cin>>phone;
            cout<<"请设置密码:";
            inputpasswd(passwd);
            cout<<endl;
            cout<<"请再次输入密码:";
            inputpasswd(passwd1);
            cout<<endl;
            while(passwd!=passwd1)
            {
                cout<<"两次输入的密码不同，请重新设置密码。"<<endl;
                cout<<"请设置密码:";
                inputpasswd(passwd);
                cout<<endl;
                cout<<"请再次输入密码:";
                inputpasswd(passwd1);
                cout<<endl;
            }
            accounts["number"]=number.c_str();
            accounts["nickname"]=nickname.c_str();
            accounts["sex"]=sex.c_str();
            accounts["address"]=address.c_str();
            accounts["birthdate"]=birthdate.c_str();
            accounts["phone"]=phone.c_str();
            accounts["passwd"]=passwd.c_str();
            string out=accounts.toStyledString();
            memcpy(client.data_buffer,out.c_str(),out.size());  //拷贝到数据buffer中


            if(client.send_to_serv(out.size(),REGISTER)==false)  //向服务器发送数据
            {
                cout<<"向服务器发送数据失败"<<endl;
                return;
            }
            if(client.recv_from_serv()==false)   //从服务器接收数据
            {
                cout<<"从服务器接收数据失败"<<endl;
                return;
            }

            NetPacketHeader *phead=(NetPacketHeader*)client.data_buffer;
            if(phead->wOpcode==REGISTER_YES)  //注册成功
            {
                cout<<"注册成功"<<endl;
                Login_Register(client);   
            }
            else
            {
                cout<<"注册失败,该帐号已被注册"<<endl;
            }
            
        }
            
        else if(choice=="3")
        {
            cout<<"Bye"<<endl;
            exit(0);
        }
        else
        {
            cout<<"输入格式错误，请重新输入"<<endl;
        }
    }
}

void Personal_data(TCPClient client)
{
    if(client.send_to_serv(0, PERSONAL_DATA)==false)  //向服务器发送数据失败
    {
        cout<<"向服务器发送数据失败"<<endl;
        return;
    }

    if(client.recv_from_serv()==false)   //从服务器接收数据
    {
        cout<<"从服务器接收数据失败"<<endl;
        return;
    }
    

    NetPacket *phead=(NetPacket*)client.data_buffer;
    
    Json::Value accounts;
    Json::Reader reader;
    string str(phead->Data);
     
    if (reader.parse(str,accounts) < 0)
    {
        cout << "json解析失败" << endl;
    }
    else {
        cout << "账号：" << accounts["number"].asString()<< endl;
        cout << "密码：" << accounts["passwd"].asString()<< endl;
        cout << "昵称：" << accounts["nickname"].asString()<< endl;
        cout << "性别：" << accounts["sex"].asString()<< endl;
        cout << "地址：" << accounts["address"].asString()<< endl;
        cout << "生日：" << accounts["birthdate"].asString()<< endl;
        cout << "电话：" << accounts["phone"].asString()<< endl;
    }
    cout << "按任意键返回菜单\n" << endl;
    getchar(); 
    getchar(); 
    menu(client);
}

void menu(TCPClient client) 
{
    int choice;
    
    cout<<"\t\t欢迎进入功能界面:"<<endl;
    cout<<"\t\t\t1.查询个人资料"<<endl;
    cout<<"\t\t\t2.聊天"<<endl;
    cout<<"\t\t\t3.退出"<<endl;
    cout<<"\n请输入选择序号:";

    while (1) {
        cin >> choice;

        switch (choice)
        {
            case 1: Personal_data(client); break;
            case 2:                        break;
            case 3: cout<<"Bye"<<endl;     exit(0);
        }
    }
}

void TCPClient::run(TCPClient& client)
{
    Login_Register(client);  //登录注册界面

}

int main(int argc ,char **argv)
{

    TCPClient client(argc,argv);


    client.run(client);

}
