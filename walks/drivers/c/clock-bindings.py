_getClock = None

def getClock(node, index):
	global _getClock
	if "clocks" in node:
		p = node["clocks"][index]
		if type(p) is str:
			c = findPH(root, p)
		else:
			c = findPH(root,p[0])
		if c:
			if "fixed-factor-clock" in c["compatible"]:
				mult = int(c["clock-mult"])
				div = int(c["clock-div"])
				return (_getClock(c, 0) * mult) / div
			else:
				if "clock-frequency" in c:
					if type(c["clock-frequency"]) is str:
						return int(c["clock-frequency"])
					else:
						return int(c["clock-frequency"][p[1]])
				else:
						return _getClock(c, 0)

_getClock = getClock

def getClockByName(node, name):
	for k in node["clock-names"]:
		if node["clock-names"][k] == name:
			i = k
	return getClock(node, i)
