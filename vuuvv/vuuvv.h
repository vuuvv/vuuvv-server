#ifndef PY__VUUVV_H
# define PY__VUUVV_H

#include "defines.h"
#include "os/win32/defines.h"
#include "os/win32/errno.h"

#include <Python.h>
#include "structmember.h"

#include <wchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>

#include "event_iocp.h"
#include "eventloop.h"

extern void v_log(int level, const char *fmt, ...);
#endif /* PY__VUUVV_H */

