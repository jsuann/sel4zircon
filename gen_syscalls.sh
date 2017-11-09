#!/bin/sh

DEFS_FILE="apps/zircon-server/include/syscall_defs.h"
TABLE_FILE="apps/zircon-server/src/syscalls/sys_table.c"
SYSNO_FILE="libzircon/src/sys_def.h"

[ ! -f $DEFS_FILE ] && exit 1
[ ! -f $TABLE_FILE ] && exit 1
[ ! -f $SYSNO_FILE ] && exit 1

echo "Generating syscalls from syscalls.list"

NUM_SYSCALLS=`wc -l syscalls.list | cut -f 1 -d " "`

echo "#include \"syscalls.h\"" > $TABLE_FILE
echo "" >> $TABLE_FILE
echo "zx_syscall_func sys_table[NUM_SYSCALLS] = {" >> $TABLE_FILE

echo "#define NUM_SYSCALLS $NUM_SYSCALLS" > $DEFS_FILE
echo "" >> $DEFS_FILE

echo -n > $SYSNO_FILE
SYS_NO=0

cat syscalls.list | while read line
do
    echo "void $line(seL4_MessageInfo_t tag, uint32_t handle);" >> $DEFS_FILE
    echo "    ${line}," >> $TABLE_FILE
    UPPER_LINE=`echo ${line} | tr [a-z] [A-Z]`
    echo "#define ZX_${UPPER_LINE} $SYS_NO" >> $SYSNO_FILE
    SYS_NO=`expr $SYS_NO + 1`
done

echo "};" >> $TABLE_FILE
