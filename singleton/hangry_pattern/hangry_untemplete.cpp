
#include <iostream>
#include <assert.h>
using namespace std;

//------------------------hangry_pattern-------------------------------------
//通过类模板来实现
class Singleton_hangry
{
  public:
    //该函数用来获取唯一实例
    static Singleton_hangry* GetInstance()
    {
      assert(_inst);
      return _inst;
    }
  protected:
    //构造函数定义为保护的，在其派生类中也可见
    Singleton_hangry(){};
  private:
    //由于静态成员函数不能调用非静态的成员变量，所以声明成静态的类对象指针
    static Singleton_hangry* _inst;
    //拷贝构造和运算符重载函数定义为私有，目的是防拷贝
    Singleton_hangry(const Singleton_hangry &);
    Singleton_hangry& operator=(const Singleton_hangry&);
};

//静态成员变量要在类外进行初始化
//初始化时创建唯一实例，在main函数调用前就会执行
//空间换时间的做法
Singleton_hangry* Singleton_hangry::_inst=new Singleton_hangry;

//-------------------Singleton_hangryestCode----------------------------------------------------
int main()
{
  Singleton_hangry* p1=Singleton_hangry::GetInstance();
  Singleton_hangry* p2=Singleton_hangry::GetInstance();
  cout<<"p1==>"<<p1<<endl;
  cout<<"p2==>"<<p2<<endl;
  return 0;
}

