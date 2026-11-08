
set(target apltest)
add_executable(${target} ${target}.c)
set_target_properties(${target} PROPERTIES FOLDER "tek4010/apl")

# add_custom_target(run_${tektest}
# COMMAND "$<TARGET_FILE:${tektest}>" > ${tektest}.plt
# DEPENDS ${tektest}
# WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
# USES_TERMINAL
# )
# set_target_properties(run_${tektest} PROPERTIES FOLDER "tek4010/tektests/run")

