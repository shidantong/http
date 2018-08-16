#!/usr/bin/python2
#coding=utf-8

import urllib2
import re
from bs4 import BeautifulSoup
import json

def OpenPage(url):
    Myheaders={}
    req=urllib2.Request(url,headers=Myheaders)
    f=urllib2.urlopen(req)
    data=f.read()
    return data

def Test1():
    url="http://jy.51uns.com:8022/Pro_StudentEmploy/StudentJobFair/Zhaoping_Detail.aspx?JobId=b7986ae4f3f94e0bb79677c68b871e30"
    print OpenPage(url)

#获取各条招聘信息的url
def ParseMainPage(page):
    #将json格式转化成python自己的数据类型
    data=json.load(page)
    #解析后的Data是一个大的字典，rows是招聘信息的key
    #rows里面保存了包含多个招聘信息的list
    #list里面的每个元素都是一条招聘信息，保存为字典类型

    #获取字典内的招聘信息数组
    InfoList=data['rows']
    #for item in InfoList:
    return rows





def Test3():
    url="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534237506286&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=15&limit=15&DateType=999&InfoState=1&WorkType=0&CompanyKey="
    page=OpenPage(url)
    print ParseMainPage(page)


#获取招聘信息详情页
#招聘id，公司名，招聘岗位，招聘内容
def ParseDetailPage(page):
    data=json.load(page)
    Info=data["Data"]
    #公司名称
    Title=Info["CompanyTitle"]
    #招聘信息id
    Id=Info["Id"]
    #招聘岗位
    Position=Info["WorkPosition"]

    Detail=Info["EmployContent"]
    soup=BeautifulSoup(Detail,"html.parser")
    #获取到的是招聘内容的具体数据
    #PList=[item.get_text() for item int soup,find_all("p")]
    #
    return Info

def Test4():
    url=""
    page=OpenPage(url)


if __name__=="__main__":
    Test3()



import MySQLdb
import base64
#把数据写入Mysql数据库
def WriteDataToMySQL(data):
    db=MySQLdb.connect(host="localhost",user="root",password="123",db="Test",charset="utf8")
    database=db.cursor()
    #对招聘内容进行base64转码，排除因可能出现的特殊字符导致的数据库插入失败
    content=base64.b64encode(data[3])
    sql="insert into diytable values('%s','%s','%s','%s')" %(data[0],data[1],data[2],content)
    try:
        database.execute(sql)
        db.commit()
    except Exception,e:
        db.rollback()
        print str(e)
    db.close()

def Test5():
    testData=("10086","bite科技","班主任","颜值担当")
    WriteDataToMySQL(testData)

