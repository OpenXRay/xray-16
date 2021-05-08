#pragma once
class IPhysicsShellHolder;
class IPHCapture
{
public:
    virtual bool Failed() = 0;
    virtual void RemoveConnection(IPhysicsShellHolder* O) = 0;
    virtual void Release() = 0;

protected:
    virtual ~IPHCapture() = 0;
};

inline IPHCapture::~IPHCapture() = default;

class CPHCharacter;
struct NearestToPointCallback;
XRPHYSICS_API IPHCapture* phcapture_create(
    CPHCharacter* ch, IPhysicsShellHolder* object, NearestToPointCallback* cb /*=0*/);
XRPHYSICS_API IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object, u16 element);
XRPHYSICS_API void phcapture_destroy(IPHCapture*& c);
