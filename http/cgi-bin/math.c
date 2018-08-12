///////////////////////////////////////////////////////
// 此处的代码是一个测试用的CGI程序
///////////////////////////////////////////////////////



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int GetQueryString(char buf[])
{
  //1.从环境变量中读取方法
  char *method=getenv("REQUEST_METHOD");
  if(method==NULL)
  {
    //此时打印错误日志不能直接向标准输出中打印
    fprintf(stderr,"getenv REQUEST_METHOD failed!\n");
    return -1;
  }
  //2.若为GET方法，从环境变量中获取到QUERY_STTRING
  if(strcasecmp(method,"GET")==0)
  {
    char *p=getenv("QUERY_STRING");
    if(p==NULL)
    {
      fprintf(stderr,"getenv QUERY_STRING failed!\n");
      return -1;
    }
    strcpy(buf,p);
  }
  else
  {
    //3.若是POST方法，从环境变量中获取到CONTENT_LENGTH,根据CONTEHT_LENGTH的数值从标准输入中读取数据
    char* p=getenv("CONTENT_LENGTH");
    if(p==NULL)
    {
      fprintf(stderr,"getenv CONTENT_LENGTH failed!\n");
      return -1;
    }
    int content_length=atoi(p);
    int i=0;
    for(;i<content_length;i++)
    {
      read(0,&buf[i],1);
    }
    buf[content_length]='\0';
  }
  return 0;  
}

int main()
{
  //1.获取到必要的信息，此处需要获取到的信息，就是两个要进行相加的数字
  //  a）如果当前是GET请求，就需要解析QUERY_STRING
  //  b）如果是POST请求，就需要解析body
  //  
  char buf[1024*4]={0};

  //这个函数的功能就是自动根据GET/POST请求获取对应的参数信息，并把参数信息填到buf缓冲区中
  int ret=GetQueryString(buf);
  if(ret<0)
  {
    fprintf(stderr,"GetQueryString failed\n");
    return 1;
  }
  //  不管数据在QUERY_STRING中，还是body中，数据格式都是形如：a=10&b=20
  //  经过函数读取后，buf中的数据格式就是a=10&b=20
  fprintf(stderr,"buf=%s\n",buf);

  //实际上我们需要的内容是10 和20,此处使用sscanf完成字符串解析获取到其中的数字
  //主要是因为代码简单，但sscanf错误处理能力比较差
  int a,b;
  sscanf(buf,"a=%d&b=%d",&a,&b);

  //2.进行计算。此处的计算规则和具体的业务强相关
  int sum=a+b;
  

  //3.拼装成HTML页面，写到标准输出（此处由于CGI程序只需要生成HTTP响应中的body，所以只拼装HTML就ok）
  printf("<h1>sum=%d</h1>",sum);
  return 0;
}
