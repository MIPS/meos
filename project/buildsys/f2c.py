#!/usr/bin/env python
####(C)2016###############################################################
#
# Copyright (C) 2016 MIPS Tech, LLC
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
####(C)2016###############################################################
#
#   Description:	Tool to convert binary to C.
#
##########################################################################

import argparse
import sys

def f2c(outfile, varname, infile, line_lim = 80):
    with open(outfile, "ab") as fo:
        fo.write(b"unsigned char " + varname + b"[] = {\n")
        with open(infile, "rb") as fi:
            i = 0
            l = 0
            c = fi.read(1)
            if c:
                l = l + 1
                fo.write(b"0x" + format(ord(c), "02x"))
                i = i + 4
                c = fi.read(1)
                while c:
                    l = l + 1
                    if i >= line_lim - 7: # limit lines to 80 characters
                        fo.write(b",\n0x" + format(ord(c), "02x"))
                        i = 4
                    else:
                        fo.write(b", 0x" + format(ord(c), "02x"))
                        i = i + 6
                    c = fi.read(1)
        fo.write(b"\n};\n")
        fo.write(b"unsigned long " + varname + b"_size = " + str(l) + b";\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert file to c array.")
    parser.add_argument("outfile", help="output file", type=str)
    parser.add_argument("varname", help="variable name", type=str)
    parser.add_argument("infile", help="input file", type=str)
    args = parser.parse_args()
    f2c(args.outfile, args.varname, args.infile)
