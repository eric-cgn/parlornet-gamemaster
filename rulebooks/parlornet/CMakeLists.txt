# The ParlorNet rulebook for GameMaster, built with the Retro68 toolchain (tools/build.sh):
#   cmake -S rulebooks/parlornet -B build/parlornet-rulebook \
#     -DCMAKE_TOOLCHAIN_FILE=<Retro68>/m68k-apple-macos/cmake/retro68.toolchain.cmake
#   cmake --build build/parlornet-rulebook
# The code is one flat code resource (PROC 128) whose first instruction leads to RULES; the rulebook file
# (type GMRB, creator GMST) comes as MacBinary (.bin) and as a small HFS disk image (.dsk).
cmake_minimum_required(VERSION 3.13)
project(ParlorNetRulebook C)

set(RB_NAME "ParlorNet")

add_executable(ParlorNet src/lobby.c src/tunnel.c src/tcp.c src/machine.c)
target_compile_options(ParlorNet PRIVATE -Os -m68000 -fno-zero-initialized-in-bss -Wall -Wno-multichar -Wno-array-bounds)
set_target_properties(ParlorNet PROPERTIES OUTPUT_NAME ParlorNet.flt
	LINK_FLAGS "-m68000 -Wl,--mac-flat -Wl,-e,RULES")

set(REZ_INC)
foreach(p ${REZ_INCLUDE_PATH})
	list(APPEND REZ_INC -I${p})
endforeach()

add_custom_command(
	OUTPUT "${RB_NAME}.bin" "${RB_NAME}.dsk"
	COMMAND ${REZ} ${CMAKE_SOURCE_DIR}/src/ParlorNet.r ${REZ_INC} -I${CMAKE_SOURCE_DIR}/../common -I${CMAKE_CURRENT_BINARY_DIR}
		-o "${RB_NAME}.bin" --cc "${RB_NAME}.dsk" -t GMRB -c GMST
	DEPENDS ParlorNet ${CMAKE_SOURCE_DIR}/src/ParlorNet.r ${CMAKE_SOURCE_DIR}/../common/GMRZ.r)
add_custom_target(ParlorNet_RB ALL DEPENDS "${RB_NAME}.bin")
