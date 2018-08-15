#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUF_SZ 1024
#define PACK_LEN 56
int sendnum=0;
int recvnum=0;
char sendpack[BUF_SZ];
char recvpack[BUF_SZ];

long diftime(const struct timeval *end,const struct timeval* begin)
{
  return (end->tv_sec-begin->tv_sec)*1000+(end->tv_usec-begin->tv_usec)/1000;
}


unsigned short chksum(unsigned short* addr,int len)
{
  unsigned int ret=0;
  while(len>0)
  {
    ret+=*addr++;
    len-=2;
  }
  if(len==1)
  {
    ret+=*(unsigned char*)addr;
  }
  ret=(ret>>16)+(ret&0xffff);
  ret+=(ret>>16);

  return (unsigned short)~ret;
}

int pack(int num,pid_t pid)
{
  struct icmp* p=()
}

void send_packet(int sfd,pid_t pid,struct sockaddr_in ad,)

int main(int argc,char* argv[])
{
  if(argc!=2)
  {
    return 1;
  }

  struct in_addr addr;
  struct sockaddr_in ad;
  
  if((addr.s_addr=inet_addr(argv[1]))==INADDR_NONE)
  {
    struct hostent *pent =gethostbyname(argv[1]);
    if(pent==NULL)
    {
      perror("gethostbyname");
      exit(1);
    }
    memcpy((char*)&addr,(char*)pent->h_addr,pent->h_length);
    printf("%s\n",inet_ntoa(addr));

    int sfd=socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(sfd==-1)
    {
      perror("socket");
      exit(1);
    }
    
    ad.sin_family=AF_INET;
    ad.sin_addr=addr;
    pid_t pid=getpid();
    while(1)
    {
      send_packet(sfd,pid,ad);
      recv_packet(sfd,pid);
      sleep(1);
    }
}
