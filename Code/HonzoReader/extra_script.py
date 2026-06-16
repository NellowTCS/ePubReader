import os
Import("env")

# Add lexepub static library to linker
lib_path = os.path.join(
    env.subst("$PROJECT_DIR"),
    "lib", "lexepub_c", "xtensa-esp32s3-espidf", "release"
)
env.Prepend(LIBPATH=[lib_path])
env.Prepend(LIBS=["lexepub"])
