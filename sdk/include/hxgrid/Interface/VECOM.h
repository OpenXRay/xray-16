#ifndef VECOM_INCLUDED
#define VECOM_INCLUDED

#include "hxplatform.h"
#include "Singleton.h"

#ifndef TINTERFACEID
typedef unsigned int TINTERFACEID;
#endif

#ifndef TCLASSID
typedef unsigned int TCLASSID;
#endif

// IUnknown methods
#define IUNKNOWN_METHODS_PURE(InterfaceId, InterfaceVersion)    \
  virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) = 0;   \
  virtual ULONG __stdcall AddRef() = 0;   \
  virtual ULONG __stdcall Release() =0 ;  \
  typedef enum                            \
  {                                       \
   ID = ##InterfaceId,                    \
   VERSION = ##InterfaceVersion,          \
   FORCE_DWORD = 0xffffffff               \
  } desc;                                 \
  static REFIID GetREFIID() { static IID iid; iid.Data1 = InterfaceId; iid.Data2 = 0; iid.Data3 = 0; return iid; }

//for singleton classes
#define IUNKNOWN_METHODS_IMPLEMENTATION_REFERENCE()    \
  virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) {*ppv=NULL; return E_NOINTERFACE;};  \
  virtual ULONG __stdcall AddRef() {return 1;};  \
  virtual ULONG __stdcall Release() {return 1;};  

#define IUNKNOWN_METHODS_IMPLEMENTATION_REFERENCE_EXCLUDEQUERYINTERFACE()    \
	virtual ULONG __stdcall AddRef() {return 1;};  \
	virtual ULONG __stdcall Release() {return 1;};

//REVIEW: QueryInterface()
//The interface returned by QueryInterface in response to a request for IID_IUnknown (the interface 
//ID for IUnknown) should have the same pointer value at all times and from all interfaces on that 
//instance. This is not required of any other interface. This pointer value is the instance's 
//identity. The identity of an object is the way in which one compares two objects to see if they 
//are actually the same one. This can be found only by comparing the IUnknown interface pointers. 
//You can find the identity of the object behind any interface by first doing a QueryInterface for IUnknown 
//and then comparing the pointers. 

namespace VECOM
{

//==========================
//TInitRefCount
//==========================
//helper class in refcount initialization and holding
class TRefCountHolder
{
 public: 
  volatile ULONG RefCount;  
 
  TRefCountHolder()
  {
   RefCount=1;
  } 
};

} //namespace

#ifndef _XBOX
 
//for instantiated classes - tracks refcount
#define IUNKNOWN_METHODS_IMPLEMENTATION_INSTANCE()    \
  VECOM::TRefCountHolder RefCountHolder; \
  virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) {*ppv=NULL; return E_NOINTERFACE;};  \
  virtual ULONG __stdcall AddRef() {volatile ULONG* p = &RefCountHolder.RefCount; __asm {mov eax,p}; __asm{LOCK INC DWORD PTR [EAX]}; return RefCountHolder.RefCount;};  \
  virtual ULONG __stdcall Release() {volatile ULONG* p = &RefCountHolder.RefCount; __asm {mov eax,p}; __asm{LOCK DEC DWORD PTR [EAX]}; ULONG r = RefCountHolder.RefCount; if (r==0) delete this; return r;};  
  
#define IUNKNOWN_METHODS_IMPLEMENTATION_INSTANCE_EXCLUDEQUERYINTERFACE()    \
    VECOM::TRefCountHolder RefCountHolder; \
    virtual ULONG __stdcall AddRef() {volatile ULONG* p = &RefCountHolder.RefCount; __asm {mov eax,p}; __asm{LOCK INC DWORD PTR [EAX]}; return RefCountHolder.RefCount;};  \
    virtual ULONG __stdcall Release() {volatile ULONG* p = &RefCountHolder.RefCount; __asm {mov eax,p}; __asm{LOCK DEC DWORD PTR [EAX]}; ULONG r = RefCountHolder.RefCount; if (r==0) delete this; return r;};  

#else

#define IUNKNOWN_METHODS_IMPLEMENTATION_INSTANCE()    \
  VECOM::TRefCountHolder RefCountHolder; \
  virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) {*ppv=NULL; return E_NOINTERFACE;};  \
  virtual ULONG __stdcall AddRef() {RefCountHolder.RefCount++; return RefCountHolder.RefCount;};  \
  virtual ULONG __stdcall Release() {RefCountHolder.RefCount--; ULONG r = RefCountHolder.RefCount; if (r==0) delete this; return r;};  

#define IUNKNOWN_METHODS_IMPLEMENTATION_INSTANCE_EXCLUDEQUERYINTERFACE()    \
    VECOM::TRefCountHolder RefCountHolder; \
    virtual ULONG __stdcall AddRef() {RefCountHolder.RefCount++; return RefCountHolder.RefCount;};  \
    virtual ULONG __stdcall Release() {RefCountHolder.RefCount--; ULONG r = RefCountHolder.RefCount; if (r==0) delete this; return r;};  

#endif  

//factory method for creating interfaces
typedef IUnknown* (__cdecl *PInterfaceFactoryMethod)(TINTERFACEID InterfaceId, DWORD version, const void* ExData);


//======================================
// IInstanceInterfaceProvider
//======================================
//используетс€ экземпл€рами объектов дл€ регистрации себ€ 
//чтобы можно было отдавать интерфейсы, которые реализует этот экземпл€р 
//(экземпл€ры одного и того же различаютс€ по ExData)
DECLARE_INTERFACE_(IInstanceInterfaceProvider, IUnknown)
{
 IUNKNOWN_METHODS_PURE(0x78923789,0x00010000)    
 
 virtual DWORD __stdcall GetVersion() const = 0;
 
 //каждый экземпл€р объекта регистрирует этот метод в registry в конструкторе,
 //чтобы по запросу отдавать указатели на интерфейсы, которые он реализует
 //экземпл€ры распознаютс€ по Exdata
 //Ќ≈ќЅ’ќƒ»ћќ ”ƒјЋј“№ »Ќ“≈–‘≈…— »« –≈√»—“–» ¬ ƒ≈—“–” “ќ–≈ Ё «≈ћѕЋя–ј, »Ќј„≈ »—ѕќ–“»“—я ¬—я —»—“≈ћј !!!
 //рекомендуетс€ наследовать от класа TInstanceInterfaceProvider
 //в нем реализовано добавление и удаление
 virtual IUnknown* __stdcall GetInterface(TINTERFACEID InterfaceId, DWORD version, const void* ExData) = 0;
};

  

//===================================================
// class TInterfaceObject
//===================================================
//примен€етс€ при описании класа, реализующего интерфейс, создаваемого спомощью factory method
template <class InterfaceClass>
class TInterfaceObject : public InterfaceClass
{
public:
	virtual DWORD __stdcall GetVersion() const
	{
		return InterfaceClass::VERSION;
	}
}; 

#endif VECOM_INCLUDED

