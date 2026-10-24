add_executable(machdemo ../coreutils/src/mach_demo.c)
chimera_add_user_crt(machdemo)
target_link_libraries(machdemo system_chimera)

add_executable(sectest ../coreutils/src/sectest.c)
chimera_add_user_crt(sectest)
target_link_libraries(sectest system_chimera)

add_executable(smpdemo ../coreutils/src/smp_demo.c)
chimera_add_user_crt(smpdemo)
target_link_libraries(smpdemo system_chimera)

add_executable(freezemon ../coreutils/src/freezemon.c)
chimera_add_user_crt(freezemon)
target_link_libraries(freezemon system_chimera)

add_executable(plutil ../coreutils/src/plutil.c)
chimera_add_user_crt(plutil)
target_link_libraries(plutil system_chimera)
