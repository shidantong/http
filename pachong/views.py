#!/usr/bin/python2.7
#coding:utf-8

import urllib2
import re
from bs4 import BeautifulSoup

#----------------根据url发送request请求，获取服务器响应内容---------------
def OpenPage(url):
    #构造请求头
    Myheaders={}
    #构造request请求
    req=urllib2.Request(url,headers=Myheaders)
    #激活request请求，向服务器端发送请求
    #服务器端的响应被获取，一种类文本的对象
    f=urllib2.urlopen(req)
    #decode解码函数  encode编码函数
    data=f.read()
    #decode/encode(编码名)
    #错误处理 ignore replace
    return data.decode("GBK",errors="ignore").encode("utf-8")

def Test1():
    print OpenPage("http://www.shengxu6.com/book/2967.html")

#----------------解析主页内容，获取各个章节的url网址-------------------------
def ParseMainPage(page):
    #BeautifulSoup方法，解析服务器端响应内容，格式化
    soup=BeautifulSoup(page,"html.parser")#"html.parser"python自带的标准解析引擎
                                          #还可通过其他第三方的解析库来解析
    #soup.find_all()查找全部符合条件的内容
    #查找所有的herf属性包含read的结果
    #re.compile 匹配  构建一个 Pattern对象
    GetUrl=soup.find_all(href=re.compile("read"))#返回一个列表
    #每个元素都是类的实例化对象
    UrlList=[]
    for item in GetUrl:
        UrlList.append("http://www.shengxu6.com"+item["href"])
    return UrlList
    #return ["http://www.shengxu6.com"+item["href"] for item in GetUrl]

def Test2():
    page=OpenPage("http://www.shengxu6.com/book/2967.html")
    List=ParseMainPage(page)
    print List

#------------------解析章节内容，获取标题和正文----------------------------
def ParseDetailPage(page):
    #解析章节内容
    soup=BeautifulSoup(page,"html.parser")
    #获取章节标题
    title=soup.find_all(class_="panel-heading")[0].get_text()
    #获取章节正文
    #get_text()方法用来取标签内部包含的内容
    content=soup.find_all(class_="content-body")[0].get_text()
    return title,content[:-12]

def Test3():
    page=OpenPage("http://www.shengxu6.com/read/2967_2008180.html")
    print ParseDetailPage(page)

#吧获取到的内容保存到txt文件
def WriteDataToFile(data):
    #上下文管理器，用来解决忘记关闭的问题
    with open("output.txt","a+") as f:
        f.write(data)
    #f=open("output.txt","a+")
    #f.write(data)
    #f.close()

def Test4():
    WriteDataToFile("")


if __name__=="__main__":
    url="http://www.shengxu6.com/book/2967.html"
    #获取主页响应内容
    MainPage=OpenPage(url)
    #解析主页内容，获取各个章节的url
    UrlList=ParseMainPage(MainPage)
    for item in UrlList:
        print "Clone url="+item
        #遍历获得各个章节的响应内容
        detailPage=OpenPage(item)
        #解析各个章节内容，获取标题和正文
        title,content=ParseDetailPage(detailPage)
        #把标题和正文组合起来
        data="\n\n\n"+title+"\n\n\n"+content
        WriteDataToFile(data.encode("utf-8"))
    print "Clone Finish"









