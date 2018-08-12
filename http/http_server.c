#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

//定义一个结构体，来存储HTTP请求中的内容
#define SIZE (1024*10)
typedef struct HttpRequest
{
  char first_line[SIZE];//保存首行
  char *method;//指向首行中method的位置
  char *url;   //指向首行中url的位置
  char *url_path;//指向首行中url_path的位置
  char *query_string;//指向首行中query_string的位置
  int content_length;//保存body的长度
}HttpRequest;


//按行从socket中读取数据
int ReadLine(int sock,char buf[],ssize_t max_size)
{
  //实际上浏览器发送到请求中换行符可能不一样
  //换行符可能有：\n（换行，光标移到行首并另起一个新行）   
  //              \r (回车，将光标移动到行首) 
  //              \r\n
  //
  //1.循环从socket中读取字符，一次读一个
  char c='\0';
  ssize_t i=0;//描述当前读到的字符应该放到缓冲区的那个下标处
  while(i<max_size)
  {
    ssize_t read_size=recv(sock,&c,1,0);
    if(read_size<=0)
    {
      //此时认为读取数据失败，因为我们预期是要读到换行符的
      //当recv返回0时，认为收到的报文时非法的
      return -1;
    }
    //2.对读到的字符进行判定
    //3.如果当前字符是 \r
    if(c=='\r')
    {
      // （a）尝试从缓冲区读取下一个字符，若下一个字符为 \n，就把这种情况处理成 \n
      recv(sock,&c,1,MSG_PEEK);
      if(c=='\n')
      {
        //当前的行分隔符是\r\n,接下来把下一个\n从缓冲区中删掉就可以了
        recv(sock,&c,1,0);
      }
      // （b）若下一个字符为其他字符，就把 \r 修改成 \n (目的是把 \r 和 \n 的情况统一在一起处理)
      else
      {  
        //此时行分隔符是\r,为了处理方便，就把c中的\r改成\n
        c='\n'; 
      }
    }
    //5.如果当前字符是其他字符，就把这个字符放到buf中
    buf[i++]=c;//将这个代码放在第四步上面，读到的一行字符串就是带换行符的，
               //带不带换行其实都可以，只要保证逻辑正确即可
               //后面在读取空行时，若不带换行就要和空字符串比较，
               //要是带了换行符就要和 "\n" 进行比较
    

    //4.如果当前字符是 \n,就退出循环，函数结束
    if(c=='\n')
    {
      break;
    }
  }
  buf[i]='\0';
  return 0;
}

//首行切分
//致命问题：主要是由于 strtok 是线程不安全函数，需要用strtok_r来替代
ssize_t Split(char first_line[],const char* split_char,char* output[])
{
  //python中自己封装了split()函数
  //可以借助于C中的strtok()函数来实现
  int output_index=0;
  //此处的tmp必须是栈上的变量
  char *tmp=NULL;
  char *p=strtok_r(first_line,split_char,&tmp);
  while(p!=NULL)
  {
    //printf("output_index=%d\n",output_index);
    output[output_index++]=p;
    //后续循环调用时，第一个参数要填NULL，此时函数就会根据上次切分的结果继续往下切分
    p=strtok_r(NULL,split_char,&tmp);
  }
  return output_index;
}


//解析首行
//GET  /  HTTP1.1
int ParseFirstLine(char first_line[],char **method_ptr,char **url_ptr)
{
   printf("In ParseFirstLine\n");

  char *tokens[100]={NULL};
  //Split切分完毕后，就会破坏掉原有的字符串，把其中的分隔符替换成\0
  ssize_t n=Split(first_line," ",tokens);
  if(n!=3)
  {
    printf("first_line Split error! n=%ld\n",n);
    return -1;
  }
  //TODO 验证tokens[2]是否包含HTTP这样的关键字
  //返回结果
  *method_ptr=tokens[0];
  *url_ptr=tokens[1];
  return 0;
}


