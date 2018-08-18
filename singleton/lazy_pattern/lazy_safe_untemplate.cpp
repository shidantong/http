
#include <iostream>
#include <pthread.h>
using namespace std;

//---------------------------lazy_pattern------------------
class Singleton_lazy
{
  public:
    //初始化互斥锁
    static pthread_mutex_t lock;

    static Singleton_lazy* GetInstance()
    {
      if(_inst==NULL)
      {
        pthread_mutex_lock(&lock);
        if(_inst==NULL)
        {
          _inst=new Singleton_lazy;
        }
        pthread_mutex_unlock(&lock);
      }
      return _inst;
    }
  protected:
    //构造函数定义成保护为了在继承时派生类可见
    Singleton_lazy(){};

  private:
    //唯一实例
    static Singleton_lazy* _inst;
    //防拷贝
    Singleton_lazy(const Singleton_lazy&);
    Singleton_lazy& operator=(const Singleton_lazy&);
};

//main函数执行前先将该实例初始化为空，运行时再加载
Singleton_lazy* Singleton_lazy::_inst=NULL;
pthread_mutex_t Singleton_lazy::lock;


//-----------------------------Singleton_lazyestCode------------------------
int main()
{
  Singleton_lazy* p1=Singleton_lazy::GetInstance();
  Singleton_lazy* p2=Singleton_lazy::GetInstance();
  cout<<"p1----->"<<(void*)p1<<endl;
  cout<<"p2----->"<<(void*)p2<<endl;
  return 0;
}
