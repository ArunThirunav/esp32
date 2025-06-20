file(GLOB_RECURSE srcs "*.c")

idf_component_register(SRCS "main.c" "crc.c" "${srcs}"
                       INCLUDE_DIRS ".")
