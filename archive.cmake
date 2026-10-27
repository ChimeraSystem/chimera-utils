set(CHIMERA_ARCHIVE_SOURCE ${CMAKE_SOURCE_DIR}/vendor/freebsd-src/contrib/libarchive)
set(CHIMERA_ARCHIVE_CONFIG ${CMAKE_SOURCE_DIR}/base-system/ports/libarchive/config.h)

# freebsd keeps the upstream build system out of its source import
file(READ ${CMAKE_SOURCE_DIR}/vendor/freebsd-src/lib/libarchive/Makefile archive_makefile)
string(FIND "${archive_makefile}" "# Man pages" archive_sources_end)
if(archive_sources_end LESS 0)
    message(FATAL_ERROR "FreeBSD libarchive source list changed")
endif()
string(SUBSTRING "${archive_makefile}" 0 ${archive_sources_end} archive_makefile)
string(REGEX MATCHALL "[A-Za-z0-9_]+\\.c([\t\r\n \\]|$)" archive_sources "${archive_makefile}")
list(TRANSFORM archive_sources REPLACE "[\t\r\n \\]+$" "")
list(TRANSFORM archive_sources PREPEND ${CHIMERA_ARCHIVE_SOURCE}/libarchive/)
add_library(archive_chimera STATIC ${archive_sources})
set_target_properties(archive_chimera PROPERTIES OUTPUT_NAME archive)
target_include_directories(archive_chimera PUBLIC ${CHIMERA_ARCHIVE_SOURCE}/libarchive)
target_compile_definitions(archive_chimera PRIVATE PLATFORM_CONFIG_H="${CHIMERA_ARCHIVE_CONFIG}")
target_compile_options(archive_chimera PRIVATE -U__APPLE__ -Wno-deprecated-declarations)
target_link_libraries(archive_chimera PUBLIC z system_chimera)

set(archive_frontend ${CHIMERA_ARCHIVE_SOURCE}/libarchive_fe)
add_library(archive_frontend STATIC ${archive_frontend}/lafe_err.c
    ${archive_frontend}/line_reader.c ${archive_frontend}/passphrase.c)
target_include_directories(archive_frontend PUBLIC ${archive_frontend})
target_compile_definitions(archive_frontend PRIVATE PLATFORM_CONFIG_H="${CHIMERA_ARCHIVE_CONFIG}")
target_link_libraries(archive_frontend PUBLIC system_chimera)

set(tar_source ${CHIMERA_ARCHIVE_SOURCE}/tar)
add_executable(tar ${tar_source}/bsdtar.c ${tar_source}/cmdline.c ${tar_source}/creation_set.c
    ${tar_source}/read.c ${tar_source}/subst.c ${tar_source}/util.c ${tar_source}/write.c)
chimera_add_user_crt(tar)
target_compile_options(tar PRIVATE -U__APPLE__ -Wno-deprecated-declarations)
target_compile_definitions(tar PRIVATE PLATFORM_CONFIG_H="${CHIMERA_ARCHIVE_CONFIG}")
target_link_libraries(tar archive_chimera archive_frontend system_chimera)
