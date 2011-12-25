import os
from pake import cc, task

src = ['hello.c']

hello_obj = cfile('hello.c', ['hello.h'], default=True)

