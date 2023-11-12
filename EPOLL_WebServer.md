# 简易实现的epoll_WebServer项目

## 一、总体设计

### 1、web服务器的简易流程

web服务器<=>本地浏览器

（1）开发web服务器已经明确要使用http协议传送html文件

（2）开发协议选择是TCP+HTTP

（3）编写一个TCP并发服务器

（4）收发消息的格式采用的是HTTP协议

### 2、利用epoll_IO实现的流程

（1）创建socket套接字

（2）绑定套接字bind

（3）设置循环监听listen

（4）创建epoll根节点

（5）根节点加入监听队列epoll_ctl

（6）开始循环监听epoll_wait：获得新连接/处理现有客户端的请求

### 3、处理客户端请求流程

（1）获取请求行

（2）读缓冲区消息

（3）解析请求行信息

（4）判断文件/资源是否存在

​	存在：发送普通文件/目录？

​	不存在：404 NOT FOUND

## 二、具体实现

### 1、主函数编写

#### （1）更改工作目录

```C
    signal(SIGPIPE,SIG_IGN);
	//切换工作目录
	//获取当前目录的工作路径
	char pwd_path[256]="";
	char * path = getenv("PWD");
	///home/itheima/share/bjc++34/07day/web-http
	strcpy(pwd_path,path);
	strcat(pwd_path,"/web-http");
	chdir(pwd_path);
```

#### （2）实现epoll_IO的流程：创建（lfd）-开启监听-上树-循环监听 

```c
    //create fds 创建套接字
    int lfd=tcp4bind(PORT,NULL);
    //listen 监听 设置最大监听时长为128
    Listen(lfd,128);
    //create tree 创建根节点
    int epfd=epoll_create(1);
    //lfd upto tree 根节点加入监听树中 设置最大监听事件为1024
    struct epoll_event ev,evs[1024];
    ev.data.fd=lfd;
    ev.events=EPOLLIN;
    //加入到监听树中
    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
```

int **tcp4bind**(short port,const char *IP) //传入端口和IP地址 #define PORT 8080

int **Listen**(int fd, int backlog)//传入服务器套接字和监听时长

extern int **epoll_create** (int __size) ___THROW;//Creates an epoll instance.  Returns an fd for the new instance.

**EPOLLIN**;//设置输入事件

//传入根节点  上树事件  设置的树  

//Returns 0 in case of success,-1 in case of error

**extern int epoll_ctl** (int __epfd, int __op, int __fd, struct epoll_event *__event) __THROW;

------

#### （3）循环监听流程

##### 开始循环监听 

设置最大监听数为1024 timeout=-1 (-1 == infinite) 一直监听 返回触发的事件数目

extern int epoll_wait (int __epfd, struct epoll_event *__events, int __maxevents, int __timeout);

```c
int nready=epoll_wait(epfd,evs,1024,-1);//max events=1024 listen all times=-1
```

```c
for(int i=0;i<nready;i++)//利用for循环循环读事件
```

```c
if(evs[i].data.fd==lfd && evs[i].events&EPOLLIN)//判断是否为lfd
```

##### 设置客户端套接字（set cfd）

```c
//set cliaddr
struct sockaddr_in cliaddr;
char ip[16]="";
socklen_t len=sizeof(cliaddr);
```

Accept：重新封装的accept函数：Await a connection on socket FD.

extern int accept (int __fd, __SOCKADDR_ARG __addr,socklen_t *__restrict __addr_len);

```c
int cfd=Accept(lfd,(struct sockaddr*)&cliaddr,&len);
```

##### 设置cfd非阻塞-套路写法

```c
int flag=fcntl(cfd,F_GETFL);
flag|=O_NONBLOCK;
fcntl(cfd,F_SETFL,flag);
```

##### cfd上监听树

```c
ev.data.fd=cfd;
ev.events=EPOLLIN;
epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
```

##### 处理客户端请求

**evs[i].events&EPOLLIN 只处理读事件**-EPOLL_IN

调用**read_client_request**

```c
else if(evs[i].events&EPOLLIN)//cfd read_client_request
{    //传入evs的地址和树(根)
     read_client_request(epfd,&evs[i]);
}
```

#### （4）整体代码实现

```c
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
                if(evs[i].data.fd==lfd && evs[i].events & EPOLLIN)//lfd
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
```

### 2、read_client_request函数实现

**读取缓冲区的内容**

**Readline**：从buf中读取一行的内容 最大长度为1024

