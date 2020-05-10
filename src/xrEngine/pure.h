#pragma once
#include "xrCommon/xr_vector.h"

// messages
constexpr int REG_PRIORITY_LOW = 0x11111111;
constexpr int REG_PRIORITY_NORMAL = 0x22222222;
constexpr int REG_PRIORITY_HIGH = 0x33333333;
constexpr int REG_PRIORITY_CAPTURE = 0x7fffffff;
constexpr int REG_PRIORITY_INVALID = std::numeric_limits<int>::lowest();

#define DECLARE_MESSAGE(name)\
struct pure##name\
{\
    virtual void On##name() = 0;\
    static ICF void __fastcall OnPure(pure##name* self) { self->On##name(); }\
}

DECLARE_MESSAGE(Frame); // XXX: rename to FrameStart
DECLARE_MESSAGE(FrameEnd);
DECLARE_MESSAGE(Render);
DECLARE_MESSAGE(AppActivate);
DECLARE_MESSAGE(AppDeactivate);
DECLARE_MESSAGE(AppStart);
DECLARE_MESSAGE(AppEnd);
DECLARE_MESSAGE(DeviceReset);
DECLARE_MESSAGE(UIReset);

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
            messages[0].Object->OnPure(messages[0].Object);
        else
        {
            for (size_t i = 0; i < messages.size(); ++i)
            {
                const auto& message = messages[i];
                if (message.Prio != REG_PRIORITY_INVALID)
                    message.Object->OnPure(message.Object);
            }
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