int ParseQueryString(char url[],char **url_path_ptr,char **query_string_ptr)
{
  printf("In ParseQueryString\n");
  //此处url我们没有考虑带域名的情况，自己在实现时可以加上这一部分
  *url_path_ptr=url;
  char *p=url;
  while(*p!='\0')
  {
    //若在url中找到了 ？，说明带有query_string,先把？替换成 \0,再让query_string指向？的后一个字符
    if(*p=='?')
    {
      *p='\0';
      *query_string_ptr=p+1;
      return 0;
    }
    p++;
  }
  //若循环结束也没找到？，就认为url中不存在query_string,就让其指向NULL
  *query_string_ptr=NULL;
  printf("url_path:%s\n",*url_path_ptr);
  return 0;
}

int HandlerHeader(int new_sock,int *content_length_ptr)
{
  printf("In HandlerHeader\n");
  char buf[SIZE]={0};
  while(1)
  {
    //此处缓冲区的长度不用减一，因为在readline函数中我们已经考虑了\0的问题
    if(ReadLine(new_sock,buf,sizeof(buf)<0))
    {
      printf("ReadLine failed!");
      return -1;
    }
    if(strcmp(buf,"")==0)
    {
      //读到了空行，header结束
      return 0;
    }

    //Content-Length:10
    const char* content_length_str="Content-Length";
    if(strncmp(buf,"Content-Length",strlen(content_length_str))==0)
    {
      *content_length_ptr=atoi(buf+strlen(content_length_str));
      //此处代码不应该直接return，本函数其实有两重含义：
      //1.找到content_length的值
      //2.把接收缓冲区中收到的数据都读取出来，也就是从缓冲区中删除掉，避免粘包问题
      //return 0;
    }
    sleep(1);
    printf("buf=%s\n",buf);
  }//end while(1)
}



int IsDir(const char* file_path)
{
  struct stat st;
  int ret=stat(file_path,&st);
  if(ret<0)
  {
    return 0;
  }
  if(S_ISDIR(st.st_mode))
  {
    return 1;
  }
  return 0;
}

//通常的url：http://www.baidu.com/index.html
//服务器看到的路径，也有很多情况下就是index.html,此处我们只处理这种情况
void HandlerFilePath(const char* url_path,char file_path[])
{
  //./wwwroot这个是我们随便起的名字，此处对于http服务器的根目录，名字没有明确规定
  //当前服务器要暴露给客户端的文件必须都放在这个目录下
  sprintf(file_path,"./wwwroot%s",url_path);

  //对于url_path还有几种特殊的情况：
  //1.若url中没有写路径，默认是 /，（HTTP服务器的根目录）
  //2.url后面写了路径，但对应的路径是一个目录，
  //  就尝试访问该目录下的index.html 文件（index.html作为一个默认的入口文件）
  
  //如果url_path以 / 结尾，说明一定是一个目录
  if(url_path[strlen(url_path)-1]=='/')
  {
    strcat(file_path,"index.html");//将index.html拼接在后面
  }
  
  //如果最后一个字符不是 / ，但仍然是一个目录
  //这时我们就要识别他到底是一个文件还是目录
  if(IsDir(file_path))
  {
    strcat(file_path,"index.html");
  }

  printf("file_path:%s\n",file_path);
  return;
}

//获取文件大小
int GetFileSize(const char*file_path)
{
  struct stat st;
  if(stat(file_path,&st)==0)
  {
    return st.st_size;
  }
  return 0;
}

//将文件写到静态页面中
int WriteStaticFile(int new_sock,const char* file_path)
{
  printf("write static file\n");
  printf("file_path:%s\n",file_path);

  //打开失败很可能是文件本身不存在
  int fd=open(file_path,O_RDONLY);
  if(fd<0)
  {
    perror("open");
    return 404;
  }

  size_t file_size=GetFileSize(file_path);
  printf("file_size=%u\n",file_size);


  //向socket中写的数据其实应该是一个HTTP响应
  const char* first_line="HTTP/1.1 200 OK\n";

  //返回的header应该包含两部分内容：
  //1.content-type可省略，浏览器能自动识别数据类型
  //2.content-length，也可以省略，因为紧接着就会关闭socket
  const char* blank_line="\n";
  int i;
  i=send(new_sock,first_line,strlen(first_line),0);
  if(i<0)
  {
    perror("send1");
  }
  i=send(new_sock,blank_line,strlen(blank_line),0);
  if(i<0)
  {
    perror("send2");
  }

  //body部分即是file_path这个文件，这个文件可能非常大，
  //我们要完成 内核-->用户-->内核 这样的拷贝开销一定很大
  //为了解决这个问题，我们借助一个函数sendfile
  //它可以完成在内核中向一个socket中写数据，节省开销
  i=sendfile(new_sock,fd,NULL,file_size);
  if(i<0)
  {
    perror("sendfile\n");
  }
  printf("write complete\n");
  close(fd);
  return 200;
}

