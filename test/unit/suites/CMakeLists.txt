MACRO(ADD_MODULE file)
    ADD_LIBRARY( ${file} MODULE ${file}.c ../utils.c ../panic.c )
    TARGET_LINK_LIBRARIES( ${file} cunit kern phantom win )
    SET_TARGET_PROPERTIES( ${file} PROPERTIES
            PREFIX ""
            LIBRARY_OUTPUT_DIRECTORY "."
    )   
ENDMACRO(ADD_MODULE file)

FILE(GLOB C_FILES RELATIVE "${CMAKE_SOURCE_DIR}/suites" "${CMAKE_SOURCE_DIR}/suites/*.c")

INCLUDE_DIRECTORIES ( "${CMAKE_SOURCE_DIR}" )
INCLUDE_DIRECTORIES ( "../../../include" "../../../include/ia32" )


SET(CMAKE_C_FLAGS " -std=c99 -O3 -Wall -Wextra -Wimplicit -DARCH_ia32 -DBOARD_ia32-default -include ../../../include/kernel/config.h -include ../../../include/ia32/arch/arch-config.h  -include ../../../include/ia32/arch/board-ia32_default-config.h -L../../../phantom/lib/")


FOREACH ( module ${C_FILES} )
    STRING( REGEX REPLACE ".c$" "" module "${module}" )
    MESSAGE(STATUS "Found test suite: ${module}")
    ADD_MODULE(${module})
ENDFOREACH ( module ${MODULES} )
