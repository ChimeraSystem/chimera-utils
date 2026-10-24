# 4. FreeBSD / ravynOS BSD sh Port
set(SH_SRC_DIR ${CMAKE_SOURCE_DIR}/vendor/sh)
set(SH_SOURCES
    ${SH_SRC_DIR}/bltin/echo.c
    ${SH_SRC_DIR}/alias.c
    ${SH_SRC_DIR}/arith_yacc.c
    ${SH_SRC_DIR}/arith_yylex.c
    ${SH_SRC_DIR}/cd.c
    ${SH_SRC_DIR}/error.c
    ${SH_SRC_DIR}/eval.c
    ${SH_SRC_DIR}/exec.c
    ${SH_SRC_DIR}/expand.c
    ${SH_SRC_DIR}/histedit.c
    ${SH_SRC_DIR}/input.c
    ${SH_SRC_DIR}/jobs.c
    ${SH_SRC_DIR}/mail.c
    ${SH_SRC_DIR}/main.c
    ${SH_SRC_DIR}/memalloc.c
    ${SH_SRC_DIR}/miscbltin.c
    ${SH_SRC_DIR}/mystring.c
    ${SH_SRC_DIR}/options.c
    ${SH_SRC_DIR}/output.c
    ${SH_SRC_DIR}/parser.c
    ${SH_SRC_DIR}/redir.c
    ${SH_SRC_DIR}/show.c
    ${SH_SRC_DIR}/trap.c
    ${SH_SRC_DIR}/var.c
    ${SH_SRC_DIR}/builtins.c
    ${SH_SRC_DIR}/nodes.c
    ${SH_SRC_DIR}/syntax.c
    ${SH_SRC_DIR}/shims.c
)

add_executable(sh ${SH_SOURCES})
chimera_add_user_crt(sh)
target_link_libraries(sh system_chimera)
target_include_directories(sh PRIVATE ${SH_SRC_DIR} ${CMAKE_SOURCE_DIR}/base-system/libsystem/include)
target_compile_definitions(sh PRIVATE -DHAVE_CONFIG_H -DBSD=1 -D_GNU_SOURCE -DUSE_TEE=0 -DHAVE_KILLPG=1 -DSHELL -DHAVE_BSEARCH=1 -DHAVE_STRTOD=1 -DNO_HISTORY=1)
target_compile_options(sh PRIVATE -Wno-int-conversion -Wno-implicit-function-declaration -Wno-unused-variable -Wno-nullability-completeness -Wno-everything)

# 5. Zsh Shell Port (ravynOS / Darwin)
set(ZSH_SRC_DIR ${CMAKE_SOURCE_DIR}/vendor/zsh/Src)
set(ZSH_SOURCES
    ${ZSH_SRC_DIR}/main.c
    ${ZSH_SRC_DIR}/init.c
    ${ZSH_SRC_DIR}/builtin.c
    ${ZSH_SRC_DIR}/compat.c
    ${ZSH_SRC_DIR}/cond.c
    ${ZSH_SRC_DIR}/context.c
    ${ZSH_SRC_DIR}/exec.c
    ${ZSH_SRC_DIR}/glob.c
    ${ZSH_SRC_DIR}/hashnameddir.c
    ${ZSH_SRC_DIR}/hashtable.c
    ${ZSH_SRC_DIR}/hist.c
    ${ZSH_SRC_DIR}/input.c
    ${ZSH_SRC_DIR}/jobs.c
    ${ZSH_SRC_DIR}/lex.c
    ${ZSH_SRC_DIR}/linklist.c
    ${ZSH_SRC_DIR}/loop.c
    ${ZSH_SRC_DIR}/math.c
    ${ZSH_SRC_DIR}/mem.c
    ${ZSH_SRC_DIR}/module.c
    ${ZSH_SRC_DIR}/options.c
    ${ZSH_SRC_DIR}/params.c
    ${ZSH_SRC_DIR}/parse.c
    ${ZSH_SRC_DIR}/pattern.c
    ${ZSH_SRC_DIR}/prompt.c
    ${ZSH_SRC_DIR}/signals.c
    ${ZSH_SRC_DIR}/signames.c
    ${ZSH_SRC_DIR}/sort.c
    ${ZSH_SRC_DIR}/string.c
    ${ZSH_SRC_DIR}/subst.c
    ${ZSH_SRC_DIR}/text.c
    ${ZSH_SRC_DIR}/utils.c
    ${ZSH_SRC_DIR}/Modules/parameter.c
    ${ZSH_SRC_DIR}/Modules/termcap.c
    ${ZSH_SRC_DIR}/Modules/zutil.c
    ${ZSH_SRC_DIR}/Zle/zle_bindings.c
    ${ZSH_SRC_DIR}/Zle/zle_hist.c
    ${ZSH_SRC_DIR}/Zle/zle_keymap.c
    ${ZSH_SRC_DIR}/Zle/zle_main.c
    ${ZSH_SRC_DIR}/Zle/zle_misc.c
    ${ZSH_SRC_DIR}/Zle/zle_move.c
    ${ZSH_SRC_DIR}/Zle/zle_params.c
    ${ZSH_SRC_DIR}/Zle/zle_refresh.c
    ${ZSH_SRC_DIR}/Zle/zle_thingy.c
    ${ZSH_SRC_DIR}/Zle/zle_tricky.c
    ${ZSH_SRC_DIR}/Zle/zle_utils.c
    ${ZSH_SRC_DIR}/Zle/zle_vi.c
    ${ZSH_SRC_DIR}/Zle/zle_word.c
    ${ZSH_SRC_DIR}/Zle/textobjects.c
    ${ZSH_SRC_DIR}/Zle/compcore.c
    ${ZSH_SRC_DIR}/Zle/compctl.c
    ${ZSH_SRC_DIR}/Zle/complete.c
    ${ZSH_SRC_DIR}/Zle/complist.c
    ${ZSH_SRC_DIR}/Zle/compmatch.c
    ${ZSH_SRC_DIR}/Zle/compresult.c
    ${ZSH_SRC_DIR}/Zle/computil.c
    ${ZSH_SRC_DIR}/Zle/zleparameter.c
)