int HandlerStaticFile(int new_sock,const HttpRequest* req)
{
  //1.根据上面解析出的url_path，获取到对应的真实文件路径
  char file_path[SIZE]={0};
  HandlerFilePath(req->url_path,file_path);
  //2.打开文件，把文件中的内容读取出来，并写入socket  
  int err_code=WriteStaticFile(new_sock,file_path);
  return err_code;
}


//处理父进程逻辑
void HandlerCGIFather(int new_sock,int child_pid,int father_read,int father_write,const HttpRequest* req)
{
  //1.对POST把body中的数据写到管道中
  char c='\0';
  if(strcasecmp(req->method,"POST")==0)
  {
    //从socket中读出数据写到管道中，这里不能使用sendfile，
    //因为它只能把数据写到socket中
    //所以我们直接一字节个一个字节从socket中读出来再写到管道中去
    ssize_t i=0;
    for(;i<req->content_length;i++)
    {
      read(new_sock,&c,1);
      write(father_write,&c,1);
    }
  }

  //2.父进程需要构造一个完整的HTTP协议数据
  //  对于HTTP协议而言需要我们按照指定格式返回数据
  //  对于CGI只要求CGI程序返回的结果是body部分
  //  而header、首行等都要父进程自己构造
  const char *first_line="HTTP/1.1 200 OK\n";

  //Content-Length和Content-type此处就都省略了
  const char *blank_line="\n";
  send(new_sock,first_line,strlen(first_line),0);
  send(new_sock,blank_line,strlen(blank_line),0);

  //3.从管道中尝试读取数据，协会到socket中
  //  father_read对应的是child_write,对父进程来说，child_write已经关闭了
  //  对子进程来说，若CGI程序处理完，进程也就退出了，进程退出同时也就会关闭child_write
  //  此时意味着所有管道的写端都关闭，再尝试读，read返回0
  while(read(father_read,&c,1)>0)
  {
    write(new_sock,&c,1);
  }

  //4.进行进程等待
  //此处不应该使用wait，因为我们的服务器会给每个客户端都创建一个线程，
  //每个线程又很可能创建子进程，此时如果用wait，任何一个子进程结束都可能导致wait返回，
  //这样的话子进程就不是由对应的线程来回收，严谨期间，我们用witpid来配对回收
  //wait();
  waitpid(child_pid,NULL,0);

}



//处理子进程逻辑
void HandlerCGIChild(int child_read,int child_write,const HttpRequest* req)
{
  //1.创建环境变量：REQUEST_METHOD QUERY_STRING CONTENT_LENGTH
  char method_env[SIZE]={0};
  sprintf(method_env,"REQUEST_METHOD=%s",req->method);
  putenv(method_env);

  if(strcasecmp(req->method,"GET")==0)
  {
    //设置QUERY_STRING
    char query_string_env[SIZE]={0};
    sprintf(query_string_env,"QUERY_STRING=%s",req->query_string);
    putenv(query_string_env);
  }
  else
  {
    //设置CONTENT_LENGTH
    char content_length_env[SIZE]={0};
    sprintf(content_length_env,"query_string_env=%d",req->content_length);
    putenv(content_length_env);
  }

  //2.把子进程的标准输入和输出重定向到管道上
  dup2(child_read,0);
  dup2(child_write,1);

  //3.进程的程序替换
  //  通过url_path拼装成一个完整的路径，把对应的文件进行替换
  char file_path[SIZE]={0};
  HandlerFilePath(req->url_path,file_path);

  //采用execl的原因有这么几点：
  // a）此处CGI程序不需要指定命令行参数，l/v不太影响
  // b）此处CGI程序的全路径是知道的，就不用p了
  // c）此处CGI程序环境变量在在子进程中通过putenv的方式进行设定
  //    若使用execle的话，就不必使用putenv了，这里两个方式都可以
  execl(file_path,file_path,NULL);

  //4.如果execl执行失败，需要进行错误处理，如果不退出子进程
  //  就会出现子进程和父进程监听相同端口号的情况，而此时我们只希望
  //  子进程去调用CGI程序调用CGI程序，处理客户端连接应该由父进程来完成
  exit(0);
}


