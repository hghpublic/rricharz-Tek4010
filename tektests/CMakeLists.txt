
set(tektests
    circles
    dodecagon
    gintest
    incremental
)

foreach(tektest ${tektests})
    add_executable(${tektest} ${tektest}.c)
    if(tektest STREQUAL "dodecagon")
    target_sources(${tektest} PRIVATE tekio.c)
    endif()
    set_target_properties(${tektest} PROPERTIES FOLDER "tek4010/tektests")
    
    add_custom_target(run_${tektest}
    COMMAND "$<TARGET_FILE:${tektest}>" > ${tektest}.plt
    DEPENDS ${tektest}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    USES_TERMINAL
    )
    set_target_properties(run_${tektest} PROPERTIES FOLDER "tek4010/tektests/run")
endforeach()

