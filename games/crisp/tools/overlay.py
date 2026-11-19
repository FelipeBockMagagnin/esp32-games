# PlatformIO post-script: links the games' shared RAM, the overlay.ld tools/carts.py writes. It
# goes between the framework's memory.ld, which declares the region it fills, and sections.ld,
# the script its INSERT refers to: ld inserts a script's statements only into one read after it.
from os.path import join

Import("env")  # noqa: F821
flags = list(env["LINKFLAGS"])  # noqa: F821
at = flags.index("sections.ld") - 1
flags[at:at] = ["-T", join(env.subst("$BUILD_DIR"), "carts", "overlay.ld")]  # noqa: F821
env.Replace(LINKFLAGS=flags)  # noqa: F821
