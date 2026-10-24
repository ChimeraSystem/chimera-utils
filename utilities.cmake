add_executable(gl_gears ../coreutils/src/gl_gears.c)
chimera_add_user_crt(gl_gears)
target_link_libraries(gl_gears GL drm system_chimera)

add_executable(defaults ../coreutils/src/defaults.c)
chimera_add_user_crt(defaults)
target_link_libraries(defaults system_chimera)

add_executable(fuse ../coreutils/src/fuse.c)
chimera_add_user_crt(fuse)
target_link_libraries(fuse system_chimera)

add_executable(fuse-fetch ../coreutils/src/fuse_fetch.c)
chimera_add_user_crt(fuse-fetch)
target_link_libraries(fuse-fetch libcurl ${CMAKE_SOURCE_DIR}/vendor/openssl/libssl.a
    ${CMAKE_SOURCE_DIR}/vendor/openssl/libcrypto.a z system_chimera)

add_executable(cp ../coreutils/src/cp/cp.c ../coreutils/src/cp/utils.c)
target_include_directories(cp PRIVATE ../coreutils/src/cp)
chimera_add_user_crt(cp)
target_link_libraries(cp system_chimera)

add_executable(mv ../coreutils/src/mv.c)
chimera_add_user_crt(mv)
target_link_libraries(mv system_chimera)

add_executable(head ../coreutils/src/head.c)
chimera_add_user_crt(head)
target_link_libraries(head system_chimera)

add_executable(tail ../coreutils/src/tail/tail.c ../coreutils/src/tail/forward.c ../coreutils/src/tail/misc.c ../coreutils/src/tail/read.c ../coreutils/src/tail/reverse.c)
target_include_directories(tail PRIVATE ../coreutils/src/tail)
chimera_add_user_crt(tail)
target_link_libraries(tail system_chimera)

add_executable(grep ../coreutils/src/grep.c)
chimera_add_user_crt(grep)
target_link_libraries(grep system_chimera)

add_executable(wc ../coreutils/src/wc.c)
chimera_add_user_crt(wc)
target_link_libraries(wc system_chimera)

add_executable(chmod ../coreutils/src/chmod.c)
chimera_add_user_crt(chmod)
target_link_libraries(chmod system_chimera)

add_executable(uname ../coreutils/src/uname.c)
chimera_add_user_crt(uname)
target_link_libraries(uname system_chimera)

add_executable(date ../coreutils/src/date/date.c ../coreutils/src/date/vary.c)
target_include_directories(date PRIVATE ../coreutils/src/date)
chimera_add_user_crt(date)
target_link_libraries(date system_chimera)

add_executable(env ../coreutils/src/env/env.c ../coreutils/src/env/envopts.c)
target_include_directories(env PRIVATE ../coreutils/src/env)
chimera_add_user_crt(env)
target_link_libraries(env system_chimera)

add_executable(whoami ../coreutils/src/id.c)
chimera_add_user_crt(whoami)
target_link_libraries(whoami system_chimera)

add_executable(login commands/login.c)
chimera_add_user_crt(login)
target_link_libraries(login system_chimera crypt)

add_executable(passwd commands/passwd_cmd.c)
chimera_add_user_crt(passwd)
target_link_libraries(passwd system_chimera crypt)

add_executable(su commands/su.c)
chimera_add_user_crt(su)
target_link_libraries(su system_chimera crypt)

add_executable(id ../coreutils/src/id.c)
chimera_add_user_crt(id)
target_link_libraries(id system_chimera)

add_executable(sleep ../coreutils/src/sleep.c)
chimera_add_user_crt(sleep)
target_link_libraries(sleep system_chimera)

add_executable(true ../coreutils/src/true.c)
chimera_add_user_crt(true)
target_link_libraries(true system_chimera)

add_executable(false ../coreutils/src/false.c)
chimera_add_user_crt(false)
target_link_libraries(false system_chimera)

add_executable(yes ../coreutils/src/yes.c)
chimera_add_user_crt(yes)
target_link_libraries(yes system_chimera)

add_executable(clear ../coreutils/src/clear.c)
chimera_add_user_crt(clear)
target_link_libraries(clear system_chimera)

add_executable(kill ../coreutils/src/kill.c)
chimera_add_user_crt(kill)
target_link_libraries(kill system_chimera)

add_executable(uptime ../coreutils/src/uptime.c)
chimera_add_user_crt(uptime)
target_link_libraries(uptime system_chimera)

add_executable(stat ../coreutils/src/stat.c)
chimera_add_user_crt(stat)
target_link_libraries(stat system_chimera)

add_executable(df ../coreutils/src/df.c)
chimera_add_user_crt(df)
target_link_libraries(df system_chimera)

add_executable(dmesg ../coreutils/src/dmesg.c)
chimera_add_user_crt(dmesg)
target_link_libraries(dmesg system_chimera)

add_executable(reboot commands/reboot.c)
chimera_add_user_crt(reboot)
target_link_libraries(reboot system_chimera)

add_executable(sysctl ../coreutils/src/sysctl.c)
chimera_add_user_crt(sysctl)
target_link_libraries(sysctl system_chimera)

add_executable(lsblk ../coreutils/src/lsblk.c)
chimera_add_user_crt(lsblk)
target_link_libraries(lsblk system_chimera)