```c
//读取请求(先读取一行,在把其他行读取,扔掉)
char buf[1024]="";//read and print
char tmp[1024]="";//read and wait for print
int n=Readline(ev->data.fd,buf,sizeof(buf));
```

**读取失败直接退出关闭套接字**

```C
if(n<=0)
{
   //if not read->down tree
   printf("close or err\n");
   epoll_ctl(epfd,EPOLL_CTL_DEL,ev->data.fd,ev);
   close(ev->data.fd);
   return;
}
```

**读取成功就一直读**

利用while循环读

前方设置cfd非阻塞的原因：可以一直读

```C
//if ret>0 while(reading)
//cfd unblock-> while reading ok
while(ret=Readline(ev->data.fd,tmp,sizeof(tmp))>0);
printf("read ok \n");
```

**解析请求**：**GET /a.txt  HTTP/1.1\R\N**

读取buf内传来的请求：method,content,protocol

```C
char method[256]="";
char content[256]="";
char protocol[256]="";
sscanf(buf,"%[^ ] %[^ ] %[^ \r\n]",method,content,protocol);
printf("[%s]  [%s]  [%s]\n",method,content,protocol );
```

**解析请求的流程：**

**判断是否为get请求, get请求->处理**

**得到浏览请求的文件/如过对方没有请求文件->do 默认请求**

**判断文件是否存在,如果存在(普通文件/目录)?**

**不存在发送error.html**

判断是否是get请求

**strcasecmp函数**：判断

/* Compare S1 and S2, ignoring case.  */
extern int strcasecmp (const char *__s1, const char *__s2) __THROW __attribute_pure__ __nonnull ((1, 2));

```c
if(strcasecmp(method,"get")==0)
```

**字符解码：//[GET]  [/%E8%8B%A6%E7%93%9C.txt]  [HTTP/1.1]**

**struct stat s;**

stat：在 C 语言中，`<sys/stat.h>` 头文件提供了有关文件状态的信息，包括 `stat` 结构体。`stat` 结构体用于保存文件的状态信息，例如文件大小、创建时间、修改时间等。以下是 `stat` 结构体的定义：

```c
struct stat 
{
    dev_t     st_dev;         /* 文件的设备 ID */
    ino_t     st_ino;         /* 文件的 inode 号 */
    mode_t    st_mode;        /* 文件的类型和权限 */
    nlink_t   st_nlink;       /* 连接数 */
    uid_t     st_uid;         /* 文件的用户 ID */
    gid_t     st_gid;         /* 文件的组 ID */
    dev_t     st_rdev;        /* 如果文件是设备文件，则为设备 ID */
    off_t     st_size;        /* 文件的大小，以字节为单位 */
    blksize_t st_blksize;     /* 文件系统块的大小 */
    blkcnt_t  st_blocks;      /* 文件占用的块数 */
    time_t    st_atime;       /* 最后访问时间 */
    time_t    st_mtime;       /* 最后修改时间 */
    time_t    st_ctime;       /* 最后更改时间 */
};
```

```c
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
```

**判断文件的类型**

```c
int stat(const char *path, struct stat *buf);
```

`stat` 函数通过文件路径 `path` 获取文件状态信息，并将结果存储在提供的 `struct stat` 结构体指针 `buf` 中。函数成功时返回 0，失败时返回 -1。

**请求的文件不存在-发送报头-获得文件类型（404 NOT FOUND）-空行-发送404html**

调用send_file和send_header函数

```c
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
```

**请求的文件存在-判断是普通文件还是目录文件**

`S_ISREG` 是一个宏，通常在 C 语言的文件状态检查中使用。它用于测试给定的文件模式是否表示一个常规文件（regular file）。在使用时，你可以将文件模式作为参数传递给 `S_ISREG` 宏，它将返回一个非零值（真），表示文件是一个常规文件；否则，它将返回零（假）。

**普通文件**

```c
if(S_ISREG(s.st_mode))
{
  printf("file\n");
  //先发送 报头(状态行  消息头  空行)
  /*st_size: Size of file, in bytes.*/
  send_header(ev->data.fd,200,"OK",get_mime_type(strfile),s.st_size);
  //发送文件
  send_file(ev->data.fd,strfile,ev,epfd,1);
}
```

**目录文件**

`S_ISDIR` 是一个在 C 语言的文件状态检查中使用的宏。它用于测试给定的文件模式是否表示一个目录（directory）。你可以将文件模式作为参数传递给 `S_ISDIR` 宏，它将返回一个非零值（真），表示文件是一个目录；否则，它将返回零（假）。

**scandir函数**

`scandir` 是一个 C 语言标准库函数，用于读取目录并检索其内容。它在 `<dirent.h>` 头文件中声明。

