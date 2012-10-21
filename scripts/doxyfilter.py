#!/usr/bin/python
#
# Doxygen 1.6.3 gets it wrong for files with the same name in different
# directories, unless we fix it as follows (by always including with
# libfoo/name even from files also in libfoo).

import os
import sys
import posix
import string
import os.path

f = open(sys.argv[1], "r")

lines = f.read().split('\n')

for line in lines:
    inc = line[0:10]
    if inc == "#include \"":
        if line.count("/") == 0:
            name = line[10:-1]
            f2 = os.path.basename(os.path.dirname(sys.argv[1])) + "/" + name
            if os.path.exists(f2):
                print "#include \""+f2+"\""
            else:
                print line
        else:
            print line
    else:
        print line
        