add_executable(hexdump
    ../coreutils/src/hexdump/conv.c
    ../coreutils/src/hexdump/display.c
    ../coreutils/src/hexdump/hexdump.c
    ../coreutils/src/hexdump/hexsyntax.c
    ../coreutils/src/hexdump/odsyntax.c
    ../coreutils/src/hexdump/parse.c
)
target_include_directories(hexdump PRIVATE ../coreutils/src/hexdump)
chimera_add_user_crt(hexdump)
target_link_libraries(hexdump system_chimera)

add_executable(tee ../coreutils/src/tee.c)
chimera_add_user_crt(tee)
target_link_libraries(tee system_chimera)

add_executable(seq ../coreutils/src/seq.c)
chimera_add_user_crt(seq)
target_link_libraries(seq system_chimera)

add_executable(basename ../coreutils/src/basename.c)
chimera_add_user_crt(basename)
target_link_libraries(basename system_chimera)

add_executable(dirname ../coreutils/src/dirname.c)
chimera_add_user_crt(dirname)
target_link_libraries(dirname system_chimera)

add_executable(test ../coreutils/src/test.c)
chimera_add_user_crt(test)
target_link_libraries(test system_chimera)

add_executable(cut ../coreutils/src/cut.c)
chimera_add_user_crt(cut)
target_link_libraries(cut system_chimera)

add_executable(paste ../coreutils/src/paste.c)
chimera_add_user_crt(paste)
target_link_libraries(paste system_chimera)

add_executable(uniq ../coreutils/src/uniq.c)
chimera_add_user_crt(uniq)
target_link_libraries(uniq system_chimera)

add_executable(du ../coreutils/src/du.c)
chimera_add_user_crt(du)
target_link_libraries(du system_chimera)

add_executable(cmp ../coreutils/src/cmp/cmp.c ../coreutils/src/cmp/link.c ../coreutils/src/cmp/misc.c ../coreutils/src/cmp/regular.c ../coreutils/src/cmp/special.c)
target_include_directories(cmp PRIVATE ../coreutils/src/cmp)
chimera_add_user_crt(cmp)
target_link_libraries(cmp system_chimera)

add_executable(tr ../coreutils/src/tr/tr.c ../coreutils/src/tr/str.c ../coreutils/src/tr/cmap.c ../coreutils/src/tr/cset.c)
target_include_directories(tr PRIVATE ../coreutils/src/tr)
chimera_add_user_crt(tr)
target_link_libraries(tr system_chimera)

add_executable(xargs ../coreutils/src/xargs/xargs.c ../coreutils/src/xargs/strnsubst.c)
target_include_directories(xargs PRIVATE ../coreutils/src/xargs)
chimera_add_user_crt(xargs)
target_link_libraries(xargs system_chimera)

add_executable(dd ../coreutils/src/dd/args.c ../coreutils/src/dd/conv.c ../coreutils/src/dd/conv_tab.c ../coreutils/src/dd/dd.c ../coreutils/src/dd/misc.c ../coreutils/src/dd/position.c)
target_include_directories(dd PRIVATE ../coreutils/src/dd)
chimera_add_user_crt(dd)
target_link_libraries(dd system_chimera)

add_executable(sed ../coreutils/src/sed/main.c ../coreutils/src/sed/compile.c ../coreutils/src/sed/process.c ../coreutils/src/sed/misc.c)
target_include_directories(sed PRIVATE ../coreutils/src/sed)
chimera_add_user_crt(sed)
target_link_libraries(sed system_chimera)

add_executable(diff ../coreutils/src/diff/diff.c ../coreutils/src/diff/diffdir.c ../coreutils/src/diff/diffreg.c ../coreutils/src/diff/diffreg_new.c ../coreutils/src/diff/pr.c ../coreutils/src/diff/xmalloc.c)
target_include_directories(diff PRIVATE ../coreutils/src/diff)
chimera_add_user_crt(diff)
target_link_libraries(diff system_chimera)

add_executable(sort ../coreutils/src/sort/bwstring.c ../coreutils/src/sort/coll.c ../coreutils/src/sort/file.c ../coreutils/src/sort/mem.c ../coreutils/src/sort/radixsort.c ../coreutils/src/sort/sort.c ../coreutils/src/sort/vsort.c)
target_include_directories(sort PRIVATE ../coreutils/src/sort)
chimera_add_user_crt(sort)
target_link_libraries(sort system_chimera)

add_executable(find ../coreutils/src/find/find.c ../coreutils/src/find/function.c ../coreutils/src/find/ls.c ../coreutils/src/find/main.c ../coreutils/src/find/misc.c ../coreutils/src/find/operator.c ../coreutils/src/find/option.c ../coreutils/src/find/printf.c ../coreutils/src/find/getdate.c)
target_include_directories(find PRIVATE ../coreutils/src/find)
chimera_add_user_crt(find)
target_link_libraries(find system_chimera)

add_executable(kilo ${CMAKE_SOURCE_DIR}/vendor/kilo/kilo.c)
chimera_add_user_crt(kilo)
target_link_libraries(kilo system_chimera)

add_executable(nano ../coreutils/src/nano.c)
chimera_add_user_crt(nano)
target_link_libraries(nano system_chimera)

add_executable(memload ../coreutils/src/memload.c)
chimera_add_user_crt(memload)
target_link_libraries(memload system_chimera)

add_executable(locale ../coreutils/src/locale.c)
chimera_add_user_crt(locale)
target_link_libraries(locale system_chimera)
