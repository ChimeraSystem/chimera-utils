set(FASTFETCH_DIR ${CMAKE_SOURCE_DIR}/vendor/fastfetch)
set(CHIMERA_FASTFETCH_BUILTINS ${CMAKE_CURRENT_BINARY_DIR}/fastfetch-generated/builtin.c)
add_custom_command(OUTPUT ${CHIMERA_FASTFETCH_BUILTINS}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/fastfetch_logo.py
        ${CHIMERA_ASSETS_DIR}/branding/chimera.txt ${FASTFETCH_DIR}/src/logo ${CHIMERA_FASTFETCH_BUILTINS}
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/fastfetch_logo.py ${CHIMERA_ASSETS_DIR}/branding/chimera.txt
        ${FASTFETCH_DIR}/src/logo/builtin.c ${FASTFETCH_DIR}/src/logo/ascii/c.inc
    VERBATIM)
add_custom_target(chimera_fastfetch_logo DEPENDS ${CHIMERA_FASTFETCH_BUILTINS})

file(GLOB FASTFETCH_MODULE_SRCS "${FASTFETCH_DIR}/src/modules/*/*.c")

set(FASTFETCH_SRCS
    ${FASTFETCH_DIR}/src/3rdparty/yyjson/yyjson.c
    ${FASTFETCH_DIR}/src/common/impl/commandoption.c
    ${FASTFETCH_DIR}/src/common/impl/duration.c
    ${FASTFETCH_DIR}/src/common/impl/font.c
    ${FASTFETCH_DIR}/src/common/impl/format.c
    ${FASTFETCH_DIR}/src/common/impl/frequency.c
    ${FASTFETCH_DIR}/src/common/impl/genconfig.c
    ${FASTFETCH_DIR}/src/common/impl/init.c
    ${FASTFETCH_DIR}/src/common/impl/jsonconfig.c
    ${FASTFETCH_DIR}/src/common/impl/netif.c
    ${FASTFETCH_DIR}/src/common/impl/netif_chimera.c
    ${FASTFETCH_DIR}/src/common/impl/networking_common.c
    ${FASTFETCH_DIR}/src/common/impl/networking_linux.c
    ${FASTFETCH_DIR}/src/common/impl/option.c
    ${FASTFETCH_DIR}/src/common/impl/parsing.c
    ${FASTFETCH_DIR}/src/common/impl/percent.c
    ${FASTFETCH_DIR}/src/common/impl/printing.c
    ${FASTFETCH_DIR}/src/common/impl/processing_linux.c
    ${FASTFETCH_DIR}/src/common/impl/properties.c
    ${FASTFETCH_DIR}/src/common/impl/settings.c
    ${FASTFETCH_DIR}/src/common/impl/size.c
    ${FASTFETCH_DIR}/src/common/impl/temps.c
    ${FASTFETCH_DIR}/src/common/impl/time.c
    ${FASTFETCH_DIR}/src/common/impl/edidHelper.c
    ${FASTFETCH_DIR}/src/common/impl/base64.c
    ${FASTFETCH_DIR}/src/common/impl/binary_nosupport.c
    ${FASTFETCH_DIR}/src/common/impl/FFlist.c
    ${FASTFETCH_DIR}/src/common/impl/FFstrbuf.c
    ${FASTFETCH_DIR}/src/common/impl/path.c
    ${FASTFETCH_DIR}/src/common/impl/FFPlatform.c
    ${FASTFETCH_DIR}/src/common/impl/FFPlatform_unix.c
    ${FASTFETCH_DIR}/src/common/impl/smbios.c
    ${FASTFETCH_DIR}/src/common/impl/strutil.c
    ${FASTFETCH_DIR}/src/common/impl/sysctl.c
    ${FASTFETCH_DIR}/src/common/impl/io_unix.c
    ${FASTFETCH_DIR}/src/common/impl/memrchr.c
    ${FASTFETCH_DIR}/src/common/impl/wcwidth.c
    ${FASTFETCH_DIR}/src/options/display.c
    ${FASTFETCH_DIR}/src/options/general.c
    ${FASTFETCH_DIR}/src/options/logo.c
    ${CHIMERA_FASTFETCH_BUILTINS}
    ${FASTFETCH_DIR}/src/logo/logo.c
    ${FASTFETCH_DIR}/src/logo/image/image_nosupport.c
    ${FASTFETCH_DIR}/src/modules/modules.c
    ${FASTFETCH_MODULE_SRCS}
    ${FASTFETCH_DIR}/src/detection/battery/battery_nosupport.c
    ${FASTFETCH_DIR}/src/detection/bios/bios_nosupport.c
    ${FASTFETCH_DIR}/src/detection/bluetooth/bluetooth_nosupport.c
    ${FASTFETCH_DIR}/src/detection/bluetoothradio/bluetoothradio_nosupport.c
    ${FASTFETCH_DIR}/src/detection/board/board_nosupport.c
    ${FASTFETCH_DIR}/src/detection/bootmgr/bootmgr_nosupport.c
    ${FASTFETCH_DIR}/src/detection/brightness/brightness_nosupport.c
    ${FASTFETCH_DIR}/src/detection/btrfs/btrfs_nosupport.c
    ${FASTFETCH_DIR}/src/detection/camera/camera_nosupport.c
    ${FASTFETCH_DIR}/src/detection/chassis/chassis_nosupport.c
    ${FASTFETCH_DIR}/src/detection/codec/codec.c
    ${FASTFETCH_DIR}/src/detection/codec/codec_nosupport.c
    ${FASTFETCH_DIR}/src/detection/command/command.c
    ${FASTFETCH_DIR}/src/detection/cpu/cpu.c
    ${FASTFETCH_DIR}/src/detection/cpu/cpu_x86.c
    ${FASTFETCH_DIR}/src/detection/cpu/cpu_chimera.c
    ${FASTFETCH_DIR}/src/detection/cpucache/cpucache_nosupport.c
    ${FASTFETCH_DIR}/src/detection/cpuusage/cpuusage.c
    ${FASTFETCH_DIR}/src/detection/cpuusage/cpuusage_nosupport.c
    ${FASTFETCH_DIR}/src/detection/cursor/cursor_nosupport.c
    ${FASTFETCH_DIR}/src/detection/de/de_nosupport.c
    ${FASTFETCH_DIR}/src/detection/disk/disk.c
    ${FASTFETCH_DIR}/src/detection/disk/disk_chimera.c
    ${FASTFETCH_DIR}/src/detection/diskio/diskio.c
    ${FASTFETCH_DIR}/src/detection/diskio/diskio_nosupport.c
    ${FASTFETCH_DIR}/src/detection/displayserver/displayserver.c
    ../coreutils/adapters/fastfetch_display.c
    ${FASTFETCH_DIR}/src/detection/dns/dns_linux.c
    ${FASTFETCH_DIR}/src/detection/editor/editor.c
    ${FASTFETCH_DIR}/src/detection/font/font.c
    ${FASTFETCH_DIR}/src/detection/font/font_nosupport.c
    ${FASTFETCH_DIR}/src/detection/gamepad/gamepad_nosupport.c
    ${FASTFETCH_DIR}/src/detection/gpu/gpu.c
    ../coreutils/adapters/fastfetch_gpu.c
    ${FASTFETCH_DIR}/src/detection/host/host_mac.c
    ${FASTFETCH_DIR}/src/detection/host/host_chimera.c
    ${FASTFETCH_DIR}/src/detection/icons/icons_nosupport.c
    ${FASTFETCH_DIR}/src/detection/initsystem/initsystem_nosupport.c
    ${FASTFETCH_DIR}/src/detection/keyboard/keyboard_nosupport.c
    ${FASTFETCH_DIR}/src/detection/libc/libc_apple.c
    ${FASTFETCH_DIR}/src/detection/lm/lm_nosupport.c
    ${FASTFETCH_DIR}/src/detection/loadavg/loadavg_nosupport.c
    ${FASTFETCH_DIR}/src/detection/locale/locale_linux.c
    ${FASTFETCH_DIR}/src/detection/localip/localip_chimera.c
    ${FASTFETCH_DIR}/src/detection/media/media.c
    ${FASTFETCH_DIR}/src/detection/media/media_nosupport.c
    ${FASTFETCH_DIR}/src/detection/memory/memory_chimera.c
    ${FASTFETCH_DIR}/src/detection/mouse/mouse_nosupport.c
    ${FASTFETCH_DIR}/src/detection/netio/netio.c
    ${FASTFETCH_DIR}/src/detection/netio/netio_nosupport.c
    ${FASTFETCH_DIR}/src/detection/opencl/opencl.c
    ${FASTFETCH_DIR}/src/detection/opengl/opengl_nosupport.c
    ${FASTFETCH_DIR}/src/detection/os/os.c
    ${FASTFETCH_DIR}/src/detection/os/os_chimera.c
    ${FASTFETCH_DIR}/src/detection/packages/packages.c
    ${FASTFETCH_DIR}/src/detection/packages/packages_chimera.c
    ${FASTFETCH_DIR}/src/detection/physicaldisk/physicaldisk_nosupport.c
    ${FASTFETCH_DIR}/src/detection/physicalmemory/physicalmemory_nosupport.c
    ${FASTFETCH_DIR}/src/detection/poweradapter/poweradapter_nosupport.c
    ${FASTFETCH_DIR}/src/detection/processes/processes_chimera.c
    ${FASTFETCH_DIR}/src/detection/publicip/publicip.c
    ${FASTFETCH_DIR}/src/detection/sound/sound_nosupport.c
    ${FASTFETCH_DIR}/src/detection/swap/swap_nosupport.c
    ${FASTFETCH_DIR}/src/detection/terminalfont/terminalfont.c
    ${FASTFETCH_DIR}/src/detection/terminalfont/terminalfont_chimera.c
    ${FASTFETCH_DIR}/src/detection/terminalshell/terminalshell_chimera.c
    ${FASTFETCH_DIR}/src/detection/terminalsize/terminalsize_linux.c
    ${FASTFETCH_DIR}/src/detection/terminaltheme/terminaltheme.c
    ${FASTFETCH_DIR}/src/detection/theme/theme_nosupport.c
    ${FASTFETCH_DIR}/src/detection/top/top.c
    ${FASTFETCH_DIR}/src/detection/top/top_nosupport.c
    ${FASTFETCH_DIR}/src/detection/tpm/tpm_nosupport.c
    ${FASTFETCH_DIR}/src/detection/uptime/uptime_chimera.c
    ${FASTFETCH_DIR}/src/detection/users/users_chimera.c
    ${FASTFETCH_DIR}/src/detection/version/version.c
    ${FASTFETCH_DIR}/src/detection/vulkan/vulkan.c
    ${FASTFETCH_DIR}/src/detection/wallpaper/wallpaper_nosupport.c
    ${FASTFETCH_DIR}/src/detection/weather/weather.c
    ${FASTFETCH_DIR}/src/detection/wifi/wifi_nosupport.c
    ${FASTFETCH_DIR}/src/detection/wm/wm_nosupport.c
    ${FASTFETCH_DIR}/src/detection/wmtheme/wmtheme_nosupport.c
    ${FASTFETCH_DIR}/src/detection/zpool/zpool.c
)

