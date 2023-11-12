#include "stdio.h"
#include "wrap.h"
#include "sys/epoll.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "pub.h"
#include <dirent.h>
#include "signal.h"
#ifndef __USE_MISC
/* File types for `d_type'.  */
enum
  {
    DT_UNKNOWN = 0,
# define DT_UNKNOWN	DT_UNKNOWN
    DT_FIFO = 1,
# define DT_FIFO	DT_FIFO
    DT_CHR = 2,
# define DT_CHR		DT_CHR
    DT_DIR = 4,
# define DT_DIR		DT_DIR
    DT_BLK = 6,
# define DT_BLK		DT_BLK
    DT_REG = 8,
# define DT_REG		DT_REG
    DT_LNK = 10,
# define DT_LNK		DT_LNK
    DT_SOCK = 12,
# define DT_SOCK	DT_SOCK
    DT_WHT = 14
# define DT_WHT		DT_WHT
  };

/* Convert between stat structure types and directory types.  */
# define IFTODT(mode)	(((mode) & 0170000) >> 12)
# define DTTOIF(dirtype)	((dirtype) << 12)
#endif
#define PORT 8080
//replace alphasort
int alphabetic_compare(const struct dirent **a, const struct dirent **b) 
{
    return strcoll((*a)->d_name, (*b)->d_name);
}
void send_header(int cfd, int code,char *info,char *filetype,int length)
{
    //发送状态行
    //send head and length
    char buf[1024]="";
    int len=0;
    //主要功能是把格式化的数据写入某个字符串中，即发送格式化输出到 string 所指向的字符串。
    len=sprintf(buf,"HTTP/1.1 %d %s\r\n",code,info);
    //Read N bytes into BUF from socket FD. flags=0
    send(cfd,buf,len,0);
    //发送消息头 Content-Type
    len = sprintf(buf,"Content-Type:%s\r\n",filetype);
    send(cfd,buf,len,0);
    if(length>0)
    {
        //发送消息头
		len = sprintf(buf,"Content-Length:%d\r\n",length);
		send(cfd,buf,len,0);
    }
    //空行
	send(cfd,"\r\n",2,0);
}
void send_file(int cfd,char *path,struct epoll_event *ev,int epfd,int flag)
{
    //open and read_only
    int fd=open(path,O_RDONLY);
    if(fd<0)
    {
        perror("");
        return ;
    }
    char buf[1024]="";
    int len=0;
    while(1)
    {
        len=read(fd,buf,sizeof(buf));
        if(len<0)//read unsuccess
        {
            perror("");
            break;
        }
        else if(len==0)
        {
            break;//no data
        }
        else
        {
            int n=0;
            n=send(cfd,buf,len,0);
            printf("len=%d\n", n);
        }
    }
    close(fd);
    //关闭cfd,下树
    if(flag==1)
    {
        close(cfd);
        epoll_ctl(epfd,EPOLL_CTL_DEL,cfd,ev);
    }
}
void read_client_request(int epfd ,struct epoll_event *ev)
{
    //读取请求(先读取一行,在把其他行读取,扔掉)
    char buf[1024]="";//read and print
    char tmp[1024]="";//read and wait for print
    int n=Readline(ev->data.fd,buf,sizeof(buf));
    if(n<=0)
    {
        //if not read->down tree
        printf("close or err\n");
        epoll_ctl(epfd,EPOLL_CTL_DEL,ev->data.fd,ev);
        close(ev->data.fd);
        return;
    }
    //print buf
    printf("[%s]\n", buf);
    int ret=0;
    //if ret>0 while(reading)
    //cfd unblock-> while reading ok
    while(ret=Readline(ev->data.fd,tmp,sizeof(tmp))>0);\
    printf( "read ok \n" );
    /*
    解析请求
    判断是否为get请求, get请求->处理
    得到浏览请求的文件/如过对方没有请求文件->do 默认请求
    判断文件是否存在,如果存在(普通文件/目录)?
    不存在发送error.html
    */
    //解析请求行  GET /a.txt  HTTP/1.1\R\N
    //client_request协议组包
    char method[256]="";
	char content[256]="";
	char protocol[256]="";
    sscanf(buf,"%[^ ] %[^ ] %[^ \r\n]",method,content,protocol);
    printf("[%s]  [%s]  [%s]\n",method,content,protocol );
    //判断是否为get请求 get GET
    if(strcasecmp(method,"get")==0)
    {
        //request get a txt_file
        //[GET]  [/%E8%8B%A6%E7%93%9C.txt]  [HTTP/1.1]
        char *strfile=content+1;
        //void strdecode(char *to, char *from)
        //解码
        strdecode(strfile,strfile);
        //do not have any request
        //GET / HTTP/1.1\R\N
        //如果没有请求文件,默认请求当前目录
        if(*strfile == 0) strfile="./";
        //判断请求的文件在不在
        struct stat s;
        if(stat(strfile,&s)<0)/* Get file attributes for FILE and put them in BUF. */
        {
            //文件不存在
            printf("file not fount\n");
            //send header
            //先发送 报头(状态行  消息头  空行)
            //通过文件名字获得文件类型 char *get_mime_type(char *name)
            send_header(ev->data.fd,404,"not found",get_mime_type("*.html"),0);
            //send error_file->error.html
            send_file(ev->data.fd,"error.html",ev,epfd,1);
        }
        else//file exist
        {
            //请求的是一个普通的文件 is regular
            if(S_ISREG(s.st_mode))
            {
                printf("file\n");
                //先发送 报头(状态行  消息头  空行)
                /*st_size: Size of file, in bytes.*/
                send_header(ev->data.fd,200,"OK",get_mime_type(strfile),s.st_size);
                //发送文件
                send_file(ev->data.fd,strfile,ev,epfd,1);
            }
            else if(S_ISDIR(s.st_mode))//请求的是一个目录 is dictionary
            {
                printf("dir\n");
                //发送一个列表  网页
				//传入一个大小0的 网页 里面有列表
                send_header(ev->data.fd,200,"OK",get_mime_type("*.html"),0);
                //send header.html
                send_file(ev->data.fd,"dir_header.html",ev,epfd,0);
                /*scandir 读取目录下的文件
                struct dirent **mylist : //指向指针数组的首元素的地址
                int scandir(const char *dirp, struct dirent ***namelist,
                int (*filter)(const struct dirent *),
                int (*compar)(const struct dirent **, const struct dirent **));
                参数: 
                dirp: 目录的路径名
                namelist:  mylist地址
                filter: 过滤的函数入口地址
                compar : 排序函数入口地址   写 alphasort(字母排序)
                返回值: 读取文件的个数*/
                struct dirent **mylist=NULL;
				char buf[1024]="";
			    int len =0;
				int n = scandir(strfile,&mylist,NULL,alphabetic_compare);
                //print all names
                for(int i=0;i<n;i++)
                {
                    //然后在编译的时候加上 -D_BSD_SOURCE enable DT_DIR
                    if(mylist[i]->d_type==DT_DIR)
                    {
                        len = sprintf(buf,"<li><a href=%s/ >%s</a></li>",mylist[i]->d_name,mylist[i]->d_name);    
                    }
                    else
                    {
						len = sprintf(buf, "<li><a href=%s >%s</a></li>", mylist[i]->d_name, mylist[i]->d_name);
					}
                    send(ev->data.fd,buf,len ,0);
					free(mylist[i]);
                }
                free(mylist);
                send_file(ev->data.fd,"dir_tail.html",ev,epfd,1);
            }
        }
    }
}
int main()
{
    signal(SIGPIPE,SIG_IGN);
	//切换工作目录
	//获取当前目录的工作路径
	char pwd_path[256]="";
	char * path = getenv("PWD");
	///home/itheima/share/bjc++34/07day/web-http
	strcpy(pwd_path,path);
	strcat(pwd_path,"/web-http");
	chdir(pwd_path);

    //create fds
    int lfd=tcp4bind(PORT,NULL);
    //listen
    Listen(lfd,128);
    //create tree
    int epfd=epoll_create(1);
    //lfd upto tree
    struct epoll_event ev,evs[1024];
    ev.data.fd=lfd;
    ev.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    //while(listen)
    while(1)
    {
        //listen
        int nready=epoll_wait(epfd,evs,1024,-1);//max events=1024 listen all times=-1
        if(nready < 0)
		{
			perror("");
			break;
		}
        else
        {
            for(int i=0;i<nready;i++)
            {
                printf("001\n");
                //判断是否是lfd
                if(evs[i].data.fd==lfd && evs[i].events&EPOLLIN)//lfd
                {
                    //set cliaddr
                    struct sockaddr_in cliaddr;
                    char ip[16]="";
                    socklen_t len=sizeof(cliaddr);
                    int cfd=Accept(lfd,(struct sockaddr*)&cliaddr,&len);
                    //print details
                    printf("new client ip=%s port=%d\n",
						inet_ntop(AF_INET,&cliaddr.sin_addr.s_addr,ip,16),
						ntohs(cliaddr.sin_port));
                    //cfd unblock
                    //Do the file control operation described by CMD on FD.
                    //The remaining arguments are interpreted depending on CMD.
                    //cfd unblock
                    int flag=fcntl(cfd,F_GETFL);
                    flag|=O_NONBLOCK;
                    fcntl(cfd,F_SETFL,flag);
                    //cfd upto tree
                    ev.data.fd=cfd;
                    ev.events=EPOLLIN;
                    epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
                }
                else if(evs[i].events&EPOLLIN)//cfd read_client_request
                {
                    //传入evs的地址和树(根)
                    read_client_request(epfd,&evs[i]);
                }
            }
        }
    }
    return 0;
}