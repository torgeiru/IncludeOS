#pragma once
#ifndef REMOTE_FS_HPP
#define REMOTE_FS_HPP

// #include <posix/...>
#include "fs_compatible.hpp"

class Remote_fs : public FD_compatible
{
    Remote_fs() {}
};

#endif