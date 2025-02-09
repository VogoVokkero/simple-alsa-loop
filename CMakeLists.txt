cmake_minimum_required(VERSION 3.9)

project(esg-bsp-test VERSION 1.0.1 DESCRIPTION "ESG test-stub for audio and SPI")

# build with DLT, but by default, traces will be muted, so we can decouple MII stress from audio + SPI.
add_definitions(-DENABLE_DLT=${ENABLE_DLT})

find_package(PkgConfig REQUIRED)
pkg_check_modules(CDLT REQUIRED dlt-client)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(GPIOD REQUIRED libgpiod)

include(GNUInstallDirs)

add_executable(esg-bsp-test
    main.c
    options/cmdline.c
    audio/alsa-audio-runner-poll.c
    audio/alsa-device.c
    uart/elite-uart-runner.c
    gpiod/elite-gpiod-runner.c
    gpiod/elite-slave-ready-gpio.c
    auvitran/avxSpi.c
    auvitran/rack-runner.c
    auvitran/rackAuvitran.c
    spidev/esg-spidev.c
    stm32/stm32-runner.c
    multi_core_tools/wi_time.c
    )

add_definitions(-g -O0 -fstack-protector-strong -fno-omit-frame-pointer)

target_link_libraries(esg-bsp-test -pthread
    ${CDLT_LIBRARIES}
    ${GPIOD_LIBRARIES}
    ${ALSA_LIBRARIES})

target_include_directories(esg-bsp-test PUBLIC ./inc ./multi_core_tools  ./spidev ./gpiod ./stm32 ./auvitran
    ${CDLT_INCLUDE_DIRS}
    ${GPIOD_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS})

target_compile_options(esg-bsp-test PUBLIC
    ${CDLT_CFLAGS_OTHER}
    ${GPIOD_CFLAGS_OTHER}
    ${ALSA_CFLAGS_OTHER})

install(TARGETS esg-bsp-test DESTINATION bin)


# also built ALSA example app.
add_subdirectory(audio)
