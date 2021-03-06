#!/bin/bash

GENSYS_PATH="$(dirname "$(realpath "$0")")"
SZX_PATH="$(realpath "${GENSYS_PATH}"/..)"

ZIRCON_PATH="${SZX_PATH}"/../zircon-src
SYSCALL_ABIGEN=system/public/zircon/syscalls.abigen

ABIGEN_FLAT="${GENSYS_PATH}"/abigen-flat

DEFS_FILE="${SZX_PATH}"/apps/zircon-server/include/syscall_defs.h
TABLE_FILE="${SZX_PATH}"/apps/zircon-server/src/syscalls/sys_table.cxx
SYSNO_FILE="${SZX_PATH}"/libzircon/src/sys_def.h

DEFINED_LIST="${GENSYS_PATH}"/syscalls.list
SKIP_LIST="${GENSYS_PATH}"/skip-sys.list
EXTRA_SYSCALLS="${GENSYS_PATH}"/extra-sys.list

LIBZIRCON_HEADER="${SZX_PATH}"/libzircon/include/zircon/zx_calls.h
LIBZIRCON_DEFS="${SZX_PATH}"/libzircon/src/zx_calls.def

AUTOGEN_TEXT="/* This is an autogenerated file, do not edit. */"

if [ ! -f "${ZIRCON_PATH}/${SYSCALL_ABIGEN}" ]; then
    echo "Could not find syscall.abigen at ${ZIRCON_PATH}/${SYSCALL_ABIGEN}!"
    exit 1
fi

echo "Generating syscall files from syscalls.abigen..."

# initialise files
echo "${AUTOGEN_TEXT}
#include \"syscalls.h\"

