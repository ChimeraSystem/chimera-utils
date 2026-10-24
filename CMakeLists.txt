add_executable(ls ../coreutils/src/ls/ls.c ../coreutils/src/ls/cmp.c ../coreutils/src/ls/print.c ../coreutils/src/ls/util.c)
target_include_directories(ls PRIVATE ../coreutils/src/ls)
add_executable(recovery_guest EXCLUDE_FROM_ALL ../tests/test_recovery_guest.c)
chimera_add_user_crt(recovery_guest)
target_link_libraries(recovery_guest system_chimera)

add_executable(fuse_guest EXCLUDE_FROM_ALL ../tests/test_fuse_boot.c)
chimera_add_user_crt(fuse_guest)
target_link_libraries(fuse_guest system_chimera)
if(CHIMERA_BUILD_SECURITY_TESTS)
    add_executable(telemetry_guest EXCLUDE_FROM_ALL ../tests/test_telemetry_guest.c)
    chimera_add_user_crt(telemetry_guest)
    target_link_libraries(telemetry_guest system_chimera)
    add_executable(gpu_handoff_guest EXCLUDE_FROM_ALL ../tests/test_gpu_handoff_guest.c)
    chimera_add_user_crt(gpu_handoff_guest)
    target_link_libraries(gpu_handoff_guest system_chimera)
    add_executable(nic_guest EXCLUDE_FROM_ALL ../tests/test_nic_guest.c)
    chimera_add_user_crt(nic_guest)
    target_link_libraries(nic_guest system_chimera)
endif()
chimera_add_user_crt(ls)
target_link_libraries(ls system_chimera)

add_executable(cat ../coreutils/src/cat.c)
chimera_add_user_crt(cat)
target_link_libraries(cat system_chimera)

add_executable(echo ../coreutils/src/echo.c)
chimera_add_user_crt(echo)
target_link_libraries(echo system_chimera)

add_executable(mkdir ../coreutils/src/mkdir.c)
chimera_add_user_crt(mkdir)
target_link_libraries(mkdir system_chimera)

add_executable(rmdir ../coreutils/src/rmdir.c)
chimera_add_user_crt(rmdir)
target_link_libraries(rmdir system_chimera)

add_executable(ln ../coreutils/src/ln.c)
chimera_add_user_crt(ln)
target_link_libraries(ln system_chimera)

add_executable(rm ../coreutils/src/rm.c)
chimera_add_user_crt(rm)
target_link_libraries(rm system_chimera)

add_executable(touch ../coreutils/src/touch.c)
chimera_add_user_crt(touch)
target_link_libraries(touch system_chimera)

add_executable(pwd ../coreutils/src/pwd.c)
chimera_add_user_crt(pwd)
target_link_libraries(pwd system_chimera)

add_executable(proclist ../coreutils/src/proclist.c)
chimera_add_user_crt(proclist)
target_link_libraries(proclist system_chimera)

add_executable(ifconfig ../coreutils/src/ifconfig.c)
chimera_add_user_crt(ifconfig)
target_link_libraries(ifconfig system_chimera)

add_executable(ping ../coreutils/src/ping.c)
chimera_add_user_crt(ping)
target_link_libraries(ping system_chimera)

add_executable(nc ../coreutils/src/nc.c)
chimera_add_user_crt(nc)
target_link_libraries(nc system_chimera)

add_executable(srv ../coreutils/src/srv.c)
chimera_add_user_crt(srv)
target_link_libraries(srv system_chimera)
