"""ParlorNet's rulebooks for GameMaster and their installer: the sources (license, no credits, plain 68000 code for
the flat PROC, the GMRZ resource), the versions, the build script, the release workflow and READMEs, and the built
files when tools/build.sh has made them.

    python3 -m unittest discover -s tests
"""
import collections
import hashlib
import os
import re
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")
sys.path.insert(0, os.path.join(ROOT, "tools"))
IMAGE = "ghcr.io/autc04/retro68@sha256:96ae710e8c80ac92a90da19598115a572ed6a50143b713ef7444fadc0ea0fef9"
INSTALLER = "parlornet-rulebooks-installer"
CREDITS = (b"Eric", b"Claude", b"Anthropic", b"Credits", b"Thaller")

# The versions the ParlorNet server's GameMaster extension is known to work with (README, "The ParlorNet server
# they work with"): a change here goes with a change there.
GAME_VERSION, GAME_GMRZ, GAME_OLDEST = "1.1.1", "0x0111", "0x0100"
GRIDWORDS_VERSION, GRIDWORDS_GMRZ = "1.2.0", "0x0120"
LOBBY_VERSION, INSTALLER_VERSION = "0.6", "1.6"

# slug: the downloads' name; name: the file on the Mac; folder: under rulebooks/; stem: its .r file; rb_id: its
# GMRZ ID string; version and gmrz: its version as the vers and GMRZ resources give it.
Rulebook = collections.namedtuple("Rulebook", "slug name folder stem rb_id version gmrz")
RULEBOOKS = [
    Rulebook("checkers-rulebook", "Checkers", "checkers", "Checkers", "Checkers [ParlorNet]",
             GAME_VERSION, GAME_GMRZ),
    Rulebook("mancala-rulebook", "Mancala", "mancala", "Mancala", "Mancala [ParlorNet]", GAME_VERSION, GAME_GMRZ),
    Rulebook("fiveinarow-rulebook", "Five in a Row", "fiveinarow", "FiveInARow", "Five in a Row [ParlorNet]",
             GAME_VERSION, GAME_GMRZ),
    Rulebook("gridwords-rulebook", "Grid Words", "gridwords", "GridWords", "Grid Words [ParlorNet]",
             GRIDWORDS_VERSION, GRIDWORDS_GMRZ),
    Rulebook("rummytile-rulebook", "Rummy Tile", "rummytile", "RummyTile", "Rummy Tile [ParlorNet]",
             GAME_VERSION, GAME_GMRZ),
    Rulebook("cannonfire-rulebook", "Cannon Fire", "cannonfire", "CannonFire", "Cannon Fire [ParlorNet]",
             GAME_VERSION, GAME_GMRZ),
    Rulebook("backgammon-rulebook", "Backgammon", "backgammon", "Backgammon", "Backgammon [ParlorNet]",
             GAME_VERSION, GAME_GMRZ),
]
PARLORNET = Rulebook("parlornet-rulebook", "ParlorNet", "parlornet", "ParlorNet", "ParlorNet", LOBBY_VERSION,
                     "0x0060")


def read(*path, mode="r"):
    with open(os.path.join(ROOT, *path), mode) as f:
        return f.read()


def built(slug, ext):
    path = os.path.join(BUILD, "%s.%s" % (slug, ext))
    if not os.path.isfile(path):
        return None
    with open(path, "rb") as f:
        return f.read()


