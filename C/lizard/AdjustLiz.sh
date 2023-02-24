#!/bin/bash
# /TR 2023-02-24

function repl() {
  #sed -e "s|LIZ_||g" -i *.c *.h
  sed -e "s|$1|LIZ_${1}|g" -i *.c *.h
}

repl HUF_readStats
repl HUF_getErrorName
repl HUF_isError

repl FSE_readNCount
repl FSE_getErrorName
repl FSE_isError
repl FSE_versionNumber

repl FSE_compressBound
repl FSE_compress_usingCTable
repl FSE_buildCTable_rle
repl FSE_normalizeCount
repl FSE_optimalTableLog
repl FSE_writeNCount
repl FSE_NCountWriteBound
repl FSE_buildCTable_wksp

repl HUF_optimalTableLog
repl HUF_compress4X_usingCTable
repl HUF_compress1X_usingCTable
repl HUF_compressBound already
repl HUF_buildCTable_wksp
repl HUF_readCTable

repl HUF_decompress4X_usingDTable
repl HUF_decompress1X_usingDTable
repl HUF_selectDecoder
