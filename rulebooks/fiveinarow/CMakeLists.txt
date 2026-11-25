# The Five in a Row rulebook for GameMaster, built with the Retro68 toolchain (tools/build.sh):
#   cmake -S rulebooks/fiveinarow -B build/fiveinarow-rulebook \
#     -DCMAKE_TOOLCHAIN_FILE=<Retro68>/m68k-apple-macos/cmake/retro68.toolchain.cmake
#   cmake --build build/fiveinarow-rulebook
# One flat code resource (PROC 128) whose first instruction leads to RULES (rulebooks/common/rb.c); the
# rulebook file (type GMRB, creator GMST) comes as MacBinary (.bin) and as a small HFS disk image (.dsk).
cmake_minimum_required(VERSION 3.13)
project(FiveInARowRulebook C)

set(RB_NAME "Five in a Row")
set(COMMON ${CMAKE_SOURCE_DIR}/../common)

add_executable(FiveInARow src/fiveinarow.c ${COMMON}/rb.c)
target_include_directories(FiveInARow PRIVATE ${COMMON})
target_compile_options(FiveInARow PRIVATE -Os -m68000 -fno-zero-initialized-in-bss -Wall -Wno-multichar -Wno-array-bounds)
set_target_properties(FiveInARow PROPERTIES OUTPUT_NAME FiveInARow.flt
	LINK_FLAGS "-m68000 -Wl,--mac-flat -Wl,-e,RULES")

set(REZ_INC)
foreach(p ${REZ_INCLUDE_PATH})
	list(APPEND REZ_INC -I${p})
endforeach()

add_custom_command(
	OUTPUT "${RB_NAME}.bin" "${RB_NAME}.dsk"
	COMMAND ${REZ} ${CMAKE_SOURCE_DIR}/src/FiveInARow.r ${REZ_INC} -I${COMMON} -I${CMAKE_CURRENT_BINARY_DIR}
		-o "${RB_NAME}.bin" --cc "${RB_NAME}.dsk" -t GMRB -c GMST
	DEPENDS FiveInARow ${CMAKE_SOURCE_DIR}/src/FiveInARow.r ${COMMON}/GMRZ.r)
add_custom_target(FiveInARow_RB ALL DEPENDS "${RB_NAME}.bin")