class Sources(unittest.TestCase):

    def test_no_credits(self):
        """No file of the rulebooks or the release READMEs names a person or a credits list."""
        for top in ("rulebooks", "release"):
            for d, _, fs in os.walk(os.path.join(ROOT, top)):
                for f in fs:
                    with open(os.path.join(d, f), "rb") as fh:
                        data = fh.read()
                    for credit in CREDITS:
                        self.assertNotIn(credit, data, os.path.join(d, f))

    def test_the_rulebook_sources(self):
        """The ParlorNet rulebook's source: its license, plain 68000 code for the flat PROC, and a GMRZ whose author
        is empty and whose copyright line is the license."""
        base = os.path.join(ROOT, "rulebooks", "parlornet")
        for f in ("CMakeLists.txt", "LICENSE", "README.md", "src/lobby.c", "src/tunnel.c", "src/tcp.c",
                  "src/gm.h", "src/tcp.h", "src/tunnel.h", "src/ParlorNet.r"):
            self.assertTrue(os.path.isfile(os.path.join(base, f)), f)
        cmake = read("rulebooks", "parlornet", "CMakeLists.txt")
        self.assertIn("-m68000", cmake)
        self.assertIn("--mac-flat", cmake)
        r = read("rulebooks", "parlornet", "src", "ParlorNet.r")
        self.assertIn('9, "ParlorNet", 0, "", 11, "MIT license", 9, "ParlorNet",', r)
        self.assertIn('"%s, MIT license"' % LOBBY_VERSION, r)
        self.assertIn('9, "ParlorNet", 0, "", 11, "MIT license", 9, "ParlorNet",\n\t%s,' % PARLORNET.gmrz, r)
        lobby = read("rulebooks", "parlornet", "src", "lobby.c")
        self.assertIn('#define kVersion "%s"' % LOBBY_VERSION, lobby)
        self.assertIn('{ "Grid Words", "gridwords/1 standard", ', lobby)    # the extension's rules id for it
        self.assertIn("build rulebooks/parlornet parlornet-rulebook ParlorNet", read("tools", "build.sh"))
        self.assertEqual(read("rulebooks", "parlornet", "LICENSE"), read("LICENSE"))

    def test_the_game_rulebook_sources(self):
        """Each game rulebook: the license, plain 68000 code for the flat PROC, a GMRZ with no author, the license
        as its copyright line, its own ID string and the version, and the build script and the installer
        building it."""
        script = read("tools", "build.sh")
        ini = read("rulebooks", "installer", "rulebooks.ini")
        for r in RULEBOOKS:
            base = os.path.join(ROOT, "rulebooks", r.folder)
            for f in ("CMakeLists.txt", "LICENSE", "README.md", "src/%s.r" % r.stem):
                self.assertTrue(os.path.isfile(os.path.join(base, f)), (r.folder, f))
            cmake = read("rulebooks", r.folder, "CMakeLists.txt")
            self.assertIn("-m68000", cmake)
            self.assertIn("--mac-flat", cmake)
            rez = read("rulebooks", r.folder, "src", r.stem + ".r")
            self.assertIn('%d, "%s", 0, "", 11, "MIT license", %d, "%s",' % (len(r.name), r.name, len(r.rb_id),
                                                                         r.rb_id), rez)
            self.assertIn("\t%s, %s, 0x0100, 0x0100," % (r.gmrz, GAME_OLDEST), rez)
            self.assertIn('"%s, MIT license"' % r.version, rez)
            major, minor, fix = (int(n) for n in r.version.split("."))
            self.assertEqual(r.gmrz, "0x%02x%x%x" % (major, minor, fix))
            self.assertIn("\t0x%02x, 0x%x%x, release," % (major, minor, fix), rez)
            self.assertIn('build rulebooks/%s %s "%s"' % (r.folder, r.slug, r.name), script)
            self.assertIn('cp "$OUT/%s.bin" "$stage/%s.bin"' % (r.slug, r.name), script)
            self.assertIn("source = %s.bin" % r.name, ini)
            self.assertEqual(read("rulebooks", r.folder, "LICENSE"), read("LICENSE"))
        self.assertIn("source = ParlorNet.bin", ini)
        self.assertIn("detect = GameMaster\nrequire = GameMaster", ini)
        for f in ("rb.c", "rb.h", "rulebook.h", "GMRZ.r"):
            self.assertTrue(os.path.isfile(os.path.join(ROOT, "rulebooks", "common", f)), f)

    def test_the_installer(self):
        """The installer's version in its INI, its Get Info text and its first screen agree."""
        ini = read("rulebooks", "installer", "rulebooks.ini")
        self.assertIn("\nversion = %s\n" % INSTALLER_VERSION, ini)
        self.assertIn("get_info = Install ParlorNet Rulebooks %s," % INSTALLER_VERSION, ini)
        self.assertIn("output = %s\n" % INSTALLER, ini)
        self.assertTrue(read("rulebooks", "installer", "About.txt").startswith(
            "ParlorNet Rulebooks %s\n" % INSTALLER_VERSION))

    def test_the_build(self):
        """The build script uses the pinned Retro68 image and EUP at a pinned commit, and the release workflow
        runs on main, releases only an installer version with no release yet, builds, tests and then publishes."""
        script = read("tools", "build.sh")
        self.assertIn(IMAGE, script)
        self.assertIn("EUP_REPO=https://github.com/eric-cgn/eup.git", script)
        self.assertRegex(script, r"\nEUP_COMMIT=[0-9a-f]{40}\n")
        workflow = read(".github", "workflows", "release.yml")
        self.assertIn("branches: [main]", workflow)
        self.assertIn('gh release view "v$v"', workflow)
        self.assertIn("rulebooks/installer/rulebooks.ini", workflow)
        self.assertLess(workflow.index("tools/build.sh"), workflow.index("unittest discover -s tests"))
        self.assertLess(workflow.index("unittest discover -s tests"), workflow.index("gh release create"))
        # the release is the installer as .hqx, .bin and .dsk, each linked from the notes, and nothing else
        create = workflow[workflow.index("gh release create"):]
        self.assertEqual(re.findall(r"build/release/\S+", create),
                         ["build/release/$f.%s" % e for e in ("hqx", "bin", "dsk")])
        self.assertIn('f=%s\n' % INSTALLER, workflow)
        for e in ("hqx", "bin", "dsk"):
            self.assertIn("[$f.%s]($url/$f.%s)" % (e, e), workflow)
        self.assertIn('url="https://github.com/$GITHUB_REPOSITORY/releases/download/v$VERSION"', workflow)
        self.assertNotIn("SUMS", workflow)
        self.assertNotIn("readme", workflow.lower())
        # dev builds are kept as CI artifacts, never published as a release
        self.assertFalse(os.path.exists(os.path.join(ROOT, ".github", "workflows", "dev.yml")))
        ci = read(".github", "workflows", "ci.yml")
        self.assertIn("branches: [dev, main]", ci)
        self.assertNotIn("gh release", ci)
        self.assertIn("for ext in hqx bin dsk; do", script)
        for d, _, fs in os.walk(os.path.join(ROOT, "rulebooks")):      # built, never committed
            for f in fs:
                self.assertFalse(f.endswith((".bin", ".hqx", ".dsk")), os.path.join(d, f))

    def test_the_readme_versions(self):
        """The repository's README names the versions the server's extension works with."""
        readme = read("README.md")
        for text in ("ParlorNet rulebook %s" % LOBBY_VERSION, "game rulebooks %s" % GAME_VERSION, "`%s`" % GAME_GMRZ,
                     "Grid Words %s" % GRIDWORDS_VERSION, "`%s`" % GRIDWORDS_GMRZ, "PARLORNET/1",
                     "Install ParlorNet Rulebooks %s" % INSTALLER_VERSION, "port 6508"):
            self.assertIn(text, readme)


