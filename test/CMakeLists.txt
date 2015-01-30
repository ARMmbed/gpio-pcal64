# Copyright (C) 2013 ARM Limited. All rights reserved.

add_executable(testpcal64
    testpcal64.m
)
set_target_properties(testpcal64 PROPERTIES
    COMPILE_FLAGS "${CMAKE_OBJC_FLAGS}"
)
target_link_libraries(testpcal64
    libobjc2  # must always be first!
    gpio-pcal64
    bootstrap
    yottos
    yttimer
    yottos-platform
    libc
)
