#ifndef SINGLETON_INCLUDED
#define SINGLETON_INCLUDED

template <class C>
class Singleton
{
 protected:
  Singleton<C>();
  ~Singleton<C>();

 public:

  static C& GetInstance()
  {
   static C Instance;
   return Instance;
  }
};
#endif SINGLETON_INCLUDED
