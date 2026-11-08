if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/help.h")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/help.h.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/help.h"
        COPYONLY
    )
endif()

target_sources(${target} PRIVATE
    main.c
    main.h

    tek4010.c

    ards.c

    tube.c
    tube.h

    help.h
)
