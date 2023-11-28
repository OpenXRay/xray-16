#pragma once

#include <atomic>


struct XR_GL_RESOURCE
{
    void AddRef()
    {
        ++ref_counter;
    }

    virtual uint32_t Release()
    {
        VERIFY(ref_counter > 0);
        return --ref_counter;
    }

    std::atomic<uint32_t> ref_counter{ 0 };
};

struct XR_GL_TEXTURE_BASE : public XR_GL_RESOURCE
{
    GLuint handle{ GL_INVALID_VALUE };
    GLenum type{ GL_INVALID_ENUM };

    static XR_GL_TEXTURE_BASE* create(GLenum type_)
    {
        auto* obj = xr_new<XR_GL_TEXTURE_BASE>();
        obj->type = type_;
        glGenTextures(1, &obj->handle);
        obj->AddRef();
        return obj;
    }

    uint32_t Release() override
    {
        XR_GL_RESOURCE::Release();
        if (ref_counter == 0)
        {
            glDeleteTextures(1, &handle);
            xr_delete(this);
        }
        return ref_counter;
    }

    bool operator==(const XR_GL_TEXTURE_BASE& other) const
    {
        return (other.handle == handle) && (other.type == type);
    }

    bool is_valid() const
    {
        return (handle != GL_INVALID_VALUE) && (type != GL_INVALID_ENUM);
    }

//private:
    XR_GL_TEXTURE_BASE() = default;
};
