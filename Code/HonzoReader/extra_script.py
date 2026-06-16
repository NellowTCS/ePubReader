import os
Import("env")

# Restore original working pattern from old lexepub linking
lib_path = os.path.join(
    env.subst("$PROJECT_DIR"),
    "lib", "honzo_c", "xtensa-esp32s3-espidf", "release"
)
env.Prepend(LIBPATH=[lib_path])
env.Prepend(LIBS=["honzo_c"])
