function(protegen_generate)
    cmake_parse_arguments(PG 
        "YES_DOXYGEN;NO_MARKDOWN;NO_ABOUT_SECTION;SHOW_HIDDEN;OMIT_HIDDEN;NO_HELPER_FILES;NO_CSS;NO_UNRECOGNIZED;TABLE_OF_CONTENTS;LANG_C;LANG_CPP" 
        "INPUT_FILE;OUTPUT_PATH"
        "DOCS_PATH;LICENSE_FILE;TITLE_PAGE;STYLE_FILE;TRANSLATE_MACRO" 
        ${ARGN})
    
    if (PG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unrecognized arguments: ${PG_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT PG_INPUT_FILE)
        message(FATAL_ERROR "INPUT_FILE argument is required")
    endif()

    if(NOT PG_OUTPUT_PATH)
        set(PG_OUTPUT_PATH ".")
    endif()

    # Resolve the ProtoGen executable path
    if (NOT EXISTS ${PROTOGEN_EXECUTABLE})
        message(FATAL_ERROR "ProtoGen executable not found at ${PROTOGEN_EXECUTABLE}. Ensure it is installed correctly.")
    else()
        message(STATUS "Found ProtoGen executable at ${PROTOGEN_EXECUTABLE}")
    endif()

    set(PG_CMD "${PROTOGEN_EXECUTABLE}" "${PG_INPUT_FILE}" "${PG_OUTPUT_PATH}")

    if(PG_DOCS_PATH)
        list(APPEND PG_CMD "-docs" "${PG_DOCS_PATH}")
    endif()

    if(PG_LICENSE_FILE)
        list(APPEND PG_CMD "-license" "${PG_LICENSE_FILE}")
    endif()

    if(PG_TITLE_PAGE)
        list(APPEND PG_CMD "-titlepage" "${PG_TITLE_PAGE}")
    endif()

    if(PG_STYLE_FILE)
        list(APPEND PG_CMD "-style" "${PG_STYLE_FILE}")
    endif()

    if(PG_TRANSLATE_MACRO)
        list(APPEND PG_CMD "-translate" "${PG_TRANSLATE_MACRO}")
    endif()

    if(PG_YES_DOXYGEN)
        list(APPEND PG_CMD "-yes-doxygen")
    endif()

    if(PG_NO_MARKDOWN)
        list(APPEND PG_CMD "-no-markdown")
    endif()

    if(PG_NO_ABOUT_SECTION)
        list(APPEND PG_CMD "-no-about-section")
    endif()

    if(PG_SHOW_HIDDEN)
        list(APPEND PG_CMD "-show-hidden")
    endif()

    if(PG_OMIT_HIDDEN)
        list(APPEND PG_CMD "-omit-hidden")
    endif()

    if(PG_NO_HELPER_FILES)
        list(APPEND PG_CMD "-no-helper-files")
    endif()

    if(PG_NO_CSS)
        list(APPEND PG_CMD "-no-css")
    endif()

    if(PG_NO_UNRECOGNIZED)
        list(APPEND PG_CMD "-no-unrecognized")
    endif()

    if(PG_TABLE_OF_CONTENTS)
        list(APPEND PG_CMD "-table-of-contents")
    endif()

    if(PG_LANG_C)
        list(APPEND PG_CMD "-lang-c")
    endif()

    if(PG_LANG_CPP)
        list(APPEND PG_CMD "-lang-cpp")
    endif()
    
    message(STATUS "Protogen Command: ${PG_CMD}")
    execute_process(
        COMMAND ${PG_CMD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    if (NOT result EQUAL 0)
        message(FATAL_ERROR "ProtoGen failed with error: ${error_output}")
    else()
        message(STATUS "ProtoGen output:\n${output}")
    endif()

endfunction()
