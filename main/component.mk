#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
COMPONENT_PRIV_INCLUDEDIRS +=   .                   \
                                blue                \
                                blue/a2dp                \
                                blue/ble                \
                                blue/ble/profiles                \
                                blue/spp                \
                                blue/ota                \
                                blue/spp               \
                                system                \
                                uta                \
                                comms                \
                                comms/pic                \
                                peripherals                \



COMPONENT_SRCDIRS +=    .                      \
                                blue                \
                                blue/a2dp                \
                                blue/ble                \
                                blue/ble/profiles                \
                                blue/ota                \
                                blue/spp              \
                                system                \
                                uta                \
                                comms                \
                                comms/pic                \
                                peripherals                \


$(call compile_only_if,$(CONFIG_IS_SUBWOOFER),main_sub.o)
$(call compile_only_if_not,$(CONFIG_IS_SUBWOOFER),main_sat.o)
