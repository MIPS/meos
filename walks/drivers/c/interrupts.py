if not "interruptParent" in variables:
	variables["interruptParent"] = dict()

def decodeInterrupt(node, interrupt, descName):
	ip = findPH(root, findUp(node, "interrupt-parent"))
	variables["interruptParent"][ip["compatible"]](interrupt, descName)
