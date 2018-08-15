#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
  //获取ICMP的报文信息
  int sfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
  if(sfd==-1)
  {
    perror("socket");
    exit(1);
  }

  //设置选项使其能够取到IP头部信息
  int opt=1;
  setsockopt(sfd,IPPROTO_IP,IP_HDRINCL,&opt,sizeof(opt));
  
  char buf[1500];//这里的缓冲区大小其实应该写IP数据包的最大报文长度
                  //我们为了简便直接写数据链路层的最大长度1500
  while(1)
  {
    memset(buf,0x00,sizeof(buf));
    int r=read(sfd,buf,sizeof(buf));
    if(r<=0)
      break;
    struct iphdr *iph=(struct iphdr*)buf;
    
    //获取ip头中的源和目的IP
    struct in_addr ad;
    ad.s_addr=iph->saddr;
    printf("protocol:%hhd  %s<=====",iph->protocol,inet_ntoa(ad));
    ad.s_addr=iph->daddr;
    printf("%s\n",inet_ntoa(ad));

  }

}