//处理CGI程序
int HandlerCGI(int new_sock,const HttpRequest* req)
{
  int err_code=200;
  //1.创建一对管道
  int fd1[2],fd2[2];
  pipe(fd1);
  pipe(fd2);
  int father_read=fd1[0];
  int father_write=fd2[1];
  int child_read=fd2[0];
  int child_write=fd1[1];
  //2.创建子进程
  pid_t pid=fork();
  if(pid>0)
  {
    //父进程逻辑
    close(child_read);
    close(child_write);
    HandlerCGIFather(new_sock,pid,father_read,father_write,req);
  }
  else if(pid==0)
  {
    //子进程逻辑
    close(father_read);
    close(father_write);
    HandlerCGIChild(child_read,child_write,req);
  }
  else
  {
    perror("fork");
    err_code=404;
    //goto END;
  }
  return err_code;
}

//构造一个错误处理页面，严格遵守HTTP响应格式
int Handler404(int new_sock)
{
  printf("new_sock : %d\n",new_sock);
  printf("In Handler404\n");
  const char* first_line="HTTP/1.1 404 Not Found\n";
  const char* blank_line="\n";
  //body部分的内容就是HTML
  const char* body="<head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"></head>"
                   "<h1>您的页面被喵星人吃掉了！！！</h1>";
  char content_length[SIZE]={0};
  sprintf(content_length,"Content-Length:%lu\n",strlen(body));

  int ret = send(new_sock,first_line,strlen(first_line),0);
  if(ret < 0)
  {
    printf("ERROR\n");
  }
  send(new_sock,content_length,strlen(content_length),0);
  send(new_sock,blank_line,strlen(blank_line),0);
  send(new_sock,body,strlen(body),0);
  printf("Handler404 complete!-----%d------%s----\n",__LINE__,__FUNCTION__);
  return 0;
}



