// Portable shim for <direct.h>.
#ifndef FL_PLATFORM_DIRECT_H
#define FL_PLATFORM_DIRECT_H
#include "win_compat.h"
#include <sys/stat.h>
#include <unistd.h>
inline int _mkdir(const char* p) { return mkdir(FlPathBuf(p), 0755); }
inline int _rmdir(const char* p) { return rmdir(FlPathBuf(p)); }
inline int _chdir(const char* p) { return chdir(FlPathBuf(p)); }
inline char* _getcwd(char* b, int n) { return getcwd(b, (size_t)n); }
#endif
