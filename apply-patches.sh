#!/bin/sh

SZX_PROJ="$(dirname "$(realpath "$0")")"
BASE_DIR="$(realpath "${SZX_PROJ}"/../..)"

(cd "${BASE_DIR}"/kernel && \
    git am "${SZX_PROJ}"/patches/0001-Patch-make-fault-addr-word_t.patch)

(cd "${BASE_DIR}"/projects/seL4_libs && \
    git am "${SZX_PROJ}"/patches/0001-libsel4simple-default-Fixes-to-cap-info-funcs.patch)
