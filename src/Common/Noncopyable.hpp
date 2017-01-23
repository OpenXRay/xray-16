#pragma once

class Noncopyable
{
public:
    Noncopyable() = default;
    Noncopyable(Noncopyable&) = delete;
    Noncopyable& operator=(Noncopyable&) = delete;
};