add_executable(fastfetch
    ${FASTFETCH_SRCS}
    ${FASTFETCH_DIR}/src/fastfetch.c
)
chimera_add_user_crt(fastfetch)
add_dependencies(fastfetch chimera_fastfetch_logo)
target_link_libraries(fastfetch system_chimera)
target_include_directories(fastfetch PRIVATE
    ${CMAKE_SOURCE_DIR}/base-system/libsystem/include
    ${FASTFETCH_DIR}/src
    ${FASTFETCH_DIR}/src/logo
    ${FASTFETCH_DIR}/src/3rdparty/yyjson
    ${FASTFETCH_DIR}/src/3rdparty/widecharwidth
)
target_compile_definitions(fastfetch PRIVATE
    -D__MACH__=1
    -D__CHIMERA__=1
    -D__APPLE__=1
    -D_GNU_SOURCE=1
    -D_DARWIN_C_SOURCE=1
    -DFF_DISABLE_DLOPEN=1
    -DFF_SUPPORTS_COUNT_OF=1
    -DFF_ENABLE_WCWIDTH=1
    -DFF_PACKAGES_DISABLE_LIST=0
    -DFASTFETCH_DATATEXT_JSON_HELP="{}"
)
target_compile_options(fastfetch PRIVATE -std=c23 -Wno-everything)

