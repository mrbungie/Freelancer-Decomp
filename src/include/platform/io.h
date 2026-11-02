// io.h shim for non-Windows platforms
#pragma once

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif
