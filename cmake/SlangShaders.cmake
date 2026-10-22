# Compiles Slang shaders to SPIR-V

find_program(SLANGC_EXECUTABLE slangc
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
    REQUIRED)
message(STATUS "Using slangc: ${SLANGC_EXECUTABLE}")

function(add_slang_shaders target)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDE_DIRS" ${ARGN})

    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(multi_config)
        set(out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders")
    else()
        set(out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
    endif()
    set(include_flags "")
    foreach(dir IN LISTS ARG_INCLUDE_DIRS)
        list(APPEND include_flags -I "${dir}")
    endforeach()

    set(spv_files "")
    foreach(src IN LISTS ARG_SOURCES)
        get_filename_component(src_abs "${src}" ABSOLUTE)
        get_filename_component(name "${src}" NAME_WE)
        set(spv "${out_dir}/${name}.spv")
        set(dep "${CMAKE_CURRENT_BINARY_DIR}/slang/$<CONFIG>/${name}.d")

        add_custom_command(
            OUTPUT "${spv}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${out_dir}" "${CMAKE_CURRENT_BINARY_DIR}/slang/$<CONFIG>"
            COMMAND "${SLANGC_EXECUTABLE}" "${src_abs}"
                -target spirv
                -profile spirv_1_6
                -fvk-use-entrypoint-name
                -matrix-layout-column-major
                ${include_flags}
                "$<IF:$<CONFIG:Debug>,-g;-O0,-O2>"
                -depfile "${dep}"
                -o "${spv}"
            DEPENDS "${src_abs}"
            DEPFILE "${dep}"
            COMMAND_EXPAND_LISTS
            COMMENT "slangc ${name}.slang"
            VERBATIM)
        list(APPEND spv_files "${spv}")
    endforeach()

    add_custom_target(${target}_shaders DEPENDS ${spv_files} SOURCES ${ARG_SOURCES})
    add_dependencies(${target} ${target}_shaders)
endfunction()
