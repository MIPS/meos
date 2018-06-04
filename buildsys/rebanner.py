#!/usr/bin/env python
####(C)2015###############################################################
#
# Copyright (C) 2015 MIPS Tech, LLC
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
####(C)2015###############################################################
#
#   Description:	Tool to automatically reapply copyright banners.
#
##########################################################################

import sys
import re
import os.path
import datetime

banner = """Copyright (C) $Y MIPS Tech, LLC

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.""".splitlines(True)

if len(sys.argv) < 2 or len(sys.argv) > 3:
	print "Usage: " + sys.argv[0] + " SRC [BANNER]"
	sys.exit(-1)

file = sys.argv[1]
if len(sys.argv) == 3:
	with open(sys.argv[2], 'r') as content_file:
		banner = content_file.read().splitlines(True)

def newbanner(a, b, z, y):
	l = 67 - len(y)
	n = str(datetime.datetime.now().year)

	s = a + b * 3 + "(C)" + y + b * l + "\n"
	m = b + "\n" + "".join([b + " " + i.replace("$Y", y).replace("$N", n) for i in z]) + "\n" + b + "\n"
	e = b * 4 + "(C)" + y + b * (l - 1) + a + "\n"

	return s + m + e

def verifybanner(lines, b, bx, i, y):
	for j in range(i + 1, len(lines)):
		l = lines[j]
		if len(l) == 0 or l[0] != b:
			return None
		r = re.match(bx * 4 + "\(C\)([0-9][0-9][0-9][0-9])" + bx * 2, l)
		if r is not None:
			if r.group(1) == y:
				return j
			else:
				return None
	return None

with open(file, 'r') as content_file:
    lines = content_file.read().splitlines(True)

i = 0
while i < len(lines):
	l = lines[i]
	if len(l) > 2:
		a = l[0]
		b = l[1]
		ax = "\\x" + "%02x" % ord(a)
		bx = "\\x" + "%02x" % ord(b)
		r = re.match(ax + bx * 3 + "\(C\)([0-9][0-9][0-9][0-9])" + bx * 2, l)
		if r is not None:
			y = r.group(1)
			print "Found potential banner at line %d, year %s" % (i + 1, y)
			j = verifybanner(lines, b, bx, i, y)
			if j is not None:
				print "Verified banner"
				nl = "".join(lines[:i]) + newbanner(a, b, banner, y)
				i = len(nl.splitlines(True))
				print "Resuming at %d" % (i + 1)
				lines = (nl + "".join(lines[j + 1:])).splitlines(True)
				print "New length %d " % len(lines)
	i = i + 1

with open(file, "w") as t:
    t.write("".join(lines))
