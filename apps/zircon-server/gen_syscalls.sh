#!/bin/sh

DEFS_FILE="include/syscall_defs.h"
TABLE_FILE="src/syscalls/sys_table.h"
SYSNO_FILE="../zircon-test/src/sys_def.h"

NUM_SYSCALLS=`wc -l syscalls.list | cut -f 1 -d " "`

echo -n > $TABLE_FILE
echo -n > $SYSNO_FILE

echo "#define NUM_SYSCALLS $NUM_SYSCALLS" > $DEFS_FILE
echo "" >> $DEFS_FILE

SYS_NO=0

cat syscalls.list | while read line
do
    echo "void $line(uint32_t handle);" >> $DEFS_FILE
    echo "${line}," >> $TABLE_FILE
    UPPER_LINE=`echo ${line} | tr [a-z] [A-Z]`
    echo "#define ZX_${UPPER_LINE} $SYS_NO" >> $SYSNO_FILE
    SYS_NO=`expr $SYS_NO + 1`
done
