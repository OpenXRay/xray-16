#pragma once
#include "xrCommon/xr_vector.h"

// messages
#define REG_PRIORITY_LOW 0x11111111
#define REG_PRIORITY_NORMAL 0x22222222
#define REG_PRIORITY_HIGH 0x33333333
#define REG_PRIORITY_CAPTURE 0x7fffffff
#define REG_PRIORITY_INVALID 0x80000000 // -2147483648, lowest for int

struct IPure
{
    virtual ~IPure() = default;
    virtual void OnPure() = 0;
};

#define DECLARE_MESSAGE(name)\
struct pure##name : IPure\
{\
    virtual void On##name() = 0;\
private:\
    void OnPure() override { On##name(); }\
};

DECLARE_MESSAGE(Frame); // XXX: rename to FrameStart
DECLARE_MESSAGE(FrameEnd);
DECLARE_MESSAGE(Render);
DECLARE_MESSAGE(AppActivate);
DECLARE_MESSAGE(AppDeactivate);
DECLARE_MESSAGE(AppStart);
DECLARE_MESSAGE(AppEnd);
DECLARE_MESSAGE(DeviceReset);
DECLARE_MESSAGE(UIReset);
DECLARE_MESSAGE(ScreenResolutionChanged);

struct MessageObject
{
    IPure* Object;
    int Prio;
};

template<class T>
class MessageRegistry
{
    bool changed, inProcess;
    xr_vector<MessageObject> messages;

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
        for (auto& message : messages)
            VERIFY(!(message.Prio != REG_PRIORITY_INVALID && message.Object == newMessage.Object));
#endif
        messages.emplace_back(newMessage);

        if (inProcess)
            changed = true;
        else
            Resort();
    }

    void Remove(T* object)
    {
        for (auto& it : messages)
        {
            if (it.Object == object)
                it.Prio = REG_PRIORITY_INVALID;
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
            for (const auto& message : messages)
                if (message.Prio != REG_PRIORITY_INVALID)
                    message.Object->OnPure();
        }

        if (changed)
            Resort();

        inProcess = false;
    }

    void Resort()
    {
        if (!messages.empty()) {
            std::sort(std::begin(messages), std::end(messages),
                [](const auto& a, const auto& b) { return a.Prio > b.Prio; });
        }

        while (!messages.empty() && messages.back().Prio == REG_PRIORITY_INVALID)
            messages.pop_back();

        if (messages.empty())
            messages.clear();

        changed = false;
    }
};
