file(GLOB_RECURSE srcs "Src/*.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "Inc/")
