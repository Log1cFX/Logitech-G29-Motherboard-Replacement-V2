# After the link: checksum.py patches the ELF for the bootloader and writes
# the .bin from it, then the size and the listing of the final ELF
#   cmake -DPYTHON= -DCHECKSUM= -DELF= -DBIN= -DLIST= -DSIZE= -DOBJDUMP=
#         -P postbuild.cmake
execute_process(COMMAND "${PYTHON}" "${CHECKSUM}" "${ELF}" "${BIN}"
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  # The ELF is left unpatched: the bootloader would refuse it
  file(REMOVE "${ELF}" "${BIN}")
  message(FATAL_ERROR "checksum.py failed (${result})")
endif()

execute_process(COMMAND "${SIZE}" "${ELF}")
execute_process(COMMAND "${OBJDUMP}" -h -S "${ELF}" OUTPUT_FILE "${LIST}")
