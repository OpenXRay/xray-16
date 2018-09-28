#pragma once

#include "ICollideValidator.h"
typedef u32 CLClassBits;
typedef u32 CLBits;
class CPHObject;
class CPHCollideValidator
{
    enum ETypeFlags
    {
        cbNCGroupObject = 1 << 0,
        cbNCStatic = 1 << 1,
        cbClassDynamic = 1 << 2,
        cbNCClassDynamic = 1 << 3,
        cbClassCharacter = 1 << 4,
        cbNCClassCharacter = 1 << 5,
        cbClassSmall = 1 << 6,
        cbNCClassSmall = 1 << 7,
        cbClassRagDoll = 1 << 8,
        cbNCClassRagDoll = 1 << 9,
        cbClassAnimated = 1 << 10, //animated physical object class
        cbNCClassAnimated = 1 << 11, //ignore property of animated physical object class
        cbNone = 1 << 12

    };

public:
    static CGID RegisterGroup();
    static CGID LastGroupRegistred();
    static IC const CGID& GetGroup(const CPHObject& obj) { return obj.collide_bits(); }
    ////////////////////////////////////////////////////////////////////////////////
    static void InitObject(CPHObject& obj);
    static void RegisterObjToGroup(CGID group, CPHObject& obj);
    static void RegisterObjToLastGroup(CPHObject& obj);
    static void RestoreGroupObject(const CPHObject& obj);
    static bool IsGroupObject(const CPHObject& obj);
    static bool IsAnimatedObject(const CPHObject& obj);
    static void SetStaticNotCollide(CPHObject& obj);
    static void SetNonDynamicObject(CPHObject& obj);
    static void SetDynamicNotCollide(CPHObject& obj);
    static void SetCharacterClass(CPHObject& obj);
    static void SetCharacterClassNotCollide(CPHObject& obj);
    static void SetRagDollClass(CPHObject& obj);
    static void SetRagDollClassNotCollide(CPHObject& obj);
    static void SetAnimatedClass(CPHObject& obj);
    static void SetAnimatedClassNotCollide(CPHObject& obj);

    static void SetClassSmall(CPHObject& obj);
    static void SetClassSmallNotCollide(CPHObject& obj);
    static void Init();

    static IC bool DoCollide(const CPHObject& obj1, const CPHObject& obj2)
    {
        // switch(CollideType(obj1.collide_class_bits().flags,obj2.collide_class_bits().flags)) {
        //	case cbNCGroupObject:	return DoCollideGroup		(obj1,obj2)			;break;
        //	case 0:					return DoCollideNonMatched	(obj1,obj2)			;break;
        //	default: NODEFAULT;
        //#ifdef DEBUG
        //	return false;
        //#endif // DEBUG
        return (CollideType(obj1.collide_class_bits().flags, obj2.collide_class_bits().flags) != cbNCGroupObject ||
                   DoCollideGroup(obj1, obj2)) &&
            DoCollideNonMatched(obj1, obj2);
    }

    static IC bool DoCollideStatic(const CPHObject& obj) { return !obj.collide_class_bits().test(cbNCStatic); }
protected:
private:
    static IC bool DoCollideNonMatched(const CPHObject& obj1, const CPHObject& obj2)
    {
        return !(((obj1.collide_class_bits().flags & ClassNCFlags.flags) &
                     ((obj2.collide_class_bits().flags & ClassFlags.flags) << 1)) ||
            ((obj2.collide_class_bits().flags & ClassNCFlags.flags) &
                ((obj1.collide_class_bits().flags & ClassFlags.flags) << 1)));
    }

    static IC bool DoCollideGroup(const CPHObject& obj1, const CPHObject& obj2)
    {
        return !(obj1.collide_bits() == obj2.collide_bits());
    }

    static IC CLClassBits CollideType(CLClassBits cb1, CLClassBits cb2) { return ((cb1 & cb2) & NonTypeFlags.flags); }
    static CGID freeGroupID;
    static _flags<CLClassBits> ClassFlags;
    static _flags<CLClassBits> ClassNCFlags;
    static _flags<CLClassBits> NonTypeFlags;
};
