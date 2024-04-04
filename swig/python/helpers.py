"""
	Define helpers that have no direct dependencies on QEMU.
	So these are quirks that are needed in the context of libpython inside QEMU.
"""

import os
import io
import sys
import builtins


def _stdout_write(s):
    written = 0
    while written < len(s):
        try:
            written = written + os.write(sys.stdout.fileno(), s[written:])
        except:
            pass

# Work around for builtin print(), so it will not trow exception when writing to non-blocking stdout
def print(*args, **kwargs):
	buf = io.StringIO()
	builtins.print(*args, file=buf, **kwargs)
	_stdout_write(buf.getvalue().encode())