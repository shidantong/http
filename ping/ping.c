#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#define SIZE 1024 //缓冲区大小
#define PACK_LEN 56//数据大小

int sendnum=0;//发送包的序号
int recvnum=0;//接受包的序号
char sendpack[SIZE];//发送包的缓冲区
char recvpack[SIZE];//接受包的缓冲区

//计算rtt（ms）
double diftime(const struct timeval *end,const struct timeval *begin)
{
  return (double)(end->tv_sec-begin->tv_sec)*1000+(end->tv_usec-begin->tv_usec)/1000;
}

//求校验和
unsigned short chksum(unsigned short* addr,int len)
{
  //1.先将校验和字段置为0
  unsigned short ret=0;

  //2.对需要校验的字段看成16bit为单位的数字组成
  //  依次进行求和
  while(len>0)
  {
    ret+=*addr++;
    len-=2;
  }
  if(len==1)
  {
    ret+=*(char*)addr;
  }
  ret=(ret>>16)+(ret&0xffff);
  ret+=ret>>16;

  //最后将结果取反存入校验和字段
  return (unsigned short)~ret;
}

//----------------------------组包并发送------------------------------------

//组包
int pack(pid_t pid)
{
  memset(sendpack,0x00,sizeof(sendpack));
  struct icmp* p = (struct icmp*)sendpack;
  p->icmp_type=ICMP_ECHO;//回显请求报文类型
  p->icmp_code=0;//回显请求报文代码为0
  p->icmp_cksum=0;//校验和先清零
  p->icmp_id=pid;//标识符
  p->icmp_seq=sendnum;//序号
  //数据部分填时间
  struct  timeval begin;
  gettimeofday(&begin,NULL);
  memcpy((void*)p->icmp_data,(void*)&begin,sizeof(begin));
  printf("pack:%d.%d\n",begin.tv_sec,begin.tv_usec);
  //计算校验和
  p->icmp_cksum=chksum((unsigned short*)sendpack,PACK_LEN+8);//加上8字节的ICMP头部
  printf("send_num=%d\n",sendnum);
  return PACK_LEN+8;
}
//发包
void send_packet(int sfd,pid_t pid,struct sockaddr_in ad)
{
  sendnum++;
  int r=pack(pid);//返回包大小
  struct icmp* p=(struct icmp*)sendpack;
  printf("send:sendnum=%d\n",p->icmp_seq);
  int ret=sendto(sfd,sendpack,r,0,(struct sockaddr*)&ad,sizeof(ad));
  if(ret<0)
  {
    perror("sendto");
  }
}


//------------------------------接收并解包-----------------------------

//解包
void unpack(pid_t pid,struct sockaddr_in* peer)
{
  //收到的数据包前面是IP头部
  struct ip* pip=(struct ip*)recvpack;
  //使指针偏移ip头部的大小指向icmp的头处
  struct icmp* picmp=(struct icmp*)(recvpack+(pip->ip_hl<<2));

  printf("unpack:%d\n",picmp->icmp_seq);
  if(pid != picmp->icmp_id)
    return;

  //拿出icmp包数据部分的时间与当前时间求差值，即为rtt
  struct timeval end;
  gettimeofday(&end,NULL);

  struct timeval *begin=(struct timeval*)(picmp->icmp_data);

  printf("%d.%d\n",begin->tv_sec,begin->tv_usec);
  printf("%d.%d\n",end.tv_sec,end.tv_usec);

  printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
          PACK_LEN+8,
          inet_ntoa(peer->sin_addr),
          picmp->icmp_seq,
          pip->ip_ttl,
          diftime(&end,(struct timeval*)picmp->icmp_data));
}

//收包
void recv_packet(int sfd,pid_t pid)
{
  printf("in recvpack\n");
  memset(recvpack,0x00,sizeof(recvpack));
  struct sockaddr_in peer;
  socklen_t len=sizeof(peer);
  int ret=recvfrom(sfd,recvpack,sizeof(recvpack),0,(struct sockaddr*)&peer,&len);
  printf("recv complete\n");
  if(ret<0)
  {
    perror("recvfrom");
  }
  recvnum++;
  unpack(pid,&peer);

  
}

//处理SIGINT信号
void handler(int s)
{
  printf("--- www.a.shifen.com ping statistics ---\n");
  printf("%d packets transmitted, %d received, %.3f%% packet loss\n",
            sendnum,recvnum,((sendnum-recvnum)/sendnum)*100);
  exit(0);
}

int main(int argc,char* argv[])
{
  if(argc!=2)
  {
    return 1;
  }

  signal(SIGINT,handler);
  struct in_addr addr;
  struct sockaddr_in ad;
  if((addr.s_addr=inet_addr(argv[1]))==INADDR_NONE)
  {
    //若不是IP地址，则通过域名转换IP地址
    struct  hostent *pent=gethostbyname(argv[1]);
    if(pent==NULL)
    {
      //域名无效
      perror("gethostbyname");
      exit(1);
    }
    memcpy((char*)&addr,(char*)pent->h_addr,pent->h_length);
    printf("%s\n",inet_ntoa(addr));
  }

  //创建一个原始套接字来收发网络层的数据包
  int sfd=socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);
  if(sfd<0)
  {
    perror("socket");
    exit(1);
  }

  //网络层不涉及端口号
  ad.sin_family=AF_INET;
  ad.sin_addr=addr;

  //进程id唯一标识
  pid_t pid=getpid();
  printf("PING %s (%s) %d bytes of data.\n",
          argv[1],
          inet_ntoa(addr),
          PACK_LEN);
  while(1)
  {
    //组包发送
    send_packet(sfd,pid,ad);
    //收包
    recv_packet(sfd,pid);
    sleep(1);
  }
}
