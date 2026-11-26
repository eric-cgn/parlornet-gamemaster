#!/bin/sh
# Builds ParlorNet's rulebooks for GameMaster with Retro68 in Docker, and Install ParlorNet Rulebooks with EUP,
# as the release workflow does (.github/workflows/release.yml), into build/ (git-ignored):
#
#   build/parlornet-rulebook.bin                the ParlorNet rulebook (rulebooks/parlornet)
#   build/<game>-rulebook.bin                   the game rulebooks, one for each folder of rulebooks/ named
#                                               below (checkers, mancala, ...)
#   build/parlornet-rulebooks-installer.bin .hqx .dsk
#                                               Install ParlorNet Rulebooks, all the rulebooks above in one
#                                               installer (rulebooks/installer)
#   build/release/                              the release files, which are the installer's .hqx, .bin and
#                                               .dsk and nothing else
#
# Each rulebook's .bin (MacBinary) comes from Retro68's Rez, with its dates set to the rulebook's release date
# by tools/patcher_dates.py, so that the same source always gives the same file. The rulebooks go into the
# installer and are not released on their own. EUP writes the installer's .bin and .hqx, dated to the release
# date by SOURCE_DATE_EPOCH. The installer's .dsk is an 800K HFS disk image holding the installer, for
# emulators and floppy writers. It is made with the hfsutils of the same Retro68 image, which keep the
# installer's type, creator, Finder flags and dates, and tools/patcher_dates.py sets the volume's dates.
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

build() {   # build <source folder> <build name> <file name> <release date>
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
	python3 tools/patcher_dates.py "$day" "$OUT/$dir.bin" "$OUT/$dir/$app.dsk"
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
DAY=2026-11-26
INSTALLER=parlornet-rulebooks-installer
SOURCE_DATE_EPOCH=$(python3 -c "import datetime; print(int(datetime.datetime.strptime('$DAY 12', '%Y-%m-%d %H').replace(tzinfo=datetime.timezone.utc).timestamp()))") \
	"$EUP" build "$stage/rulebooks.ini" -o "$OUT"

# The installer on an 800K HFS disk. hcopy -m copies it from its MacBinary with both forks, its type, creator,
# Finder flags and dates. patcher_dates.py sets the volume's dates on the disk; it is given a copy of the .bin,
# since the installer's own .bin and .hqx stay as EUP wrote them.
rm -f "$OUT/$INSTALLER.dsk"
docker run --rm --platform linux/amd64 --network none -u "$(id -u):$(id -g)" -v "$PWD:/src" -w /src "$IMAGE" sh -c "
	set -e
	export HOME=/tmp
	dd if=/dev/zero of=$OUT/$INSTALLER.dsk bs=1024 count=800 2>/dev/null
	hformat -l 'Install ParlorNet Rulebooks' $OUT/$INSTALLER.dsk >/dev/null
	hcopy -m $OUT/$INSTALLER.bin :
	humount
"
cp "$OUT/$INSTALLER.bin" "$OUT/$INSTALLER-dates.bin"
python3 tools/patcher_dates.py "$DAY" "$OUT/$INSTALLER-dates.bin" "$OUT/$INSTALLER.dsk"
rm "$OUT/$INSTALLER-dates.bin"

rel="$OUT/release"
rm -rf "$rel"
mkdir -p "$rel"
for ext in hqx bin dsk; do
	cp "$OUT/$INSTALLER.$ext" "$rel/"
done
ls -l "$rel"
(cd "$rel" && python3 -c '
import hashlib, sys
for name in sys.argv[1:]:
    with open(name, "rb") as f:
        print("%s  %s" % (hashlib.sha256(f.read()).hexdigest(), name))
' $INSTALLER.hqx $INSTALLER.bin $INSTALLER.dsk)
