# SPDX-License-Identifier: MIT
if(CONFIG_SHIELD_ZAPHOD_TRACKPOINT)
  message(FATAL_ERROR "Select only one Zaphod TrackPoint shield: spare pins or display replacement")
endif()
if(NOT CONFIG_BOARD_ZAPHOD)
  message(FATAL_ERROR "zaphod_trackpoint_display_replacement requires board zaphod")
endif()
zephyr_library()
zephyr_library_sources(unused_display_pins.c)