add_executable(zsh ${ZSH_SOURCES})
chimera_add_user_crt(zsh)
target_link_libraries(zsh system_chimera)
target_include_directories(zsh PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/zsh
    ${ZSH_SRC_DIR}
    ${ZSH_SRC_DIR}/Builtins
    ${ZSH_SRC_DIR}/Zle
    ${ZSH_SRC_DIR}/Modules
    ${CMAKE_SOURCE_DIR}/base-system/libsystem/include
)
target_compile_definitions(zsh PRIVATE
    -DHAVE_CONFIG_H=1
    -DZSH_STATIC=1
    -D__MACH__=1
    -D__CHIMERA__=1
    -D__APPLE__=1
)
target_compile_options(zsh PRIVATE
    -Wno-everything
)

# 6. stty Terminal Utility (ravynOS / BSD)
set(STTY_SRC_DIR ${CMAKE_SOURCE_DIR}/coreutils/src/stty)
add_executable(stty
    ${STTY_SRC_DIR}/stty.c
    ${STTY_SRC_DIR}/cchar.c
    ${STTY_SRC_DIR}/gfmt.c
    ${STTY_SRC_DIR}/key.c
    ${STTY_SRC_DIR}/modes.c
    ${STTY_SRC_DIR}/print.c
    ${STTY_SRC_DIR}/util.c
)
chimera_add_user_crt(stty)
target_link_libraries(stty system_chimera)
target_include_directories(stty PRIVATE ${STTY_SRC_DIR} ${CMAKE_SOURCE_DIR}/base-system/libsystem/include)
target_compile_definitions(stty PRIVATE -D__CHIMERA__=1)
target_compile_options(stty PRIVATE -Wno-everything)


# 5. Tiny C Compiler (tcc) Port
set(TCC_SRC_DIR ${CMAKE_SOURCE_DIR}/vendor/tinycc)
add_executable(tcc EXCLUDE_FROM_ALL ${TCC_SRC_DIR}/tcc.c)
chimera_add_user_crt(tcc)
target_link_libraries(tcc system_chimera)
target_include_directories(tcc PRIVATE
    ${TCC_SRC_DIR}
    ${CMAKE_SOURCE_DIR}/base-system/libsystem/include
    ${CMAKE_SOURCE_DIR}/base-system/libsystem
)
target_compile_definitions(tcc PRIVATE
    -DTCC_TARGET_X86_64=1
    -DTCC_TARGET_MACHO=1
    -DONE_SOURCE=1
    -DCONFIG_TCCDIR=\"/lib/tcc\"
    -DCONFIG_SYSROOT=\"\"
    -DCONFIG_TCC_CRTPREFIX=\"/lib\"
    -DCONFIG_TCC_LIBPATHS=\"/lib:/usr/lib\"
    -DTCC_VERSION=\"0.9.27\"
    -DTCC_LIBTCC1=\"\"
)
