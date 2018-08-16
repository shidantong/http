#!/usr/bin/python2.7
#coding=utf-8
import urllib2
from bs4 import BeautifulSoup
import sys
import json

#-------------------------------打开主页--------------------------------------------------
def OpenPage(url):
    Myheaders={}
    req=urllib2.Request(url,headers=Myheaders)
    f=urllib2.urlopen(req)
    data=f.read()
    return data

#解析主页面，返回html格式的主页
def test1():
    url="http://jy.51uns.com:8022/Pro_StudentEmploy/StudentJobFair/Zhaoping_Detail.aspx?JobId=b7986ae4f3f94e0bb79677c68b871e30"
    print OpenPage(url)

#通过F12，在network中找到ashx的url，异步交互的url，浏览器一般需要二次获取数据
def test2():
    url="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534237504468&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=15&DateType=999&InfoState=1&WorkType=0&CompanyKey="
    print OpenPage(url)


#-------------------------解析主页数据的url，从其中获得各公司id拼接成完整的url--------------------------------------------------
def ParseMainPage(page):
    #json.dumps()转化数据成json格式
    #json.loads()转化json数据成python格式
    data=json.loads(page)#data是一个大字典
    rows=data["rows"]#获得字典中键为rows对应的值，其值是一个列表，列表中的每个元素都是一个字典，对应每个公司的招聘信息

    #由于每个公司具体招聘详情页的url处JobID字段不同，其他都相同
    #所以我们可以得到每个公司的JobID，与前面相同部分拼接起来，就能得到每个公司的url
    preStr="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534310056429&fn=GetOneZhaopin&StartDate=2000-01-01&JobId="
    urlList=[]
    for item in rows:
        urlList.append(preStr+item["Id"])
    return urlList
    #return [urlStr+item["Id"] for item in rows]


def Test3():
    url="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534163378627&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=15&DateType=999&InfoState=1&WorkType=0&CompanyKey="
    page=OpenPage(url)
    print ParseMainPage(page)

#-----------------------解析招聘信息详情页-----------------------------------------------------
def ParseDetailPage(page):
    #data是一个大字典，我们要获得键为data的数据
    data=json.loads(page)
    if data["Succeed"]==False:
        print "error"
        return
    #获得的Data部分也是一个字典
    data=data["Data"]
    #我们从该字典中得到EmployContent对应的信息
    detail=data["EmployContent"]
    #解析这些信息
    soup=BeautifulSoup(detail,"html.parser")

    #查找所有的p标签
    getP=soup.find_all("p")
    #得到各个p标签中的内容,并保存在一个列表中
    content=[item.get_text() for item in getP]

    #打印各个p标签中的内容
    for item in content:
        print item

    content="\n".join(content)
    return data["Id"],data["CompanyTitle"],data["WorkPositon"],content
   # return data.encode("utf-8")
   # return data

def Test4():
    url="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534310056429&fn=GetOneZhaopin&StartDate=2000-01-01&JobId=b7986ae4f3f94e0bb79677c68b871e30"
    page=OpenPage(url)
    jobid,title,position,content=ParseDetailPage(page)
    print jobid
    print title
    print position
    print content
   # print ParseDetailPage(page)
   # return ParseDetailPage(page)
#------------------------------将获得的数据写入文件----------------------------
def WriteDataToFile(data):
    f=open("output.txt","a+")
    for item in data:
        item=item.encode("utf-8")
        f.write(item)
    f.close

def Test5():
    url="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534310056429&fn=GetOneZhaopin&JobId=b7986ae4f3f94e0bb79677c68b871e30&StartDate=2000-01-01"
    page=OpenPage(url)
    data=ParseDetailPage(page)
    WriteDataToFile(data)

#--------------------------------将数据写入数据库--------------------------------
def WriteDataToMySql(data):
    db = MySQLdb.connect(host="localhost", user="root", passwd="1", db="Job", charset="utf8")
    cursor = db.cursor()
    # base64 编码可以保证将中文和特殊字符的内容存进数据库
    # content_0 = base64.b64encode(data[0])
    # content_1 = base64.b64encode(data[1])
    # content_2 = base64.b64encode(data[2])
    # content_3 = base64.b64encode(data[3])
    content_0 = data[0].encode("utf-8")
    content_1 = data[1].encode("utf-8")
    content_2 = data[2].encode("utf-8")
    content_3 = data[3].encode("utf-8")
    # 构建sql语句
    # sql = "insert into ClawerSchool values('%s', '%s', '%s', '%s')" % (data[0], data[1], data[2], data[3])
    sql = "insert into ClawerSchool values('%s', '%s', '%s', '%s');" % (content_0, content_1, content_2, content_3)
    print "sql = " + sql
    try:
        # 执行sql语句
        cursor.execute(sql)
        db.commit()
    except Exception, e:
        # 插入失败时，为了保证数据库的原子性，出错进行回滚
        db.rollback()


#------------------------------主函数-----------------------------------

if  __name__=="__main__":
    #根据主页ajax，获取数据url，获取服务器端响应的招聘信息
    url="http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534237504468&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=15&DateType=999&InfoState=1&WorkType=0&CompanyKey="

    mainPage=OpenPage(url)

    #分析服务器端响应，得到招聘信息详情页的数据获取url
    urlList=ParseMainPage(mainPage)
    for item in urlList:
        print "crawler url="+item
        #获取招聘信息详情
        detailPage=OpenPage(item)
        #解析
        data=ParseDetailPage(detailPage)
        WriteDataToFile("\n".join(data))
    print "crawler done"




















