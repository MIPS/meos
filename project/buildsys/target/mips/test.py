#!/usr/bin/env Codescape-Python

from imgtec.console import *
import imgtec.codescape
from imgtec.test import *
import project

@test.suiteSetUp
def suiteSetUp():
    project.start()

@test.suiteTearDown
def suiteTearDown():
    project.stop()

@test
def test0():
    res = project.load()
    devs = res["targets"]
    go(quiet, devices=devs)
    project.waitforhalt(1800, all, probe().socs[0])
    if regs("a0") != 0:
      fail()

if  imgtec.codescape.environment == "standalone":
    sys.exit(project.main())
else:
    project.load(rsts, ldrs)
