#pragma once

#ifdef WIN32
#include "systemwindows.h"
#else
#include "systemlinux.h"
#define SYS Utils::System
#endif
