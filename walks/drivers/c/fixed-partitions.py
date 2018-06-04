filtered = collections.OrderedDict((k, v) for k, v in node.items() if (type(v) is collections.OrderedDict) and (k != "__parent"))

for i in filtered:
	node[i]["compatible"] = "mti,fixed-partition"