class Releases(unittest.TestCase):
    """The READMEs that come with the downloads."""

    def test_parlornet_rulebook_readme(self):
        readme = read("release", "parlornet-rulebook-readme.txt")
        self.assertTrue(readme.startswith("ParlorNet rulebook for GameMaster %s\n" % LOBBY_VERSION))
        self.assertIn("A Mac Plus or later with System 6.0 or later", readme)
        self.assertTrue(all(len(l) <= 78 for l in readme.splitlines()))
        self.assertIn("MIT License", readme)

    def test_game_rulebooks_readme(self):
        readme = read("release", "gamemaster-rulebooks-readme.txt")
        self.assertTrue(all(len(l) <= 78 for l in readme.splitlines()))
        self.assertIn(INSTALLER + ".hqx", readme)
        self.assertIn(INSTALLER + ".bin", readme)
        self.assertIn("The installer needs System 6.0.3 or later", " ".join(readme.split()))
        self.assertIn("MIT License", readme)
        self.assertIn("https://github.com/eric-cgn/eup", readme)


class Built(unittest.TestCase):
    """The files tools/build.sh made, when it has run."""
    def test_rulebooks(self):
        """Each rulebook's MacBinary, which goes into the installer: its name, type and creator."""
        import binhex4
        for r in [PARLORNET] + RULEBOOKS:
            with self.subTest(r.slug):
                data = built(r.slug, "bin")
                if data is None:
                    self.skipTest("not built (tools/build.sh)")
                mb = binhex4.read_macbinary(data)
                self.assertEqual((mb["name"], mb["type"], mb["creator"]), (r.name.encode(), b"GMRB", b"GMST"))

    def test_installer(self):
        """The installer's .hqx and .bin hold the same file, and its .dsk is an 800K HFS disk that holds it
        (both forks, under its own name); the release files are those three and nothing else, as built."""
        import binhex4
        files = {e: built(INSTALLER, e) for e in ("hqx", "bin", "dsk")}
        if not all(files.values()):
            self.skipTest("not built (tools/build.sh)")
        hqx, mb = binhex4.decode(files["hqx"]), binhex4.read_macbinary(files["bin"])
        self.assertEqual(hqx, mb)
        self.assertEqual((mb["name"], mb["type"], mb["creator"]), (b"Install ParlorNet Rulebooks", b"APPL", b"EUPi"))
        dsk = files["dsk"]
        self.assertEqual(len(dsk), 800 * 1024)
        self.assertEqual(dsk[1024:1026], b"BD")
        self.assertEqual(dsk[1024 + 36:1024 + 37 + dsk[1024 + 36]], b"\x1bInstall ParlorNet Rulebooks")  # volume
        self.assertIn(mb["data"], dsk)
        self.assertIn(mb["rsrc"], dsk)
        release = os.path.join(BUILD, "release")
        if os.path.isdir(release):
            self.assertEqual(sorted(os.listdir(release)), sorted("%s.%s" % (INSTALLER, e) for e in files))
            for e, data in files.items():
                with open(os.path.join(release, "%s.%s" % (INSTALLER, e)), "rb") as f:
                    self.assertEqual(hashlib.sha256(f.read()).hexdigest(), hashlib.sha256(data).hexdigest())


class BinHex(unittest.TestCase):
    """tools/binhex4.py: runs, 0x90 bytes and CRCs."""

    def test_round_trip(self):
        import binhex4
        info = {"name": b"Test", "type": b"TEXT", "creator": b"ttxt", "flags": 0x0100,
                "data": b"\x90" * 7 + b"abc" + b"\0" * 600 + b"\x90x\x90\x90", "rsrc": bytes(range(256)) * 3}
        out = binhex4.encode(info)
        self.assertEqual(binhex4.decode(out), info)
        self.assertLess(len(out), 2000)  # the run of 600 zeros is packed
        bad = bytearray(out)
        bad[-10] = ord("!") if bad[-10] != ord("!") else ord("#")
        with self.assertRaises(ValueError):
            binhex4.decode(bytes(bad))


if __name__ == "__main__":
    unittest.main()
