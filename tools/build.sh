#!/bin/sh
# Builds ParlorNet's rulebooks for GameMaster with Retro68 in Docker, and Install ParlorNet Rulebooks with EUP,
# as the release workflow does (.github/workflows/release.yml), into build/ (git-ignored):
#
#   build/parlornet-rulebook.bin .hqx .dsk      the ParlorNet rulebook (rulebooks/parlornet)
#   build/<game>-rulebook.bin .hqx .dsk         the game rulebooks, one for each folder of rulebooks/ named
#                                               below (checkers, mancala, ...)
#   build/parlornet-rulebooks-installer.bin .hqx
#                                               Install ParlorNet Rulebooks, all the rulebooks above in one
#                                               installer (rulebooks/installer)
#   build/release/                              the release files: all of the above, the two READMEs
#                                               (release/), and SHA256SUMS and MD5SUMS of them all
#
# The .bin (MacBinary) and the .dsk (an 800K HFS disk holding the file) come from Retro68's Rez, with
# their dates set to the release date by tools/patcher_dates.py, so that the same source always
# gives the same files; the .hqx is made from the .bin by tools/binhex4.py. The installer's date is the
# release date too (SOURCE_DATE_EPOCH).
#
# RETRO68_IMAGE overrides the toolchain image (pinned by digest: the one the releases are built with).
# EUP names an eup command to use; without it, EUP is fetched from its public repository at the pinned
# commit below (release 1.4) into build/eup and built with its Makefile, which needs git and a C compiler.
set -eu
cd "$(dirname "$0")/.."
IMAGE="${RETRO68_IMAGE:-ghcr.io/autc04/retro68@sha256:96ae710e8c80ac92a90da19598115a572ed6a50143b713ef7444fadc0ea0fef9}"
EUP_REPO=https://github.com/eric-cgn/eup.git
EUP_COMMIT=d72556e6373fc1b5e99162394763167875199a96
TC=/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
OUT=build
mkdir -p "$OUT"

if [ -z "${EUP:-}" ]; then
	if [ ! -d "$OUT/eup/.git" ]; then
		git clone --quiet "$EUP_REPO" "$OUT/eup"
	fi
	git -C "$OUT/eup" checkout --quiet "$EUP_COMMIT"
	make -C "$OUT/eup" eup >/dev/null
	EUP="$PWD/$OUT/eup/eup"
fi

build() {   # build <source folder> <download name> <file name> <release date>
	src=$1
	dir=$2
	app=$3
	day=$4
	rm -rf "$OUT/$dir"
	docker run --rm --platform linux/amd64 -u "$(id -u):$(id -g)" -v "$PWD:/src" -w /src "$IMAGE" sh -c "
		set -e
		cmake -S $src -B $OUT/$dir -DCMAKE_TOOLCHAIN_FILE=$TC -DCMAKE_BUILD_TYPE=MinSizeRel >/dev/null
		cmake --build $OUT/$dir >/dev/null
	"
	cp "$OUT/$dir/$app.bin" "$OUT/$dir.bin"
	cp "$OUT/$dir/$app.dsk" "$OUT/$dir.dsk"
	python3 tools/patcher_dates.py "$day" "$OUT/$dir.bin" "$OUT/$dir.dsk"
	python3 tools/binhex4.py "$OUT/$dir.bin" "$OUT/$dir.hqx"
}

build rulebooks/parlornet parlornet-rulebook ParlorNet 2026-11-24
build rulebooks/checkers checkers-rulebook "Checkers" 2026-11-22
build rulebooks/mancala mancala-rulebook "Mancala" 2026-11-22
build rulebooks/fiveinarow fiveinarow-rulebook "Five in a Row" 2026-11-22
build rulebooks/gridwords gridwords-rulebook "Grid Words" 2026-11-22
build rulebooks/rummytile rummytile-rulebook "Rummy Tile" 2026-11-22
build rulebooks/cannonfire cannonfire-rulebook "Cannon Fire" 2026-11-22
build rulebooks/backgammon backgammon-rulebook "Backgammon" 2026-11-24

stage="$OUT/rulebooks-installer"
rm -rf "$stage"
mkdir -p "$stage"
cp rulebooks/installer/rulebooks.ini rulebooks/installer/About.txt "$stage/"
cp "$OUT/parlornet-rulebook.bin" "$stage/ParlorNet.bin"
cp "$OUT/checkers-rulebook.bin" "$stage/Checkers.bin"
cp "$OUT/mancala-rulebook.bin" "$stage/Mancala.bin"
cp "$OUT/fiveinarow-rulebook.bin" "$stage/Five in a Row.bin"
cp "$OUT/gridwords-rulebook.bin" "$stage/Grid Words.bin"
cp "$OUT/rummytile-rulebook.bin" "$stage/Rummy Tile.bin"
cp "$OUT/cannonfire-rulebook.bin" "$stage/Cannon Fire.bin"
cp "$OUT/backgammon-rulebook.bin" "$stage/Backgammon.bin"
# the release date, for an installer that comes out the same on every build
SOURCE_DATE_EPOCH=$(python3 -c 'import datetime; print(int(datetime.datetime(2026, 11, 24, 12, tzinfo=datetime.timezone.utc).timestamp()))') \
	"$EUP" build "$stage/rulebooks.ini" -o "$OUT"

rel="$OUT/release"
rm -rf "$rel"
mkdir -p "$rel"
files="parlornet-rulebooks-installer.hqx parlornet-rulebooks-installer.bin"
for slug in parlornet checkers mancala fiveinarow gridwords rummytile cannonfire backgammon; do
	files="$files $slug-rulebook.hqx $slug-rulebook.bin $slug-rulebook.dsk"
done
for f in $files; do
	cp "$OUT/$f" "$rel/"
done
cp release/parlornet-rulebook-readme.txt release/gamemaster-rulebooks-readme.txt "$rel/"
files="$files parlornet-rulebook-readme.txt gamemaster-rulebooks-readme.txt"
(
	cd "$rel"
	python3 - $files <<'EOF'
import hashlib, sys
for algo, out in (("sha256", "SHA256SUMS"), ("md5", "MD5SUMS")):
    with open(out, "w") as f:
        for name in sys.argv[1:]:
            with open(name, "rb") as g:
                f.write("%s  %s\n" % (hashlib.new(algo, g.read()).hexdigest(), name))
EOF
)
ls -l "$rel"
cat "$rel/SHA256SUMS"
