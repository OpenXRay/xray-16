#pragma once

class ENGINE_API DeveloperMode
{
public:
    static bool Requested();
    static bool HasOption(pcstr option);
};
