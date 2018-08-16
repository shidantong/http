#!/usr/lib/python2.7
# coding=utf-8

import urllib2
import re
from bs4 import BeautifulSoup
import json
#import MySQLdb
import base64

# 读取url信息，打开页面，将页面数据返回
def OpenPage(url):
    Myheaders = {}
    req = urllib2.Request(url, headers=Myheaders)
    f = urllib2.urlopen(req)
    data = f.read()
    return data

def Test1():
    '''
测试之后发现，编码属于utf-8不用进行解码
并且信息不完整，没有就业单位信息
这是因为，采用了异步请求,我们需要二次获取
在主页中F12查找NetWork ，刷新之后，发现本次总共访问的请求
在最后几个xhr就为动态加载的ajax请求
下面部分属于动态变化的，我们使用的url只是访问了一次，只把固定的部分爬取下来了
我们分析爬取下来的页面的html信息后，发现有如下部分信息
//单位行业
$.ajax({
这里的参数我们发现并不是一个url，这时因为这里为了安全策略，采用一种键值对的形式来进行访问

url: getCommonDataUrl('GetCompanyTypes'),
type: 'GET',
async: true,
data: "",
dataType: 'json',
success: function (ajaxResult) {
if (ajaxResult) {
    $('#SearchCompanyType').empty();
    $('#SearchCompanyType').append("<option value=>--请选择--</option>");
    $.each(ajaxResult, function (index, obj) {
    $('#SearchCompanyType').append("<option value=" + obj.value + ">" + obj.text + "</option>");

        });
      }
    }
});

    '''
    print OpenPage("http://jy.51uns.com:8022/Pro_StudentEmploy/StudentJobFair/Zhaoping.aspx?WorkType0")

def Test2():
    '''
    http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534034231300&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=15&DateType=999&InfoState=1&WorkType=0&CompanyKey=
    我们发现抓取下来的信息是该页面内的招聘详情（15个,刚好为一页有15个的公司的信息limite字段限制了）中有一个Id字段，当我们点开一个具体的详情发现
    "Id":"b7986ae4f3f94e0bb79677c68b871e30"这里取了第一个
    http://iy.51nns.com:8022/Pro_StudentEmploy/StudentJobFair/Zhaoping_Detail.aspx?JobId=b7986ae4f3f94e0bb79677c68b871e30
    也有一个id字段,发现每一个都是相互对应的
    我们分析处每一个具体的招聘详情都是根据id字段来进行区分的，然而这个id可以根据我们自己在异步ajax请求中可以查找出来
    就像上面的我们发现将limite字段修改之后，就可以将所有的id获取到，这样就可以根据拼接的url进行访问每一个招聘信息的页面
    '''
    print OpenPage("http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534034231300&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=15&DateType=999&InfoState=1&WorkType=0&CompanyKey=")
#
def ParseMainPage(page):
    '''
    json.dumps()
    将数据转换为json格式
    将json格式的数据转化为python数据
    json.loads()
    '''
    data = json.loads(page)
    #  data 为大的字典，rows是招聘的信息数据
    rows = data["rows"]
    # rows 为一个保存了多个招聘信息的list
    # list 中每一个元素都是一条招聘信息，保存为字典类型
#     prefix = "http://jy.51uns.com:8022/Pro_StudentEmploy/StudentJobFair/Zhaoping_Detail.aspx?JobId="
    # prefix = "http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534164775674&fn=GetZhaopinList&StartDate=2000-01-01&JobId="
     # prefix = "http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534220883227&fn=GetOneZhaopin&StartDate=2000-01-01&JobId="
    prefix = "http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534220883227&fn=GetOneZhaopin&JobId="
    tail = "&StartDate=2000-01-01"
    #  将每个招聘信息的详情页面的链接获取出来，最后只是拼接上id就可以获取全部招聘信息的内容
    Idlist = []
    for item in rows:
        Idlist.append(prefix + item['Id'] + tail)
    return Idlist

def Test3():
    #  注意下面可以直接修改limite= 65*15，就可以将所有是数据加载
    page = OpenPage("http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534164775674&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=56&DateType=999&InfoState=1&WorkType=0&CompanyKey=")
    rows = ParseMainPage(page)
    print rows

def ParseDetailePage(page):
    '''
    将数据的格式转化为json格式
    每一个data为一个大的字典，我们需要拿到Data部分
    Data又是一个字典，里面有招聘详情
    '''
    data = json.loads(page)
    if data["Succeed"] == False:
        print "error"
        return
    data = data["Data"]
    detail = data["EmployContent"]
    soup = BeautifulSoup(detail, "html.parser")
    GetP = soup.find_all("p")  # 查找所有的p标签

    # data = data.decode("GBK", errors="ignore").encode("utf-8")
    content = [item.get_text() for item in GetP]

    # for item in content:
    #     print item
    content = "\n".join(content)
    return data["Id"], data["CompanyTitle"], data["WorkPositon"], content

    #return data["CompanyTitle"]
    # return soup
    # return content[0]

def Test4():
    # page = OpenPage("http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1533005501840&fn=GetOneZhaopin&JobId=b360f9f177e34d94ba1363615aabda5f&StartDate=2000-01-01")
    # 发现在一个招聘详情里面的F12发现header里面也存在异步加载的数据，我们将url获取出来
    page = OpenPage("http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534220883227&fn=GetOneZhaopin&JobId=b7986ae4f3f94e0bb79677c68b871e30&StartDate=2000-01-01")
    #jobid, title, position, content = ParseDetailePage(page)
    #print jobid
    #print title
    #print position
    #print content
    print ParseDetailePage(page)

# 写数据到文件
def WriteDataTofile(data):
    f = open("./duimianttttt.txt", "a+")
    for item in data:
        item = item.encode("utf-8")
        f.write(item)
    f.close()

def Test5():
    page = OpenPage("http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534220883227&fn=GetOneZhaopin&JobId=b7986ae4f3f94e0bb79677c68b871e30&StartDate=2000-01-01")
    data = ParseDetailePage(page)
    WriteDataTofile(data)



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


# if __name__ == "__main__":
#     url = "http://jy.51uns.com:8022/Frame/Data/jdp.ashx?rnd=1534224352671&fn=GetZhaopinList&StartDate=2000-01-01&SearchKey=&InfoType=-1&CompanyAttr=&CompanyType=&Area=&City=&CompanyProvice=&Post=&Zhuanye=&XLkey=&Age=&start=0&limit=5&DateType=999&InfoState=1&WorkType=0&CompanyKey="
#     # 根据主页ajax获取数据的url，获取服务器端的信息
#     mainPage = OpenPage(url)
#     # 分析服务器端的响应，得到招聘信息详情页的数据获取url
#     urlList = ParseMainPage(mainPage)
#     for item in urlList:
#         print "crawler url =" + item
#         detailPage = OpenPage(item)
#         data = ParseDetailePage(detailPage)
# #         WriteDataTofile(data)
#         WriteDataToMySql(data)
#     print "crawler done"

if __name__ == "__main__":
    Test4()

