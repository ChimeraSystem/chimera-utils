set(GIT_DIR ${CMAKE_SOURCE_DIR}/vendor/git)
include(${CMAKE_SOURCE_DIR}/base-system/git_sources.cmake)

add_library(libgit STATIC ${GIT_LIB_SOURCES})
target_include_directories(libgit BEFORE PRIVATE
    ${GIT_DIR}
    ${GIT_DIR}/compat
    ${CMAKE_SOURCE_DIR}/vendor/zlib
)
target_include_directories(libgit PUBLIC
    ${GIT_DIR}
    ${GIT_DIR}/compat
    ${CMAKE_SOURCE_DIR}/vendor/zlib
)
target_compile_definitions(libgit PRIVATE
    -DGIT_HOST_CPU="x86_64"
    -DSHA256_BLK
    -DINTERNAL_QSORT
    -DRUNTIME_PREFIX
    -DNO_OPENSSL
    -DSHA1_DC
    -DSHA1DC_NO_STANDARD_INCLUDES
    -DSHA1DC_INIT_SAFE_HASH_DEFAULT=0
    -DSHA1DC_CUSTOM_INCLUDE_SHA1_C="git-compat-util.h"
    -DSHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C="git-compat-util.h"
    -DPAGER_ENV="LESS=FRX"
    -DGIT_EXEC_PATH="/usr/libexec/git-core"
    -DGIT_HTML_PATH="/usr/share/doc/git-doc"
    -DGIT_MAN_PATH="/usr/share/man"
    -DGIT_INFO_PATH="/usr/share/info"
    -DDEFAULT_HELP_FORMAT="man"
    -DDEFAULT_GIT_TEMPLATE_DIR="/usr/share/git-core/templates"
    -DBINDIR="bin"
    -DFALLBACK_RUNTIME_PREFIX=""
    -DETC_GITATTRIBUTES="/etc/gitattributes"
    -DETC_GITCONFIG="/etc/gitconfig"
    -DNO_GETTEXT
    -DNO_ICONV
    -DHAVE_PATHS_H
    -DHAVE_STRINGS_H
    -DHAVE_ALLOCA_H
    -DHAVE_BSD_SYSCTL
    -DHAVE_GETDELIM
    -DHAVE_CLOCK_GETTIME
    -DHAVE_CLOCK_MONOTONIC
    -DHAVE_STRTOUMAX
    -DHAVE_MEMMEM
    -DNO_CURL
    -DGIT_VERSION="2.48.1"
    -D__CHIMERA__=1
    -D__APPLE__=1
)
target_compile_options(libgit PRIVATE -Wno-everything)

add_executable(git
    ${GIT_DIR}/common-main.c
    ${GIT_DIR}/git.c
)
chimera_add_user_crt(git)
target_link_libraries(git libgit z system_chimera)
target_include_directories(git PRIVATE
    ${GIT_DIR}
    ${GIT_DIR}/compat
    ${CMAKE_SOURCE_DIR}/vendor/zlib
    ${CMAKE_SOURCE_DIR}/base-system/libsystem/include
)
target_compile_definitions(git PRIVATE
    -DGIT_HOST_CPU="x86_64"
    -DSHA256_BLK
    -DINTERNAL_QSORT
    -DRUNTIME_PREFIX
    -DNO_OPENSSL
    -DSHA1_DC
    -DSHA1DC_NO_STANDARD_INCLUDES
    -DSHA1DC_INIT_SAFE_HASH_DEFAULT=0
    -DSHA1DC_CUSTOM_INCLUDE_SHA1_C="git-compat-util.h"
    -DSHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C="git-compat-util.h"
    -DPAGER_ENV="LESS=FRX"
    -DGIT_EXEC_PATH="/usr/libexec/git-core"
    -DGIT_HTML_PATH="/usr/share/doc/git-doc"
    -DGIT_MAN_PATH="/usr/share/man"
    -DGIT_INFO_PATH="/usr/share/info"
    -DDEFAULT_HELP_FORMAT="man"
    -DDEFAULT_GIT_TEMPLATE_DIR="/usr/share/git-core/templates"
    -DBINDIR="bin"
    -DFALLBACK_RUNTIME_PREFIX=""
    -DETC_GITATTRIBUTES="/etc/gitattributes"
    -DETC_GITCONFIG="/etc/gitconfig"
    -DNO_GETTEXT
    -DNO_ICONV
    -DHAVE_PATHS_H
    -DHAVE_STRINGS_H
    -DHAVE_ALLOCA_H
    -DHAVE_BSD_SYSCTL
    -DHAVE_GETDELIM
    -DHAVE_CLOCK_GETTIME
    -DHAVE_CLOCK_MONOTONIC
    -DHAVE_STRTOUMAX
    -DHAVE_MEMMEM
    -DNO_CURL
    -DGIT_VERSION="2.48.1"
    -D__CHIMERA__=1
    -D__APPLE__=1
)
target_compile_options(git PRIVATE -Wno-everything)