```c
int scandir(const char *dir, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
```

这个函数的参数包括：

- `dir`：指定目录的路径。
- `namelist`：用于存储目录内容的结构体数组。这是一个指向 `struct dirent **` 的指针，函数会分配内存来存储目录中的每个条目，并将指向这些条目的指针存储在 `namelist` 中。
- `filter`：一个可选的函数指针，用于过滤目录中的条目。如果为 `NULL`，则不进行过滤。
- `compar`：一个可选的函数指针，用于比较目录中的条目以进行排序。如果为 `NULL`，则不进行排序。

函数的返回值是扫描到的目录项的数量。如果出现错误，或者无法打开目录，返回值为 -1。

**关于此函数的三级指针：传入三级指针-设置二级指针数组存储文件名指针-文件名指针指向所要读取的文件名**

```c
struct dirent ***namelist
```

**关于函数alphabetic_compare-替代alphasort函数**



```c
else if(S_ISDIR(s.st_mode))//请求的是一个目录 is dictionary
{
	//传入一个大小0的 网页 里面有列表
	send_header(ev->data.fd,200,"OK",get_mime_type("*.html"),0);
	//send header.html
	send_file(ev->data.fd,"dir_header.html",ev,epfd,0);
	struct dirent **mylist=NULL;
	char buf[1024]="";
	int len =0;
	int n = scandir(strfile,&mylist,NULL,alphabetic_compare)
}
```

**for循环输出**

```c
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
```

**发送目录html**

```c
free(mylist);
send_file(ev->data.fd,"dir_tail.html",ev,epfd,1);
```

**整体函数实现**

```c
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
    while(ret=Readline(ev->data.fd,tmp,sizeof(tmp))>0);
    printf("read ok \n");
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
```

### 3、send_file函数实现

**整体函数实现**

```c
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
```

**设置为只读打开**

`O_RDONLY` 是一个用于在 C 语言中打开文件的标志，通常用于 `open` 系统调用。这个标志表示以只读方式打开文件，即文件是只可读的，不可写。

在 C 语言中，`open` 函数用于打开文件或创建文件。该函数声明在 `<fcntl.h>` 头文件中，并通常与 `<sys/types.h>` 和 `<sys/stat.h>` 头文件一起使用。

- `pathname`：要打开的文件的路径。
- `flags`：文件打开标志，可以使用 `O_RDONLY`、`O_WRONLY`、`O_RDWR` 等标志的按位 OR 运算组合。
- `mode`：当创建新文件时，指定文件的权限。通常与 `O_CREAT` 标志一起使用。

`open` 函数返回一个文件描述符（file descriptor），用于后续对该文件的读写操作。如果出现错误，返回值为 -1。

```c
#include <fcntl.h>

int open(const char *pathname, int flags, mode_t mode);
```

```c
int fd=open(path,O_RDONLY);
if(fd<0)
{
  perror("");
  return ;
}
```

**循环读取**

```c
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
```

**读完文件之后，关闭cfd，下树**

```c
close(fd);
//关闭cfd,下树
if(flag==1)
{
  close(cfd);
  epoll_ctl(epfd,EPOLL_CTL_DEL,cfd,ev);
}
```

### 4、send_header函数实现

**整体函数实现**

```c
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
```

### 5、头文件设定

```c
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include "pub.h"
#include "wrap.h"
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
```

### 6、自建库-黑马自建库源程序

#### （1）#include "pub.h" pub.c 字符编码处理

```c
#ifndef _PUB_H
#define _PUB_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <ctype.h>
char *get_mime_type(char *name);
int get_line(int sock, char *buf, int size);
int hexit(char c);//16进制转10进制
void strencode(char* to, size_t tosize, const char* from);//编码
void strdecode(char *to, char *from);//解码
#endif
```

