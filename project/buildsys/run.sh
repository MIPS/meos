#!/bin/sh
printf "[32m[[39mRUN[32m][39m\texec=$1 time=`date +%Y/%m/%d-%H:%M:%S` result="
cd `dirname $1`
DIR=`dirname $2`
mkdir -p $DIR/xml/
BASE=`basename -s .log $2`
TXML=$DIR/$BASE.xml
if /usr/bin/time -f%E -o $2.time $3 > $2 2>&1; then
{
	printf "pass log=$2 runtime="
	cat $2.time
	XML=$DIR/xml/*.xml
	if [ -e $XML ]; then
		sed -i -e "s/regression.__main__/regression.$BASE/g" $XML
		mv $XML $TXML
	else
		# Synthesize test pass XML
		printf "<?xml version=\"1.0\" ?><testsuites disabled=\"0\" errors=\"0\" failures=\"0\" tests=\"1\" time=\"0\"><testsuite disabled=\"0\" errors=\"0\" failures=\"0\" name=\"regression\" skipped=\"0\" tests=\"1\" time=\"0\"><testcase classname=\"regression.$BASE\" name=\"test0\" time=\"0\"><system-out>" > $TXML
		sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' $2 >> $TXML
		printf "</system-out></testcase></testsuite></testsuites>" >> $TXML
	fi
	if [ ! -z $5  ] ; then
		echo ---
		$4/buildsys/report.py $TXML
		echo ---
	fi
}
else
{
	printf "fail log=$2 runtime="
	cat $2.time
	XML=$DIR/xml/*.xml
	if [ -e $XML ]; then
		sed -i -e "s/regression.__main__/regression.$BASE/g" $XML
		mv $XML $TXML
	else
		# Synthesize test pass XML
		printf "<?xml version=\"1.0\" ?><testsuites disabled=\"0\" errors=\"0\" failures=\"1\" tests=\"1\" time=\"0\"><testsuite disabled=\"0\" errors=\"0\" failures=\"1\" name=\"regression\" skipped=\"0\" tests=\"1\" time=\"0\"><testcase classname=\"regression.$BASE\" name=\"test0\" time=\"0\"><failure type=\"synthetic\">fail</failure><system-out>" > $TXML
		sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' $2 >> $TXML
		printf "</system-out></testcase></testsuite></testsuites>" >> $TXML
	fi
	echo ---
	$4/buildsys/report.py $TXML
	echo ---
}
fi
rm -fr $DIR/xml
rm $2.time
