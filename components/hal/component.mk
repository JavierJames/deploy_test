#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
COMPONENT_ADD_INCLUDEDIRS := . hal-esp
COMPONENT_SRCDIRS := . hal-esp
$(call compile_only_if,$(CONFIG_IS_SUBWOOFER),hal-esp/HAL_SUB.o)
$(call compile_only_if_not,$(CONFIG_IS_SUBWOOFER),hal-esp/HAL_SAT.o)
