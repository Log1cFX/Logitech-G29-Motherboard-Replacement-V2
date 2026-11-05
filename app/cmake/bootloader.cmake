# Builds the bootloader (../bootloader, its own Makefile) with CONFIG, before
# the firmware. STAMP keeps the CONFIG of the last build: another one cleans
# the bootloader first, its objects were built with other defines
#   cmake -DMAKE=<make> -DCONFIG=<defines> -DDIR=<bootloader> -DSTAMP=<file>
#         -P bootloader.cmake
set(previous "")
if(EXISTS "${STAMP}")
  file(READ "${STAMP}" previous)
  string(STRIP "${previous}" previous)
endif()

if(NOT previous STREQUAL CONFIG)
  file(REMOVE "${STAMP}")
  # Nothing to clean is not an error
  execute_process(COMMAND "${MAKE}" -C "${DIR}" clean)
endif()

execute_process(COMMAND "${MAKE}" -C "${DIR}" all "CONFIG=${CONFIG}"
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "The bootloader did not build (${MAKE}: ${result})")
endif()
file(WRITE "${STAMP}" "${CONFIG}")
