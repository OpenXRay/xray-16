#pragma once

#include <array>

namespace crypto
{
// TODO: Yielder must be moved to the separate module

/// Represents the function-object type whose objects yield
/// the thread every iteration of hard computation logic.
/// Yielders could be used to write detailed logs, sleep the
/// worker thread or perform some business logic.
/// The only required parameter is a number that could mean
/// any progress for computation. For example, hash function
/// passes number of processed chunks to the yielder.
using yielder_t = fastdelegate::FastDelegate1<long>;

/// Yielder lambda that does nothing.
/// This one can be passed to computation algorith if does not
/// need yielding.
/// @param _ parameter required by yielder_t.
constexpr auto EmptyYielder = [](auto) {};

/// This class contains all necessary API for SHA1 hash computation
/// You can't create the object of this class. It is only needed to
/// group the hashing logic.
class XRCORE_API xr_sha1
{
public:
    /// Size of resulting SHA1 hash value.
    static constexpr size_t DigestSize = 20;

    /// Size of block that SHA1 hasher can process in one iteration.
    static constexpr  size_t BlockSize = 64;

    /// Represents SHA1 hash.
    using hash_t = std::array<u8, DigestSize>;

public:
    xr_sha1() = delete;

    /// Calculates the SHA1 hash of passed data.
    /// Applies yieder to each calculation iteration.
    /// @tparam T specifies the type of yielder. It should be auto-deducted.
    /// @param data the address of data to be hashed.
    /// @param data_size the size of data to be hashed.
    /// @param yielder function-object that yields calculation every processed block.
    static hash_t calculate_with_yielder(const u8* data, u32 data_size, const yielder_t& yielder);

    /// Calculates the SHA1 hash of passed data.
    /// @param data the address of data to be hashed.
    /// @param data_size the size of data to be hashed.
    static hash_t calculate(const u8* data, u32 data_size)
    {
        return calculate_with_yielder(data, data_size, EmptyYielder);
    }
};
} // namespace crypto