```c
#include "pub.h"
//通过文件名字获得文件类型
char *get_mime_type(char *name)
{
    char* dot;

    dot = strrchr(name, '.');	//自右向左查找‘.’字符;如不存在返回NULL
    /*
     *charset=iso-8859-1	西欧的编码，说明网站采用的编码是英文；
     *charset=gb2312		说明网站采用的编码是简体中文；
     *charset=utf-8			代表世界通用的语言编码；
     *						可以用到中文、韩文、日文等世界上所有语言编码上
     *charset=euc-kr		说明网站采用的编码是韩文；
     *charset=big5			说明网站采用的编码是繁体中文；
     *
     *以下是依据传递进来的文件名，使用后缀判断是何种文件类型
     *将对应的文件类型按照http定义的关键字发送回去
     */
    if (dot == (char*)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
//获得一行数据，每行以\r\n作为结束标记
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);//MSG_PEEK 从缓冲区读数据，但是数据不从缓冲区清除
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

//下面的函数第二天使用
/*
 * 这里的内容是处理%20之类的东西！是"解码"过程。
 * %20 URL编码中的‘ ’(space)
 * %21 '!' %22 '"' %23 '#' %24 '$'
 * %25 '%' %26 '&' %27 ''' %28 '('......
 * 相关知识html中的‘ ’(space)是&nbsp
 */
 // %E8%8B%A6%E7%93%9C
void strdecode(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from) 
    {

        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) 
        { //依次判断from中 %20 三个字符

            *to = hexit(from[1])*16 + hexit(from[2]);//字符串E8变成了真正的16进制的E8
            from += 2;                      //移过已经处理的两个字符(%21指针指向1),表达式3的++from还会再向后移一个字符
        } else
            *to = *from;
    }
    *to = '\0';
}

//16进制数转化为10进制, return 0不会出现
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

//"编码"，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
//strencode(encoded_name, sizeof(encoded_name), name);
void strencode(char* to, size_t tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}
```

#### （2）#include "wrap.h" wrap.c 套接字处理

```c
#ifndef __WRAP_H_
#define __WRAP_H_
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>

void perr_exit(const char *s);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);
int Close(int fd);
ssize_t Readn(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);
ssize_t my_read(int fd, char *ptr);
ssize_t Readline(int fd, void *vptr, size_t maxlen);
int tcp4bind(short port,const char *IP);
#endif
```

```c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>

void perr_exit(const char *s)
{
	perror(s);
	exit(-1);
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int n;

again:
	if ((n = accept(fd, sa, salenptr)) < 0) {
		if ((errno == ECONNABORTED) || (errno == EINTR))//如果是被信号中断和软件层次中断,不能退出
			goto again;
		else
			perr_exit("accept error");
	}
	return n;
}

int Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
    int n;

	if ((n = bind(fd, sa, salen)) < 0)
		perr_exit("bind error");

    return n;
}

int Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    int n;

	if ((n = connect(fd, sa, salen)) < 0)
		perr_exit("connect error");

    return n;
}

int Listen(int fd, int backlog)
{
    int n;

	if ((n = listen(fd, backlog)) < 0)
		perr_exit("listen error");

    return n;
}

int Socket(int family, int type, int protocol)
{
	int n;

	if ((n = socket(family, type, protocol)) < 0)
		perr_exit("socket error");

	return n;
}

ssize_t Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;

again:
	if ( (n = read(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)//如果是被信号中断,不应该退出
			goto again;
		else
			return -1;
	}
	return n;
}

ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
	ssize_t n;

again:
	if ( (n = write(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

int Close(int fd)
{
    int n;
	if ((n = close(fd)) == -1)
		perr_exit("close error");

    return n;
}

/*参三: 应该读取固定的字节数数据*/
ssize_t Readn(int fd, void *vptr, size_t n)
{
	size_t  nleft;              //usigned int 剩余未读取的字节数
	ssize_t nread;              //int 实际读到的字节数
	char   *ptr;

	ptr = vptr;
	nleft = n;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;
			else
				return -1;
		} else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return n - nleft;
}
/*:固定的字节数数据*/
ssize_t Writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

static ssize_t my_read(int fd, char *ptr)
{
	static int read_cnt;
	static char *read_ptr;
	static char read_buf[100];

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return -1;
		} else if (read_cnt == 0)
			return 0;
		read_ptr = read_buf;
	}
	read_cnt--;
	*ptr = *read_ptr++;

	return 1;
}

ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char    c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c  == '\n')
				break;
		} else if (rc == 0) {
			*ptr = 0;
			return n - 1;
		} else
			return -1;
	}
	*ptr  = 0;

	return n;
}

int tcp4bind(short port,const char *IP)
{
    struct sockaddr_in serv_addr;
    int lfd = Socket(AF_INET,SOCK_STREAM,0);
    bzero(&serv_addr,sizeof(serv_addr));
    if(IP == NULL){
        //如果这样使用 0.0.0.0,任意ip将可以连接
        serv_addr.sin_addr.s_addr = INADDR_ANY;
    }else{
        if(inet_pton(AF_INET,IP,&serv_addr.sin_addr.s_addr) <= 0){
            perror(IP);//转换失败
            exit(1);
        }
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port);
    int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    return lfd;
}
```

## 三、总结

第一个小项目。