//这个函数才是真正的完成一次请求处理过程的函数
////这样拆分是为了以后的扩展，若改成多进程或IO多路复用的版本
//直接调用这个函数就可以了
void HandlerRequest(int new_sock)
{

  printf("new_sock:%d,----%d----%s----\n",new_sock,__LINE__,__FUNCTION__);
  //1.读取请求并解析
  HttpRequest req;
  int err_code=200;
  //  a）从socket中读取http请求的首行
  memset(&req,0,sizeof(req));//若对性能有所要求，我们采取只将字符数组中的0号元素设为0
  if(ReadLine(new_sock,req.first_line,sizeof(req.first_line)-1)<0)//将首行读到first_line中  
  {
    printf("ReadLine first_line failed!\n");
    // 对于错误处理，统一返回404
    err_code=404;
    goto END;
  }
  printf("first_line=%s\n",req.first_line);

  //  b）解析首行，获取到方法，url，版本号（不用）
  if(ParseFirstLine(req.first_line,&req.method,&req.url)<0)//输出型参数需要取地址
  {
    printf("ParseFirstLine failed! firse_line=%s\n",req.first_line);
    err_code=404;
    goto END;
  }

  //  c）对url再进行解析，解析出其中的url_path,query_string
  if(ParseQueryString(req.url,&req.url_path,&req.query_string)<0)
  {
    
    printf("ParseQueryString failed! url=%s\n",req.url);
    err_code=404;
    goto END;
  }

  //  d）读取并解析header部分（此处为了简单，只保留content_length,其他的header内容直接丢弃）
  if(HandlerHeader(new_sock,&req.content_length)<0)
  {
    printf("HandlerHeader failed!\n");
    err_code=404;
    goto END;
  }

  //2.根据请求的详细情况执行静态页面逻辑还是动态页面逻辑
  //  a）如果是GET请求，并且没有query_string,就认为是静态页面
  //  b）如果是GET请求，还有query_string,就可以根据query_string参数内容来动态计算生成页面）
  //  c）如果是POST请求（登录页面），无论有没有query_string，都认为是动态页面
  //  
  //  假设浏览器传过来的HTTP请求中的方法叫做get，Get，geT，此时就要使用strcasecmp
  if(strcasecmp(req.method,"GET")==0&&req.query_string==NULL)//不区分大小写的字符串比较
  {
    //生成静态页面
    //err_code=HandlerStaticFile(new_sock,&req);
    err_code = 404;
  }
  else if(strcasecmp(req.method,"GET")==0&&req.query_string!=NULL)
  {
    err_code=HandlerCGI(new_sock,&req);
  }
  else if(strcasecmp(req.method,"POST")==0)
  {
    err_code=HandlerCGI(new_sock,&req);
  }
  else
  {
    printf("method not support! method=%s\n",req.method);
    err_code=404;
    goto END;
  }
END:
  //这次请求处理结束的收尾工作
  if(err_code!=200)
  {
    Handler404(new_sock);
  }
  //此处我们只考虑短连接，短连接的意思是每次客户端（浏览器）给服务器发送请求之前，
  //都是新建立一个socket进行连接，对短连接来说，若响应写完了，就可以关闭new_sock
  //此处是服务器主动断开连接，也就会进入TIME_WAIT状态，由于服务器短时间内处理了大量的连接
  //导致服务器上出现大量的TIME_WAIT,需要设置setsockopt REUSEADDR来重用TIME_WAIT状态的连接
  close(new_sock);//服务器处理完一次请求并响应给客户端后就可以关闭new_sock了，不需要等待客户端来关闭
  
}

//线程入口函数，负责这一次请求的完整过程
void* ThreadEntry(void* arg)
{
  int new_sock=(int64_t)arg;
  printf("new_sock:%d,----%d----%s----\n",new_sock,__LINE__,__FUNCTION__);
  HandlerRequest(new_sock);
  return NULL;
}

//启动服务器
void HttpServerStart(const char* ip,short port)
{
  //1.创建tcpsocket
  int listen_sock=socket(AF_INET,SOCK_STREAM,0);

  if(listen_sock<0)
  {
    perror("socket");
    return;
  } 

  //设置REUSEADDR重用TIME_WAIT状态的连接，处理后面短连接主动关闭socket的问题
  int opt=1;//表示启动这个选项
  setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

  //2.绑定端口号
  sockaddr_in addr;
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr(ip);
  addr.sin_port=htons(port);
  int ret=bind(listen_sock,(sockaddr*)&addr,sizeof(addr));
  if(ret<0)
  {
    perror("bind");
    return;
  }
  //3.监听socket
  ret=listen(listen_sock,5);
  if(ret<0)
  {
    perror("listen");
    return;
  }
  printf("http_server init ok!\n");
  //4.进入循环，处理客户端的连接
  while(1)
  {
    sockaddr_in peer;
    socklen_t len=sizeof(peer);
    int64_t new_sock=accept(listen_sock,(sockaddr*)&peer,&len);
    if(new_sock<0)
    {
      perror("accept");
      continue;
    }
    //使用多线程的方式来完成多个连接的并行处理
    pthread_t tid;
    pthread_create(&tid,NULL,ThreadEntry,(void *)new_sock);
    printf("new_sock:%d,----%d----%s----\n",new_sock,__LINE__,__FUNCTION__);
    //pthread_detach(tid);
  }
}
//创建新线程后

int main(int argc, char* argv[]) 
{
  if(argc!=3)
  {
    printf("Usage: ./http_server [ip] [port]\n");
    return 1;
  }
  HttpServerStart(argv[1],atoi(argv[2]));
  return 0;
}