zx_syscall_func sys_table[NUM_SYSCALLS] = {" > "${TABLE_FILE}"

echo "${AUTOGEN_TEXT}" > "${SYSNO_FILE}"
echo "${AUTOGEN_TEXT}" > "${DEFS_FILE}"
echo "${AUTOGEN_TEXT}" > "${ABIGEN_FLAT}"
echo "${AUTOGEN_TEXT}" > "${LIBZIRCON_HEADER}"
echo "${AUTOGEN_TEXT}" > "${LIBZIRCON_DEFS}"

syscall=
syscall_num=0

# add regular zx calls from abigen
while read -r line || [ -n "${line}" ]
do
    # skip empty lines, comments
    [ -z "${line}" ] && continue
    [[ "${line}" =~ ^# ]] && continue

    # ensure entire syscall def is on one line
    syscall="${syscall}${line}"
    [[ ! "${syscall}" =~ ^syscall.*\;$ ]] && continue

    echo "${syscall}" >> "${ABIGEN_FLAT}"

    # syscalls marked "internal" are skipped
    if grep -q 'internal' <<< "${syscall}"; then
        syscall=
        continue
    fi

    syscall_name=$(sed -e 's/^syscall \([a-z0-9_]*\).*$/\1/' <<< "${syscall}")

    # some syscalls are not handled server-side, we only want user-side declaration
    skipflag=0
    if grep -q "^zx_${syscall_name}$" "${SKIP_LIST}"; then
        skipflag=1
    fi

    # get name for syscall number definition
    syscall_def="ZX_SYS_$(tr '[:lower:]' '[:upper:]' <<< "${syscall_name}")"

    if [ ${skipflag} -eq 0 ]; then
        # define syscall number
        echo "#define ${syscall_def} ${syscall_num}" >> "${SYSNO_FILE}"

        # check if syscall has server side implementation
        if grep -q "^sys_${syscall_name}$" "${DEFINED_LIST}"; then
            # declare kernel-side handler
            echo "uint64_t sys_${syscall_name}(seL4_MessageInfo_t tag, uint64_t badge);" >> "${DEFS_FILE}"
            # add to syscall table
            echo "    sys_${syscall_name}," >> "${TABLE_FILE}"
        else
            # add as undefined to syscall table
            echo "    sys_undefined," >> "${TABLE_FILE}"
        fi
    fi

    # --- generate user side lib calls ---

    # parse for syscall args & retval
    if [[ "${syscall}" =~ noreturn ]]; then
        syscall_args=$(sed -e 's/^.*noreturn.*(\(.*\)).*$/\1/' <<< "${syscall}")
        syscall_ret=void
        syscall_ret_args=
    else
        syscall_args=$(sed -e 's/^.*(\(.*\)).*returns.*$/\1/' <<< "${syscall}")
        syscall_ret=$(sed -e 's/^.*(\(.*\));$/\1/' <<< "${syscall}")
        syscall_ret_args=$(cut -s -f 2- -d ',' <<< "${syscall_ret}")
        syscall_ret=$(cut -f 1 -d ',' <<< "${syscall_ret}")
        #echo "${syscall_ret} --- ${syscall_ret_args}"
    fi

    # generate the syscall declaration
    arg_count=0
    arg_name_list=
    is_return_arg=0
    syscall_decl="${syscall_ret} zx_${syscall_name}("
    if [ -z "${syscall_args}" ] && [ -z "${syscall_ret_args}" ]; then
        syscall_decl="${syscall_decl}void)"
    else
        arg_list="${syscall_args},return_args,${syscall_ret_args},"
        while [ -n "${arg_list}" ]
        do
            curr_arg=$(cut -f 1 -d ',' <<< "${arg_list}")
            arg_list=$(cut -f 2- -d ',' <<< "${arg_list}")
            [ -z "${curr_arg}" ] && continue

            if [ "${curr_arg}" = "return_args" ]; then
                is_return_arg=1
                continue
            fi

            arg_name=$(cut -f 1 -d ':' <<< "${curr_arg}")
            arg_type=$(cut -f 2 -d ':' <<< "${curr_arg}")
            # remove spaces at start & end
            arg_name=${arg_name# }
            arg_name=${arg_name% }
            arg_type=${arg_type# }
            arg_type=${arg_type% }

            # special case: if arg name is "name", we need to make const
            # or if syscall is a "set" or "write", and "IN" buffer
            if [ "${arg_name}" = "name" ]; then
                arg_type="const ${arg_type}"
            fi

            # check if arg type is a pointer, or has leftover abigen info
            if [[ "${arg_type}" =~ \[ ]]; then
                arg_type="$(cut -f 1 -d '[' <<< "${arg_type}")*"
                arg_type=${arg_type/any/void}
                if [[ "${syscall_name}" =~ (write|set) ]]; then
                    arg_type="const ${arg_type}"
                fi
            elif [[ "${arg_type}" =~ optional|features|handle_acquire ]]; then
                arg_type="${arg_type% *}*"
            elif [ "${syscall_name}" = "pci_get_nth_device" ] && \
                    [ "${arg_name}" = "out_info" ]; then
                arg_type="${arg_type% *}*"
            elif [ "${syscall_name}" = "vcpu_resume" ] && \
                    [ "${arg_name}" = "packet" ]; then
                arg_type="${arg_type% *}*"
            elif [ ${is_return_arg} -ne 0 ]; then
                arg_type="${arg_type% *}*"
            else
                arg_type="${arg_type% *}"
            fi


            syscall_decl="${syscall_decl}${arg_type} ${arg_name}, "
            arg_name_list="${arg_name_list}, ${arg_name}"
            arg_count=$((arg_count + 1))
        done
        # remove trailing comma, add closing bracket
        syscall_decl="${syscall_decl%, })"
    fi

    # gen lib header file
    echo >> "${LIBZIRCON_HEADER}"
    echo "extern ${syscall_decl};" >> "${LIBZIRCON_HEADER}"

    if [ ${skipflag} -eq 0 ]; then
        # gen lib def file
        echo "
${syscall_decl}
{
    ZX_SYSCALL_SEND(${syscall_def}, ${arg_count}${arg_name_list});" >> "${LIBZIRCON_DEFS}"

        if [ "${syscall_ret}" != "void" ]; then
            echo "    return seL4_GetMR(0);" >> "${LIBZIRCON_DEFS}"
        fi
        echo "}" >> "${LIBZIRCON_DEFS}"
    fi

    syscall=
    if [ ${skipflag} -eq 0 ]; then
        syscall_num=$((syscall_num + 1))
    fi
done < "${ZIRCON_PATH}/${SYSCALL_ABIGEN}"

# add extra sel4zircon syscalls
# first note this in relevant files
EXTRA_SYS_TEXT="/* sel4zircon syscalls defined below */"
echo "${EXTRA_SYS_TEXT}" >> "${SYSNO_FILE}"
echo "${EXTRA_SYS_TEXT}" >> "${DEFS_FILE}"
echo "    ${EXTRA_SYS_TEXT}" >> "${TABLE_FILE}"

# add extra call defs
while read -r line || [ -n "${line}" ]
do
    # skip empty lines, comments
    [ -z "${line}" ] && continue
    [[ "${line}" =~ ^# ]] && continue

    # get name
    syscall_name=$(sed -e 's/^zx_\([a-z0-9_]*\).*$/\1/' <<< "${line}")

    # get name for syscall number definition
    syscall_def="ZX_SYS_$(tr '[:lower:]' '[:upper:]' <<< "${syscall_name}")"

    # define syscall number
    echo "#define ${syscall_def} ${syscall_num}" >> "${SYSNO_FILE}"

    # declare kernel-side handler and add to syscall table
    echo "uint64_t sys_${syscall_name}(seL4_MessageInfo_t tag, uint64_t badge);" >> "${DEFS_FILE}"
    echo "    sys_${syscall_name}," >> "${TABLE_FILE}"

    # user side call definitions are done manually

    syscall_num=$((syscall_num + 1))
done < "${EXTRA_SYSCALLS}"

# define total number of syscalls
echo "
#define NUM_SYSCALLS ${syscall_num}" >> "${DEFS_FILE}"

echo "};" >> "${TABLE_FILE}"

echo "Done."
