#!/bin/sh

DEFS_FILE="include/syscall_defs.h"
TABLE_FILE="src/syscalls/sys_table.h"

NUM_SYSCALLS=`wc -l syscalls.list | cut -f 1 -d " "`

echo -n > $TABLE_FILE

echo "#define NUM_SYSCALLS $NUM_SYSCALLS" > $DEFS_FILE
echo "" >> $DEFS_FILE

cat syscalls.list | while read line
do
    echo "void $line(uint32_t handle);" >> $DEFS_FILE
    echo "${line}," >> $TABLE_FILE
done
