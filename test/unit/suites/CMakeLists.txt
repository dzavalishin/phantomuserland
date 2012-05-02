MACRO(ADD_MODULE file)
    ADD_LIBRARY( ${file} MODULE ${file}.c ../utils.c )
    TARGET_LINK_LIBRARIES( ${file} cunit )
    SET_TARGET_PROPERTIES( ${file} PROPERTIES
            PREFIX ""
            LIBRARY_OUTPUT_DIRECTORY "."
    )   
ENDMACRO(ADD_MODULE file)

FILE(GLOB C_FILES RELATIVE "${CMAKE_SOURCE_DIR}/suites" "${CMAKE_SOURCE_DIR}/suites/*.c")

INCLUDE_DIRECTORIES ( "${CMAKE_SOURCE_DIR}" )

FOREACH ( module ${C_FILES} )
    STRING( REGEX REPLACE ".c$" "" module "${module}" )
    MESSAGE(STATUS "Found test suite: ${module}")
    ADD_MODULE(${module})
ENDFOREACH ( module ${MODULES} )