# =============================================================================
# libcurl (vendor)
# =============================================================================
set(CURL_DIR ${CMAKE_SOURCE_DIR}/vendor/curl)
include(${CMAKE_SOURCE_DIR}/base-system/curl_sources.cmake)

add_library(libcurl STATIC ${CURL_SOURCES})
target_include_directories(libcurl BEFORE PRIVATE
    ${CURL_DIR}/lib
    ${CURL_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor/openssl/include
    ${CMAKE_SOURCE_DIR}/vendor/zlib
)
target_include_directories(libcurl PUBLIC
    ${CURL_DIR}/include
)
target_compile_definitions(libcurl PRIVATE
    -DHAVE_CONFIG_H
    -DBUILDING_LIBCURL
    -DCURL_STATICLIB
    -D__CHIMERA__=1
    -D__APPLE__=1
)
target_compile_options(libcurl PRIVATE -Wno-everything)

# =============================================================================
# git-remote-http (Git HTTP / HTTPS transport helper)
# =============================================================================
add_executable(git-remote-http
    ${GIT_DIR}/common-main.c
    ${GIT_DIR}/remote-curl.c
    ${GIT_DIR}/http.c
    ${GIT_DIR}/http-walker.c
)
chimera_add_user_crt(git-remote-http)
target_link_libraries(git-remote-http libgit libcurl ${CMAKE_SOURCE_DIR}/vendor/openssl/libssl.a ${CMAKE_SOURCE_DIR}/vendor/openssl/libcrypto.a z expat system_chimera)
target_include_directories(git-remote-http BEFORE PRIVATE
    ${GIT_DIR}
    ${GIT_DIR}/compat
    ${CURL_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor/openssl/include
    ${CMAKE_SOURCE_DIR}/vendor/zlib
    ${CMAKE_SOURCE_DIR}/vendor/libexpat/expat/lib
)
target_compile_definitions(git-remote-http PRIVATE
    -DGIT_HOST_CPU="x86_64"
    -DSHA256_BLK
    -DINTERNAL_QSORT
    -DRUNTIME_PREFIX
    -DNO_OPENSSL
    -DSHA1_DC
    -DSHA1DC_NO_STANDARD_INCLUDES
    -DSHA1DC_INIT_SAFE_HASH_DEFAULT=0
    -DSHA1DC_CUSTOM_INCLUDE_SHA1_C="git-compat-util.h"
    -DSHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C="git-compat-util.h"
    -DPAGER_ENV="LESS=FRX"
    -DGIT_EXEC_PATH="/usr/libexec/git-core"
    -DGIT_HTML_PATH="/usr/share/doc/git-doc"
    -DGIT_MAN_PATH="/usr/share/man"
    -DGIT_INFO_PATH="/usr/share/info"
    -DDEFAULT_HELP_FORMAT="man"
    -DDEFAULT_GIT_TEMPLATE_DIR="/usr/share/git-core/templates"
    -DBINDIR="bin"
    -DFALLBACK_RUNTIME_PREFIX=""
    -DETC_GITATTRIBUTES="/etc/gitattributes"
    -DETC_GITCONFIG="/etc/gitconfig"
    -DNO_GETTEXT
    -DNO_ICONV
    -DHAVE_PATHS_H
    -DHAVE_STRINGS_H
    -DHAVE_ALLOCA_H
    -DHAVE_BSD_SYSCTL
    -DHAVE_GETDELIM
    -DHAVE_CLOCK_GETTIME
    -DHAVE_CLOCK_MONOTONIC
    -DHAVE_STRTOUMAX
    -DHAVE_MEMMEM
    -DGIT_VERSION="2.48.1"
    -D__CHIMERA__=1
    -D__APPLE__=1
)
target_compile_options(git-remote-http PRIVATE -Wno-everything)
