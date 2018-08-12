///////////////////////////////////////////////////////
// 基于 mysql api 查找出数据库指定表中的所有数据
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

