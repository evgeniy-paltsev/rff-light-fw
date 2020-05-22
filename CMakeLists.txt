cmake_minimum_required(VERSION 3.13.1)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD rff-light-alarm-v0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(light-alarm)

target_compile_options(app PRIVATE "-lm")

target_link_libraries(app PUBLIC -lm)
#target_link_libraries(app PUBLIC -lc)
target_link_libraries(app PUBLIC -lgcc)


# Add your source file to the "app" target. This must come after
# the boilerplate code, which defines the target.
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/button-ctl.c)
target_sources(app PRIVATE src/status-led-ctl.c)
target_sources(app PRIVATE src/bt/bt-hm11-ctl.c)
target_sources(app PRIVATE src/pwm/tim3-pwm.c)
target_sources(app PRIVATE src/rtc/rtc-ctl.c)
target_sources(app PRIVATE src/brightness-model/brightness-unified-model.c)
target_sources(app PRIVATE src/core-model/core-model.c)
