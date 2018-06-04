#!/usr/bin/env Codescape-Python
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
#          File:	$File: //meta/fw/meos2/DEV/LISA.PARRATT/buildsys/rebanner.py $
# Revision date:	$Date: 2015/06/02 $
#   Description:	Tool to convert binary to C.
#
##########################################################################

import sys
import re
import os.path
import datetime
try:
    import xml.etree.cElementTree as ElementTree
except ImportError:
    import xml.etree.ElementTree as ElementTree

if len(sys.argv) != 2:
	print "Usage: " + sys.argv[0] + " XML"
	sys.exit(-1)

file = sys.argv[1]
with open(file, 'r') as myfile:
    xmldata=myfile.read().replace('\n', '&#10;')
xmldata=xmldata.replace('&#10;', '\n', 1)
xmldata=xmldata[::-1].replace("&#10;"[::-1], "\n"[::-1], 1)[::-1]
tree = ElementTree.fromstring(xmldata)
for elem in tree.iter():
	if elem.tag == "testsuite":
		print("Suite %(name)s - %(tests)s tests, %(errors)s errors, %(failures)s failures, %(skipped)s skipped:" % elem.attrib)
	elif elem.tag == "testcase":
		print("Test %(name)s - %(time)ss:" % elem.attrib)
		error = elem.find("error")
		if error is None:
			print elem.find("system-out").text
		else:
			print error.get("message")