add_executable(flashfetch
    ${FASTFETCH_SRCS}
    ${FASTFETCH_DIR}/src/flashfetch.c
)
chimera_add_user_crt(flashfetch)
add_dependencies(flashfetch chimera_fastfetch_logo)
target_link_libraries(flashfetch system_chimera)
target_include_directories(flashfetch PRIVATE
    ${CMAKE_SOURCE_DIR}/base-system/libsystem/include
    ${FASTFETCH_DIR}/src
    ${FASTFETCH_DIR}/src/logo
    ${FASTFETCH_DIR}/src/3rdparty/yyjson
    ${FASTFETCH_DIR}/src/3rdparty/widecharwidth
)
target_compile_definitions(flashfetch PRIVATE
    -D__MACH__=1
    -D__CHIMERA__=1
    -D__APPLE__=1
    -D_GNU_SOURCE=1
    -D_DARWIN_C_SOURCE=1
    -DFF_DISABLE_DLOPEN=1
    -DFF_SUPPORTS_COUNT_OF=1
    -DFF_ENABLE_WCWIDTH=1
    -DFF_PACKAGES_DISABLE_LIST=0
    -DFASTFETCH_DATATEXT_JSON_HELP="{}"
)
target_compile_options(flashfetch PRIVATE -std=c23 -Wno-everything)
