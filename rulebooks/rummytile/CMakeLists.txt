# The Rummy Tile rulebook for GameMaster, built with the Retro68 toolchain (tools/build.sh):
#   cmake -S rulebooks/rummytile -B build/rummytile-rulebook \
#     -DCMAKE_TOOLCHAIN_FILE=<Retro68>/m68k-apple-macos/cmake/retro68.toolchain.cmake
#   cmake --build build/rummytile-rulebook
# One flat code resource (PROC 128) whose first instruction leads to RULES (rulebooks/common/rb.c); the
# rulebook file (type GMRB, creator GMST) comes as MacBinary (.bin) and as a small HFS disk image (.dsk).
cmake_minimum_required(VERSION 3.13)
project(RummyTileRulebook C)

set(RB_NAME "Rummy Tile")
set(COMMON ${CMAKE_SOURCE_DIR}/../common)

add_executable(RummyTile src/rummytile.c ${COMMON}/rb.c)
target_include_directories(RummyTile PRIVATE ${COMMON})
target_compile_options(RummyTile PRIVATE -Os -m68000 -fno-zero-initialized-in-bss -Wall -Wno-multichar -Wno-array-bounds)
set_target_properties(RummyTile PROPERTIES OUTPUT_NAME RummyTile.flt
	LINK_FLAGS "-m68000 -Wl,--mac-flat -Wl,-e,RULES")

set(REZ_INC)
foreach(p ${REZ_INCLUDE_PATH})
	list(APPEND REZ_INC -I${p})
endforeach()

add_custom_command(
	OUTPUT "${RB_NAME}.bin" "${RB_NAME}.dsk"
	COMMAND ${REZ} ${CMAKE_SOURCE_DIR}/src/RummyTile.r ${REZ_INC} -I${COMMON} -I${CMAKE_CURRENT_BINARY_DIR}
		-o "${RB_NAME}.bin" --cc "${RB_NAME}.dsk" -t GMRB -c GMST
	DEPENDS RummyTile ${CMAKE_SOURCE_DIR}/src/RummyTile.r ${COMMON}/GMRZ.r)
add_custom_target(RummyTile_RB ALL DEPENDS "${RB_NAME}.bin")
