#pragma once

struct MotionID
{
public:
    union
    {
        struct
        {
            u16 idx : 16; // 14
            u16 slot : 16; // 2
        };
        //.		u16			val;
        u32 val{ u32(-1) };
    };

public:
    ICF MotionID() = default;

    ICF MotionID(MotionID&& other) noexcept : val(other.val)
    {
        other.invalidate();
    }

    ICF MotionID(const MotionID& other) noexcept : val(other.val) {}

    ICF MotionID(u16 motion_slot, u16 motion_idx) noexcept
        : idx(motion_idx), slot(motion_slot) {}

    ICF MotionID& operator=(MotionID&& other) noexcept
    {
        val = other.val;
        other.invalidate();
        return *this;
    }

    ICF MotionID& operator=(const MotionID& other) noexcept
    {
        val = other.val;
        return *this;
    }

    [[nodiscard]]
    ICF bool operator==(const MotionID& tgt) const noexcept { return tgt.val == val; }

    [[nodiscard]]
    ICF bool operator!=(const MotionID& tgt) const noexcept { return tgt.val != val; }

    [[nodiscard]]
    ICF bool operator<(const MotionID& tgt) const noexcept { return val < tgt.val; }

    [[nodiscard]]
    ICF bool operator!() const noexcept { return !valid(); }

    ICF void set(u16 motion_slot, u16 motion_idx)
    {
        slot = motion_slot;
        idx = motion_idx;
    }

    ICF void invalidate() noexcept { val = u32(-1); }

    [[nodiscard]]
    ICF bool valid() const noexcept { return val != u32(-1); }

    [[nodiscard]]
    ICF const MotionID* get() const noexcept { return this; };

    [[nodiscard]]
    ICF explicit operator bool() const noexcept { return valid(); }
};
