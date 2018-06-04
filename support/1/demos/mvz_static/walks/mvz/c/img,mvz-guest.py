next_decl = "/* Last in chain */"
next_call = "/* Last in chain */"
next_s_decl = "/* Last in chain */"
next_s_call = "/* Last in chain */"

if next is not None:
	next_decl = "inline static void MVZ_guest" + next["__address"] + "Config(void);\t/* Next link in chain */"
	next_call = "MVZ_guest" + next["__address"] + "Config(); /* Chain */"
	next_s_decl = "inline static void MVZ_guest" + next["__address"] + "Start(void);\t/* Next link in chain */"
	next_s_call = "MVZ_guest" + next["__address"] + "Start(); /* Chain */"

index = node["__address"]
try:
	memGrouped = zip(*[iter(node["ranges"])]*3)
except KeyError:
	memGrouped = []
try:
	intsGrouped = zip(*[iter(node["mti,passthru-ints"])]*2)
except KeyError:
	intsGrouped = []

intindex = 0
for ints in intsGrouped:
	print("	IRQ_DESC_T MVZ_guest%(index)sInt%(intindex)s;" % { "index" : index, "intindex" : intindex})
	intindex = intindex + 1

print("""
MVZ_GUEST_T MVZ_guest%(index)s;
%(next_decl)s
%(next_s_decl)s
extern reg_t _binary_guest%(index)s_elf[];

void MVZ_guest%(index)sLoad(MVZ_GUEST_T *guest)
{""" % {"index" : index, "next_decl" : next_decl, "next_s_decl" : next_s_decl})

for range in memGrouped:
	print("	MVZ_zeroGP(&MVZ_guest%(index)s, %(child)s, %(size)s);" % { "index" : index, "child" : str(range[0]), "size" : str(range[2])})

print("""	MVZ_loadELF(&MVZ_guest%(index)s, _loadELF, _binary_guest%(index)s_elf);
}

void MVZ_guest%(index)sConfig(void)
{
	MVZ_initGuest(&MVZ_guest%(index)s, %(indexp1)s, &MVZ_guest%(index)sLoad);""" % {"index" : index, "indexp1" : str(int(index) + 1)})

for range in memGrouped:
	print("	MVZ_fixMapping(&tlbIndex, %(parent)s, %(size)s, &MVZ_guest%(index)s, %(child)s, MVZ_MEM_FLAG_ALLOWALL);" % {"index" : index, "child" : str(range[0]), "parent" : str(range[1]), "size" : str(range[2])})

if intindex:
	ph = findUp(node, "interrupt-parent")
	ip = findPH(root, findUp(node, "interrupt-parent"))
	intindex = 0
	for ints in intsGrouped:
		if "mti,gic" in ip["compatible"]:
			print("""	MVZ_guest%(index)sInt%(intindex)s.intNum = 3;
	MVZ_guest%(index)sInt%(intindex)s.impSpec.extNum = %(root)s;""" % { "index" : index, "intindex" : intindex, "root" : str(ints[0])})
		else:
			print("	MVZ_guest%(index)sInt%(intindex)s.intNum = %(root)s;""" % { "index" : index, "intindex" : intindex, "root" : str(ints[0])})
		print("	MVZ_intMap(&MVZ_guest%(index)sInt%(intindex)s,&MVZ_guest%(index)s);" % { "index" : index, "intindex" : intindex})
		intindex = intindex + 1

print("""	%(next_call)s
}

void MVZ_guest%(index)sStart(void)
{
	MVZ_startGuest(&MVZ_guest%(index)s, KRN_LOWEST_PRIORITY, "%(name)s %(index)s");
	%(next_s_call)s
}""" % {"index" : index, "name" : node["__name"], "next_decl" : next_decl, "next_call" : next_call, "next_s_decl" : next_s_decl, "next_s_call" : next_s_call})
