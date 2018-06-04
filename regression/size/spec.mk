#INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := tests/size/size.py

LOG := logs/size.log

#PRINT_OUT=YES

define PLOT_FILE_1
set terminal svg size 1024,768
set output 'size.svg'

set key top left

set xtics 1
endef
export PLOT_FILE_1

define PLOT_FILE_2
set xlabel 'Component'

set ylabel 'Bytes'

set title 'MEOS $(VERSION) $(shell date) Maximal code/data sizes'

unset autoscale y
set autoscale ymax

set style data histogram
set style histogram rowstacked
set style fill pattern border
set boxwidth 0.75

plot 'size.data' using 1 title "Code" lt -1, '' using 2 title "Data" lt -1, '' using 3 title "BSS" lt -1
endef
export PLOT_FILE_2

include $(ABUILD_DIR)/../../modules.sh

GNUPLOT_EXECFILE := $(MEOS_DIR)/bin/gnuplot
GNUPLOT_EXEC ?= $(GNUPLOT_EXECFILE)

.PHONY: $(LOG)
$(LOG): sizecalc
	$(Q)echo -n $(call BANNER,RUN,$(@))$(SPACE)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo "exec=$(ABUILD_DIR)/regression/size result=pass log=$@ runtime=00:00.00"| tee -a $(UNILOG)
	$(Q)size -B -d -t $(MEOS_DIR)/lib/libMEOS.a > $(ABUILD_DIR)/raw
	$(Q)rm -f $(ABUILD_DIR)/size.data
	$(Q)\
	TEXT_NB=0;\
	DATA_NB=0;\
	BSS_NB=0;\
	for MODULE in `echo $(filter-out support,$(MODULES)) | tr '[:lower:]' '[:upper:]'`; do\
		KEY=$${MODULE}_SRC;\
		eval SRC='$$'$${KEY};\
		TEXT_ACC=0;\
		DATA_ACC=0;\
		BSS_ACC=0;\
		for FILE in $${SRC}; do \
			FN=`basename $${FILE}|sed 's/\.[^\.]*$$//'`.o;\
			LN=`grep $${FN} $(ABUILD_DIR)/raw`;\
			TEXT=`echo $${LN}|cut -d\  -f1`;\
			DATA=`echo $${LN}|cut -d\  -f2`;\
			BSS=`echo $${LN}|cut -d\  -f3`;\
			TEXT_ACC=`expr $${TEXT_ACC} + $${TEXT}`;\
			DATA_ACC=`expr $${DATA_ACC} + $${DATA}`;\
			BSS_ACC=`expr $${BSS_ACC} + $${BSS}`;\
		done;\
		echo "$${TEXT_ACC}	$${DATA_ACC}	$${BSS_ACC}	#$${MODULE}" >> $(ABUILD_DIR)/size.data;\
		TEXT_NB=`expr $${TEXT_NB} + $${TEXT_ACC}`;\
		DATA_NB=`expr $${DATA_NB} + $${DATA_ACC}`;\
		BSS_NB=`expr $${BSS_NB} + $${BSS_ACC}`;\
	done;\
	LN=`grep \(TOTALS\) $(ABUILD_DIR)/raw`;\
	TEXT=`echo $${LN}|cut -d\  -f1`;\
	DATA=`echo $${LN}|cut -d\  -f2`;\
	BSS=`echo $${LN}|cut -d\  -f3`;\
	TEXT_B=`expr $${TEXT} - $${TEXT_NB}`;\
	DATA_B=`expr $${DATA} - $${DATA_NB}`;\
	BSS_B=`expr $${BSS} - $${BSS_NB}`;\
	echo "$${TEXT_B}	$${DATA_B}	$${BSS_B}	#BSP" >> $(ABUILD_DIR)/size.data;\
	echo "$${TEXT}	$${DATA}	$${BSS}	#TOTAL" >> $(ABUILD_DIR)/size.data
	$(Q)rm -f $(ABUILD_DIR)/raw
	$(Q)echo "$${PLOT_FILE_1}" > $(ABUILD_DIR)/size.plot
	$(Q)echo -n "set xtics (" >> $(ABUILD_DIR)/size.plot
	$(Q)LST="$(call UPPER,$(filter-out support,$(MODULES)))";\
	X=0;\
	for MODULE in $${LST}; do\
		echo -n \'$${MODULE}\' $${X}, >> $(ABUILD_DIR)/size.plot;\
		X=`expr $${X} + 1`;\
	done;\
	echo -n \'BSP\' $${X}, >> $(ABUILD_DIR)/size.plot;\
	X=`expr $${X} + 1`;\
	echo "'TOTAL' $${X})" >> $(ABUILD_DIR)/size.plot;
	$(Q)echo "$${PLOT_FILE_2}" >> $(ABUILD_DIR)/size.plot
	$(Q)cd $(ABUILD_DIR)/; $(GNUPLOT_EXEC) $(ABUILD_DIR)/size.plot
	$(Q)echo "Code	Data	BSS	Module" > $@
	$(Q)cat $(ABUILD_DIR)/size.data >> $@
	$(Q)printf "<?xml version=\"1.0\" ?><testsuites disabled=\"0\" errors=\"0\" failures=\"0\" tests=\"1\" time=\"0\"><testsuite disabled=\"0\" errors=\"0\" failures=\"0\" name=\"regression\" skipped=\"0\" tests=\"1\" time=\"0\"><testcase classname=\"regression.size\" name=\"test0\" time=\"0\"><system-out>" > $(basename $@).xml
	$(Q)cat $@ >> $(basename $@).xml
	$(Q)printf "</system-out></testcase></testsuite></testsuites>" >> $(basename $@).xml
	$(Q)rm -f $(ABUILD_DIR)/size.data $(ABUILD_DIR)/size.plot
	$(Q)echo ---|tee -a $(UNILOG)
	$(Q)cat $@|tee -a $(UNILOG)
	$(Q)echo ---|tee -a $(UNILOG)
