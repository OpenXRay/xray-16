#pragma once

// Android's NDK exposes the POSIX/Bionic surface used by the Linux platform
// shim.  Keep this as a separate platform include so Android-specific policy
// (paths, logging and input) can evolve without making desktop Linux depend on
// it.  The common ABI glue is intentionally shared for now.
#include "Common/PlatformLinux.inl"
