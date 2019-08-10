#pragma once
#include "xrCommon/xr_vector.h"

// messages
#define REG_PRIORITY_LOW 0x11111111ul
#define REG_PRIORITY_NORMAL 0x22222222ul
#define REG_PRIORITY_HIGH 0x33333333ul
#define REG_PRIORITY_CAPTURE 0x7ffffffful
#define REG_PRIORITY_INVALID 0xfffffffful

// XXX: rename to FrameStart
struct pureFrame
{
    virtual void OnFrame() = 0;
    ICF void __fastcall OnPure() { OnFrame(); }
};

struct pureFrameEnd
{
    virtual void OnFrameEnd() = 0;
    ICF void __fastcall OnPure() { OnFrameEnd(); }
};

struct pureRender
{
    virtual void OnRender() = 0;
    ICF void __fastcall OnPure() { OnRender(); }
};

struct pureAppActivate
{
    virtual void OnAppActivate() = 0;
    ICF void __fastcall OnPure() { OnAppActivate(); }
};

struct pureAppDeactivate
{
    virtual void OnAppDeactivate() = 0;
    ICF void __fastcall OnPure() { OnAppDeactivate(); }
};

struct pureAppStart
{
    virtual void OnAppStart() = 0;
    ICF void __fastcall OnPure() { OnAppStart(); }
};

struct pureAppEnd
{
    virtual void OnAppEnd() = 0;
    ICF void __fastcall OnPure() { OnAppEnd(); }
};

struct pureDeviceReset
{
    virtual void OnDeviceReset() = 0;
    ICF void __fastcall OnPure() { OnDeviceReset(); }
};

struct pureScreenResolutionChanged
{
    virtual void OnScreenResolutionChanged() = 0;
    ICF void __fastcall OnPure() { OnScreenResolutionChanged(); }
};

template<class T>
class MessageRegistry
{
    struct MessageObject
    {
        T* Object;
        int Prio;
    };

    bool changed, inProcess;
    xr_vector<MessageObject> messages;

    static int __cdecl compare(const void* e1, const void* e2)
    {
        MessageObject* p1 = (MessageObject*)e1;
        MessageObject* p2 = (MessageObject*)e2;
        return p2->Prio - p1->Prio;
    }

public:
    MessageRegistry() : changed(false), inProcess(false) {}

    void Clear() { messages.clear(); }

    constexpr void Add(T* object, const int priority = REG_PRIORITY_NORMAL)
    {
        Add({ object, priority });
    }

    void Add(MessageObject&& newMessage)
    {
#ifdef DEBUG
        VERIFY(newMessage.Object);
        VERIFY(newMessage.Prio != REG_PRIORITY_INVALID);

        // Verify that we don't already have the same object with valid priority
        for (size_t i = 0; i < messages.size(); ++i)
        {
            auto& message = messages[i];
            VERIFY(!(message.Prio != REG_PRIORITY_INVALID && message.Object == newMessage.Object));
        }
#endif
        messages.emplace_back(newMessage);

        if (inProcess)
            changed = true;
        else
            Resort();
    }

    void Remove(T* object)
    {
        for (size_t i = 0; i < messages.size(); ++i)
        {
            auto& message = messages[i];
            if (message.Object == object)
                message.Prio = REG_PRIORITY_INVALID;
        }

        if (inProcess)
            changed = true;
        else
            Resort();
    }

    void Process()
    {
        if (messages.empty())
            return;

        inProcess = true;

        if (messages[0].Prio == REG_PRIORITY_CAPTURE)
            messages[0].Object->OnPure();
        else
        {
            for (size_t i = 0; i < messages.size(); ++i)
            {
                auto& message = messages[i];
                if (message.Prio != REG_PRIORITY_INVALID)
                    message.Object->OnPure();
            }
        }

        if (changed)
            Resort();

        inProcess = false;
    }

    void Resort()
    {
        if (!messages.empty())
            qsort(&messages.front(), messages.size(), sizeof(MessageObject), compare);

        while (!messages.empty() && messages.back().Prio == REG_PRIORITY_INVALID)
            messages.pop_back();

        if (messages.empty())
            messages.clear();

        changed = false;
    }
};
