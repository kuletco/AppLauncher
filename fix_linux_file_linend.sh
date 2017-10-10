#!/bin/sh

WORKMOD=list
WORKDIR=$(pwd)
filetypes="*.c *.cpp *.h *.ui *.pro *.pri *.rc *.xml *.conf *.ini"

if [ $# -gt 2 ]; then
	echo "Usage: $(basename $0) [OPTION] [WORKDIR]"
	echo " Options:"
	echo "   -l | l : list files"
	echo "   -u | u : update files"
	exit
fi

for x in $@
do
	case $x in
		-l|l)
			WORKMOD=list
		;;
		-u|u)
			WORKMOD=update
		;;
		*)
			if [ -d $x ]; then
				WORKDIR=$x
			fi
		;;
	esac
done

for ft in $filetypes
do
	find $WORKDIR -name $ft | while read line
	do
		if file $line | grep "CRLF" >/dev/null; then
			if [ x"$WORKMOD" = x"list" ]; then
				echo "Line end: CRLF, File: $line"
			else
				dos2unix $line
			fi
		fi
	done
done
