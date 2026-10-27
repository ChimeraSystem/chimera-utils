set(ufs_port ${CMAKE_SOURCE_DIR}/base-system/ports/ufs)
set(ufs_source ${CMAKE_SOURCE_DIR}/vendor/freebsd-src)
add_library(ufs_readonly STATIC ${ufs_port}/reader.c
    ${ufs_source}/sys/ufs/ffs/ffs_subr.c
    ${ufs_source}/sys/ufs/ffs/ffs_tables.c
    ${ufs_source}/sys/libkern/gsb_crc32.c)
target_include_directories(ufs_readonly PUBLIC ${ufs_port} ${CMAKE_SOURCE_DIR}/tools/freebsd/include
    ${CMAKE_SOURCE_DIR}/kernel/include/uapi)
target_compile_options(ufs_readonly PRIVATE -include ${ufs_port}/compat.h)
set_source_files_properties(${ufs_source}/sys/libkern/gsb_crc32.c PROPERTIES
    COMPILE_DEFINITIONS calculate_crc32c=ufs_crc32c32)
target_link_libraries(ufs_readonly PUBLIC system_chimera)
add_executable(dumpfs ${ufs_source}/sbin/dumpfs/dumpfs.c)
chimera_add_user_crt(dumpfs)
target_compile_options(dumpfs PRIVATE -include ${ufs_port}/compat.h)
target_link_libraries(dumpfs ufs_readonly)
add_executable(ufscheck ${ufs_port}/check.c ${ufs_port}/tree.c ${ufs_port}/recovery.c ${ufs_port}/recover_copy.c ${ufs_port}/counts.c ${ufs_port}/orphans.c)
chimera_add_user_crt(ufscheck)
target_compile_options(ufscheck PRIVATE -include ${ufs_port}/compat.h)
target_link_libraries(ufscheck ufs_readonly)
