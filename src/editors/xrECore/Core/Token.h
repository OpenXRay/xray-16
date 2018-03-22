#pragma once

namespace XRay
{
public ref class Token sealed
{
    System::String^ name;
    int id;

public:
    Token() : id(-1), name(nullptr) {}
    Token(int _id, pcstr _name) : id(_id), name(gcnew System::String(_name)) {}
    Token(int _id, System::String^ _name) : id(_id), name(_name) {}

    System::String^ ToString() override { return name; }
    System::Int32 ToInt32() { return (System::Int32)id; }
    System::Int64 ToInt64() { return (System::Int64)id; }
};
} // namespace XRay
