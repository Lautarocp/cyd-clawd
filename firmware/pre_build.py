"""Remove ARM-only SIMD assembly files from LVGL before building for ESP32 (Xtensa)."""
Import("env")
import os, glob

lvgl_path = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"),
                         env.subst("$PIOENV"), "lvgl")
patterns = ["**/helium/*.S", "**/neon/*.S"]
for pat in patterns:
    for f in glob.glob(os.path.join(lvgl_path, pat), recursive=True):
        print(f"[pre_build] removing ARM-only: {os.path.relpath(f, lvgl_path)}")
        os.remove(f)
