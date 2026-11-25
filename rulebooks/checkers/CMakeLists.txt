# The Checkers rulebook for GameMaster, built with the Retro68 toolchain (tools/build.sh):
#   cmake -S rulebooks/checkers -B build/checkers-rulebook \
#     -DCMAKE_TOOLCHAIN_FILE=<Retro68>/m68k-apple-macos/cmake/retro68.toolchain.cmake
#   cmake --build build/checkers-rulebook
# One flat code resource (PROC 128) whose first instruction leads to RULES (rulebooks/common/rb.c); the
# rulebook file (type GMRB, creator GMST) comes as MacBinary (.bin) and as a small HFS disk image (.dsk).
cmake_minimum_required(VERSION 3.13)
project(CheckersRulebook C)

set(RB_NAME "Checkers")
set(COMMON ${CMAKE_SOURCE_DIR}/../common)

add_executable(Checkers src/checkers.c ${COMMON}/rb.c)
target_include_directories(Checkers PRIVATE ${COMMON})
target_compile_options(Checkers PRIVATE -Os -m68000 -fno-zero-initialized-in-bss -Wall -Wno-multichar -Wno-array-bounds)
set_target_properties(Checkers PROPERTIES OUTPUT_NAME Checkers.flt
	LINK_FLAGS "-m68000 -Wl,--mac-flat -Wl,-e,RULES")

set(REZ_INC)
foreach(p ${REZ_INCLUDE_PATH})
	list(APPEND REZ_INC -I${p})
endforeach()

add_custom_command(
	OUTPUT "${RB_NAME}.bin" "${RB_NAME}.dsk"
	COMMAND ${REZ} ${CMAKE_SOURCE_DIR}/src/Checkers.r ${REZ_INC} -I${COMMON} -I${CMAKE_CURRENT_BINARY_DIR}
		-o "${RB_NAME}.bin" --cc "${RB_NAME}.dsk" -t GMRB -c GMST
	DEPENDS Checkers ${CMAKE_SOURCE_DIR}/src/Checkers.r ${COMMON}/GMRZ.r)
add_custom_target(Checkers_RB ALL DEPENDS "${RB_NAME}.bin")
