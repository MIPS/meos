#!/usr/bin/env python
####(C)2014###############################################################
#
# Copyright (C) 2014 MIPS Tech, LLC
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
####(C)2014###############################################################
#
#   Description:	Generate documentation from Kconfig
#
##########################################################################

import fileinput
import re
import os

def process(file, prefix, indents):
	compound = False
	help = False
	nocat = False
	description = ''
	prompt="CAN'T HAPPEN"
	ditch = False
	menuconfig = 0
	for line in file:
		try :
			ws = len(re.match('[\s]*', line).group())
		except :
			ws = 0
			pass
		trimmed = line.strip()
		split = trimmed.split(' ', 1)
		keyword = split[0]
		# Text is help text
		if (help):
			if trimmed != '':
				description = description + " " + trimmed
			else:
				# Block processed, emit
				if compound:
					if (menuconfig != 0):
						indents.append(menuconfig)
						menuconfig = 0
					if prefix=='':
						prefix = prompt
					else:
						prefix = prefix.strip() + " | " + prompt
						nocat = True
					compound = False
				if not ditch:
					if nocat:
						print(prefix)
					else:
						print(prefix + ' | ' + prompt)
					print('\t' + description)
					print
				prompt = "CAN'T HAPPEN"
				description = ''
				help = False
				ditch = False
				nocat = False
		# endchoice or explicit endmenu
		elif (keyword == 'endmenu') or (keyword == 'endchoice'):
			rsplit = prefix.rsplit('|',1)
			if len(rsplit) == 1:
				prefix = ''
			else:
				prefix=rsplit[0].strip()
		# Indent level dropped below last menuconfig
		elif ((trimmed != '') and (ws <= indents[-1])):
			indents.pop()
			rsplit = prefix.rsplit('|',1)
			if len(rsplit) == 1:
				prefix = ''
			else:
				prefix=rsplit[0].strip()
		elif (trimmed == ''):
			ditch = False
		# help: process next paragraph as help text
		elif (keyword == 'help'):
			help = True
		# prompt: collect option name
		elif (keyword == 'bool') or (keyword == 'tristate') or (keyword == 'string') or (keyword == 'hex') or (keyword == 'int') or (keyword == 'prompt'):
			try:
				prompt = split[1].strip('"')
			except:
				ditch = True
				pass
		# menuconfig/choice: defer until details collected
		elif (keyword == 'menuconfig'):
			compound = True
			menuconfig = ws
		elif (keyword == 'choice'):
			compound = True
		# menu: Fiddle prefix
		elif (keyword == 'menu'):
			if prefix=='':
				prefix = split[1].strip('"').strip()
			else:
				prefix = prefix.strip() + " | " + split[1].strip('"')
		elif (keyword == 'source'):
			process(open(os.path.expandvars(split[1].strip('"')), "rU"), prefix, indents)

process(fileinput.input(), '', [-1])
