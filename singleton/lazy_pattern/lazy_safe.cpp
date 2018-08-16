
#include <iostream>
#include <pthread.h>
using namespace std;

//---------------------------lazy_pattern------------------
template<class T>
class Singleton_lazy
{
  public:
    //初始化互斥锁
    static pthread_mutex_t lock;

    static T* GetInstance()
    {
      if(_inst==NULL)
      {
        pthread_mutex_lock(&lock);
        if(_inst==NULL)
        {
          _inst=new T;
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
    static T* _inst;
    //防拷贝
    Singleton_lazy(const T&);
    T& operator=(const T&);
};

//main函数执行前先将该实例初始化为空，运行时再加载
template<class T>
T* Singleton_lazy<T>::_inst=NULL;
template<class T>
pthread_mutex_t Singleton_lazy<T>::lock;


//-----------------------------TestCode------------------------
int main()
{
  char* p1=Singleton_lazy<char>::GetInstance();
  char* p2=Singleton_lazy<char>::GetInstance();
  cout<<"p1----->"<<(void*)p1<<endl;
  cout<<"p2----->"<<(void*)p2<<endl;
  return 0;
}
