include(CMakePushCheckState)
include(CheckCXXCompilerFlag)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(TestBigEndian)

function(check_linker_flag flag out_var)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${flag}")
  check_cxx_compiler_flag("" ${out_var})
  cmake_pop_check_state()
endfunction()

# CodeGen options.
check_cxx_compiler_flag(-fPIC                COMPILER_RT_HAS_FPIC_FLAG)
check_cxx_compiler_flag(-fPIE                COMPILER_RT_HAS_FPIE_FLAG)
check_cxx_compiler_flag(-fno-builtin         COMPILER_RT_HAS_FNO_BUILTIN_FLAG)
check_cxx_compiler_flag(-fno-exceptions      COMPILER_RT_HAS_FNO_EXCEPTIONS_FLAG)
check_cxx_compiler_flag(-fomit-frame-pointer COMPILER_RT_HAS_FOMIT_FRAME_POINTER_FLAG)
check_cxx_compiler_flag(-funwind-tables      COMPILER_RT_HAS_FUNWIND_TABLES_FLAG)
check_cxx_compiler_flag(-fno-stack-protector COMPILER_RT_HAS_FNO_STACK_PROTECTOR_FLAG)
check_cxx_compiler_flag(-fno-sanitize=safe-stack COMPILER_RT_HAS_FNO_SANITIZE_SAFE_STACK_FLAG)
check_cxx_compiler_flag(-fvisibility=hidden  COMPILER_RT_HAS_FVISIBILITY_HIDDEN_FLAG)
check_cxx_compiler_flag(-frtti               COMPILER_RT_HAS_FRTTI_FLAG)
check_cxx_compiler_flag(-fno-rtti            COMPILER_RT_HAS_FNO_RTTI_FLAG)
check_cxx_compiler_flag(-ffreestanding       COMPILER_RT_HAS_FFREESTANDING_FLAG)
check_cxx_compiler_flag(" -fno-function-sections" COMPILER_RT_HAS_FNO_FUNCTION_SECTIONS_FLAG)
check_cxx_compiler_flag(-std=c++11           COMPILER_RT_HAS_STD_CXX11_FLAG)
check_cxx_compiler_flag(-ftls-model=initial-exec COMPILER_RT_HAS_FTLS_MODEL_INITIAL_EXEC)
check_cxx_compiler_flag(-fno-lto             COMPILER_RT_HAS_FNO_LTO_FLAG)
check_cxx_compiler_flag(" -msse3" COMPILER_RT_HAS_MSSE3_FLAG)
check_cxx_compiler_flag(--sysroot=.          COMPILER_RT_HAS_SYSROOT_FLAG)

if(NOT WIN32 AND NOT CYGWIN)
  # MinGW warns if -fvisibility-inlines-hidden is used.
  check_cxx_compiler_flag("-fvisibility-inlines-hidden" COMPILER_RT_HAS_FVISIBILITY_INLINES_HIDDEN_FLAG)
endif()

check_cxx_compiler_flag(/GR COMPILER_RT_HAS_GR_FLAG)
check_cxx_compiler_flag(/GS COMPILER_RT_HAS_GS_FLAG)
check_cxx_compiler_flag(/MT COMPILER_RT_HAS_MT_FLAG)
check_cxx_compiler_flag(/Oy COMPILER_RT_HAS_Oy_FLAG)

# Debug info flags.
check_cxx_compiler_flag(-gline-tables-only COMPILER_RT_HAS_GLINE_TABLES_ONLY_FLAG)
check_cxx_compiler_flag(-g COMPILER_RT_HAS_G_FLAG)
check_cxx_compiler_flag(/Zi COMPILER_RT_HAS_Zi_FLAG)

# Warnings.
check_cxx_compiler_flag(-Wall COMPILER_RT_HAS_WALL_FLAG)
check_cxx_compiler_flag( COMPILER_RT_HAS_WERROR_FLAG)
check_cxx_compiler_flag(" -Wframe-larger-than=512" COMPILER_RT_HAS_WFRAME_LARGER_THAN_FLAG)
check_cxx_compiler_flag(" -Wglobal-constructors"   COMPILER_RT_HAS_WGLOBAL_CONSTRUCTORS_FLAG)
check_cxx_compiler_flag(" -Wc99-extensions"     COMPILER_RT_HAS_WC99_EXTENSIONS_FLAG)
check_cxx_compiler_flag(" -Wgnu"                COMPILER_RT_HAS_WGNU_FLAG)
check_cxx_compiler_flag(" -Wnon-virtual-dtor"   COMPILER_RT_HAS_WNON_VIRTUAL_DTOR_FLAG)
check_cxx_compiler_flag(" -Wvariadic-macros"    COMPILER_RT_HAS_WVARIADIC_MACROS_FLAG)
check_cxx_compiler_flag(" -Wunused-parameter"   COMPILER_RT_HAS_WUNUSED_PARAMETER_FLAG)

check_cxx_compiler_flag(/W4 COMPILER_RT_HAS_W4_FLAG)
check_cxx_compiler_flag(/WX COMPILER_RT_HAS_WX_FLAG)
check_cxx_compiler_flag(/wd4146 COMPILER_RT_HAS_WD4146_FLAG)
check_cxx_compiler_flag(/wd4291 COMPILER_RT_HAS_WD4291_FLAG)
check_cxx_compiler_flag(/wd4221 COMPILER_RT_HAS_WD4221_FLAG)
check_cxx_compiler_flag(/wd4391 COMPILER_RT_HAS_WD4391_FLAG)
check_cxx_compiler_flag(/wd4722 COMPILER_RT_HAS_WD4722_FLAG)
check_cxx_compiler_flag(/wd4800 COMPILER_RT_HAS_WD4800_FLAG)

# Symbols.
check_symbol_exists(__func__ "" COMPILER_RT_HAS_FUNC_SYMBOL)

# Libraries.
check_library_exists(c fopen "" COMPILER_RT_HAS_LIBC)
check_library_exists(dl dlopen "" COMPILER_RT_HAS_LIBDL)
check_library_exists(rt shm_open "" COMPILER_RT_HAS_LIBRT)
check_library_exists(m pow "" COMPILER_RT_HAS_LIBM)
check_library_exists(pthread pthread_create "" COMPILER_RT_HAS_LIBPTHREAD)
check_library_exists(stdc++ __cxa_throw "" COMPILER_RT_HAS_LIBSTDCXX)

# Linker flags.
if(ANDROID)
  check_linker_flag("-Wl,-z,global" COMPILER_RT_HAS_Z_GLOBAL)
  check_library_exists(log __android_log_write "" COMPILER_RT_HAS_LIBLOG)
endif()

# Architectures.

# List of all architectures we can target.
set(COMPILER_RT_SUPPORTED_ARCH)

# Try to compile a very simple source file to ensure we can target the given
# platform. We use the results of these tests to build only the various target
# runtime libraries supported by our current compilers cross-compiling
# abilities.
set(SIMPLE_SOURCE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/simple.cc)
file(WRITE ${SIMPLE_SOURCE} "#include <stdlib.h>\n#include <limits>\nint main() {}\n")

# Add $arch as supported with no additional flags.
macro(add_default_target_arch arch)
  set(TARGET_${arch}_CFLAGS "")
  set(CAN_TARGET_${arch} 1)
  list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
endmacro()

# Detect whether the current target platform is 32-bit or 64-bit, and setup
# the correct commandline flags needed to attempt to target 32-bit and 64-bit.
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND
    NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Please use architecture with 4 or 8 byte pointers.")
endif()

test_targets()

# Returns a list of architecture specific target cflags in @out_var list.
function(get_target_flags_for_arch arch out_var)
  list(FIND COMPILER_RT_SUPPORTED_ARCH ${arch} ARCH_INDEX)
  if(ARCH_INDEX EQUAL -1)
    message(FATAL_ERROR "Unsupported architecture: ${arch}")
  else()
    if (NOT APPLE)
      set(${out_var} ${TARGET_${arch}_CFLAGS} PARENT_SCOPE)
    else()
      # This is only called in constructing cflags for tests executing on the
      # host. This will need to all be cleaned up to support building tests
      # for cross-targeted hardware (i.e. iOS).
      set(${out_var} -arch ${arch} PARENT_SCOPE)
    endif()
  endif()
endfunction()

set(ARM64 aarch64)
set(ARM32 arm armhf)
set(X86 i386 i686)
set(X86_64 x86_64)
set(MIPS32 mips mipsel)
set(MIPS64 mips64 mips64el)
set(PPC64 powerpc64 powerpc64le)
set(S390X s390x)
set(WASM32 wasm32)
set(WASM64 wasm64)

if(APPLE)
  set(ARM64 arm64)
  set(ARM32 armv7 armv7s armv7k)
  set(X86_64 x86_64 x86_64h)
endif()

set(ALL_SANITIZER_COMMON_SUPPORTED_ARCH ${X86} ${X86_64} ${PPC64}
    ${ARM32} ${ARM64} ${MIPS32} ${MIPS64} ${S390X})
set(ALL_ASAN_SUPPORTED_ARCH ${X86} ${X86_64} ${ARM32} ${ARM64}
    ${MIPS32} ${MIPS64} ${PPC64})
set(ALL_DFSAN_SUPPORTED_ARCH ${X86_64} ${MIPS64} ${ARM64})
set(ALL_LSAN_SUPPORTED_ARCH ${X86_64} ${MIPS64} ${ARM64})
set(ALL_MSAN_SUPPORTED_ARCH ${X86_64} ${MIPS64} ${ARM64} ${PPC64})
set(ALL_PROFILE_SUPPORTED_ARCH ${X86} ${X86_64} ${ARM32} ${ARM64} ${PPC64}
    ${MIPS32} ${MIPS64})
set(ALL_TSAN_SUPPORTED_ARCH ${X86_64} ${MIPS64} ${ARM64} ${PPC64})
set(ALL_UBSAN_SUPPORTED_ARCH ${X86} ${X86_64} ${ARM32} ${ARM64}
    ${MIPS32} ${MIPS64} ${PPC64} ${S390X})
set(ALL_SAFESTACK_SUPPORTED_ARCH ${X86} ${X86_64} ${ARM64} ${MIPS32} ${MIPS64})
set(ALL_CFI_SUPPORTED_ARCH ${X86} ${X86_64} ${MIPS64})
set(ALL_ESAN_SUPPORTED_ARCH ${X86_64})
set(ALL_SCUDO_SUPPORTED_ARCH ${X86_64})

if(APPLE)
  include(CompilerRTDarwinUtils)

  find_darwin_sdk_dir(DARWIN_osx_SYSROOT macosx)
  find_darwin_sdk_dir(DARWIN_iossim_SYSROOT iphonesimulator)
  find_darwin_sdk_dir(DARWIN_ios_SYSROOT iphoneos)
  find_darwin_sdk_dir(DARWIN_watchossim_SYSROOT watchsimulator)
  find_darwin_sdk_dir(DARWIN_watchos_SYSROOT watchos)
  find_darwin_sdk_dir(DARWIN_tvossim_SYSROOT appletvsimulator)
  find_darwin_sdk_dir(DARWIN_tvos_SYSROOT appletvos)

  if(COMPILER_RT_ENABLE_IOS)
    list(APPEND DARWIN_EMBEDDED_PLATFORMS ios)
    set(DARWIN_ios_MIN_VER_FLAG -miphoneos-version-min)
    set(DARWIN_ios_SANITIZER_MIN_VER_FLAG
      ${DARWIN_ios_MIN_VER_FLAG}=7.0)
  endif()
  if(COMPILER_RT_ENABLE_WATCHOS)
    list(APPEND DARWIN_EMBEDDED_PLATFORMS watchos)
    set(DARWIN_watchos_MIN_VER_FLAG -mwatchos-version-min)
    set(DARWIN_watchos_SANITIZER_MIN_VER_FLAG
      ${DARWIN_watchos_MIN_VER_FLAG}=2.0)
  endif()
  if(COMPILER_RT_ENABLE_TVOS)
    list(APPEND DARWIN_EMBEDDED_PLATFORMS tvos)
    set(DARWIN_tvos_MIN_VER_FLAG -mtvos-version-min)
    set(DARWIN_tvos_SANITIZER_MIN_VER_FLAG
      ${DARWIN_tvos_MIN_VER_FLAG}=9.0)
  endif()

  # Note: In order to target x86_64h on OS X the minimum deployment target must
  # be 10.8 or higher.
  set(SANITIZER_COMMON_SUPPORTED_OS osx)
  set(PROFILE_SUPPORTED_OS osx)
  set(TSAN_SUPPORTED_OS osx)
  if(NOT SANITIZER_MIN_OSX_VERSION)
    string(REGEX MATCH "-mmacosx-version-min=([.0-9]+)"
           MACOSX_VERSION_MIN_FLAG "${CMAKE_CXX_FLAGS}")
    if(MACOSX_VERSION_MIN_FLAG)
      set(SANITIZER_MIN_OSX_VERSION "${CMAKE_MATCH_1}")
    elseif(CMAKE_OSX_DEPLOYMENT_TARGET)
      set(SANITIZER_MIN_OSX_VERSION ${CMAKE_OSX_DEPLOYMENT_TARGET})
    else()
      set(SANITIZER_MIN_OSX_VERSION 10.9)
    endif()
    if(SANITIZER_MIN_OSX_VERSION VERSION_LESS "10.7")
      message(FATAL_ERROR "Too old OS X version: ${SANITIZER_MIN_OSX_VERSION}")
    endif()
  endif()

  # We're setting the flag manually for each target OS
  set(CMAKE_OSX_DEPLOYMENT_TARGET "")
  
  set(DARWIN_COMMON_CFLAGS -stdlib=libc++)
  set(DARWIN_COMMON_LINKFLAGS
    -stdlib=libc++
    -lc++
    -lc++abi)
  
  check_linker_flag("-fapplication-extension" COMPILER_RT_HAS_APP_EXTENSION)
  if(COMPILER_RT_HAS_APP_EXTENSION)
    list(APPEND DARWIN_COMMON_LINKFLAGS "-fapplication-extension")
  endif()

  set(DARWIN_osx_CFLAGS
    ${DARWIN_COMMON_CFLAGS}
    -mmacosx-version-min=${SANITIZER_MIN_OSX_VERSION})
  set(DARWIN_osx_LINKFLAGS
    ${DARWIN_COMMON_LINKFLAGS}
    -mmacosx-version-min=${SANITIZER_MIN_OSX_VERSION})

  if(DARWIN_osx_SYSROOT)
    list(APPEND DARWIN_osx_CFLAGS -isysroot ${DARWIN_osx_SYSROOT})
    list(APPEND DARWIN_osx_LINKFLAGS -isysroot ${DARWIN_osx_SYSROOT})
  endif()

  # Figure out which arches to use for each OS
  darwin_get_toolchain_supported_archs(toolchain_arches)
  message(STATUS "Toolchain supported arches: ${toolchain_arches}")
  
  if(NOT MACOSX_VERSION_MIN_FLAG)
    darwin_test_archs(osx
      DARWIN_osx_ARCHS
      ${toolchain_arches})
    message(STATUS "OSX supported arches: ${DARWIN_osx_ARCHS}")
    foreach(arch ${DARWIN_osx_ARCHS})
      list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
      set(CAN_TARGET_${arch} 1)
    endforeach()

    foreach(platform ${DARWIN_EMBEDDED_PLATFORMS})
      if(DARWIN_${platform}sim_SYSROOT)
        set(DARWIN_${platform}sim_CFLAGS
          ${DARWIN_COMMON_CFLAGS}
          ${DARWIN_${platform}_SANITIZER_MIN_VER_FLAG}
          -isysroot ${DARWIN_${platform}sim_SYSROOT})
        set(DARWIN_${platform}sim_LINKFLAGS
          ${DARWIN_COMMON_LINKFLAGS}
          ${DARWIN_${platform}_SANITIZER_MIN_VER_FLAG}
          -isysroot ${DARWIN_${platform}sim_SYSROOT})

        set(DARWIN_${platform}sim_SKIP_CC_KEXT On)
        darwin_test_archs(${platform}sim
          DARWIN_${platform}sim_ARCHS
          ${toolchain_arches})
        message(STATUS "${platform} Simulator supported arches: ${DARWIN_${platform}sim_ARCHS}")
        if(DARWIN_${platform}_ARCHS)
          list(APPEND SANITIZER_COMMON_SUPPORTED_OS ${platform}sim)
          list(APPEND PROFILE_SUPPORTED_OS ${platform}sim)
          if(DARWIN_${platform}_SYSROOT_INTERNAL)
            list(APPEND TSAN_SUPPORTED_OS ${platform}sim)
          endif()
        endif()
        foreach(arch ${DARWIN_${platform}sim_ARCHS})
          list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
          set(CAN_TARGET_${arch} 1)
        endforeach()
      endif()

      if(DARWIN_${platform}_SYSROOT)
        set(DARWIN_${platform}_CFLAGS
          ${DARWIN_COMMON_CFLAGS}
          ${DARWIN_${platform}_SANITIZER_MIN_VER_FLAG}
          -isysroot ${DARWIN_${platform}_SYSROOT})
        set(DARWIN_${platform}_LINKFLAGS
          ${DARWIN_COMMON_LINKFLAGS}
          ${DARWIN_${platform}_SANITIZER_MIN_VER_FLAG}
          -isysroot ${DARWIN_${platform}_SYSROOT})

        darwin_test_archs(${platform}
          DARWIN_${platform}_ARCHS
          ${toolchain_arches})
        message(STATUS "${platform} supported arches: ${DARWIN_${platform}_ARCHS}")
        if(DARWIN_${platform}_ARCHS)
          list(APPEND SANITIZER_COMMON_SUPPORTED_OS ${platform})
          list(APPEND PROFILE_SUPPORTED_OS ${platform})
        endif()
        foreach(arch ${DARWIN_${platform}_ARCHS})
          list(APPEND COMPILER_RT_SUPPORTED_ARCH ${arch})
          set(CAN_TARGET_${arch} 1)
        endforeach()
      endif()
    endforeach()
  endif()

  # for list_intersect
  include(CompilerRTUtils)


  list_intersect(SANITIZER_COMMON_SUPPORTED_ARCH
    ALL_SANITIZER_COMMON_SUPPORTED_ARCH
    COMPILER_RT_SUPPORTED_ARCH
    )
  set(LSAN_COMMON_SUPPORTED_ARCH ${SANITIZER_COMMON_SUPPORTED_ARCH})
  set(UBSAN_COMMON_SUPPORTED_ARCH ${SANITIZER_COMMON_SUPPORTED_ARCH})
  list_intersect(ASAN_SUPPORTED_ARCH
    ALL_ASAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(DFSAN_SUPPORTED_ARCH
    ALL_DFSAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(LSAN_SUPPORTED_ARCH
    ALL_LSAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(MSAN_SUPPORTED_ARCH
    ALL_MSAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(PROFILE_SUPPORTED_ARCH
    ALL_PROFILE_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(TSAN_SUPPORTED_ARCH
    ALL_TSAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(UBSAN_SUPPORTED_ARCH
    ALL_UBSAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(SAFESTACK_SUPPORTED_ARCH
    ALL_SAFESTACK_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(CFI_SUPPORTED_ARCH
    ALL_CFI_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(ESAN_SUPPORTED_ARCH
    ALL_ESAN_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
  list_intersect(SCUDO_SUPPORTED_ARCH
    ALL_SCUDO_SUPPORTED_ARCH
    SANITIZER_COMMON_SUPPORTED_ARCH)
else()
  # Architectures supported by compiler-rt libraries.
  filter_available_targets(SANITIZER_COMMON_SUPPORTED_ARCH
    ${ALL_SANITIZER_COMMON_SUPPORTED_ARCH})
  # LSan and UBSan common files should be available on all architectures
  # supported by other sanitizers (even if they build into dummy object files).
  filter_available_targets(LSAN_COMMON_SUPPORTED_ARCH
    ${SANITIZER_COMMON_SUPPORTED_ARCH})
  filter_available_targets(UBSAN_COMMON_SUPPORTED_ARCH
    ${SANITIZER_COMMON_SUPPORTED_ARCH})
  filter_available_targets(ASAN_SUPPORTED_ARCH ${ALL_ASAN_SUPPORTED_ARCH})
  filter_available_targets(DFSAN_SUPPORTED_ARCH ${ALL_DFSAN_SUPPORTED_ARCH})
  filter_available_targets(LSAN_SUPPORTED_ARCH ${ALL_LSAN_SUPPORTED_ARCH})
  filter_available_targets(MSAN_SUPPORTED_ARCH ${ALL_MSAN_SUPPORTED_ARCH})
  filter_available_targets(PROFILE_SUPPORTED_ARCH ${ALL_PROFILE_SUPPORTED_ARCH})
  filter_available_targets(TSAN_SUPPORTED_ARCH ${ALL_TSAN_SUPPORTED_ARCH})
  filter_available_targets(UBSAN_SUPPORTED_ARCH ${ALL_UBSAN_SUPPORTED_ARCH})
  filter_available_targets(SAFESTACK_SUPPORTED_ARCH
    ${ALL_SAFESTACK_SUPPORTED_ARCH})
  filter_available_targets(CFI_SUPPORTED_ARCH ${ALL_CFI_SUPPORTED_ARCH})
  filter_available_targets(ESAN_SUPPORTED_ARCH ${ALL_ESAN_SUPPORTED_ARCH})
  filter_available_targets(SCUDO_SUPPORTED_ARCH
    ${ALL_SCUDO_SUPPORTED_ARCH})
endif()

if (MSVC)
  # See if the DIA SDK is available and usable.
  set(MSVC_DIA_SDK_DIR "$ENV{VSINSTALLDIR}DIA SDK")
  if (IS_DIRECTORY ${MSVC_DIA_SDK_DIR})
    set(CAN_SYMBOLIZE 1)
  else()
    set(CAN_SYMBOLIZE 0)
  endif()
else()
  set(CAN_SYMBOLIZE 1)
endif()

message(STATUS "Compiler-RT supported architectures: ${COMPILER_RT_SUPPORTED_ARCH}")

if(ANDROID)
  set(OS_NAME "Android")
else()
  set(OS_NAME "${CMAKE_SYSTEM_NAME}")
endif()

if (SANITIZER_COMMON_SUPPORTED_ARCH AND NOT LLVM_USE_SANITIZER AND
    (OS_NAME MATCHES "Android|Darwin|Linux|FreeBSD" OR
    (OS_NAME MATCHES "Windows" AND MSVC)))
  set(COMPILER_RT_HAS_SANITIZER_COMMON TRUE)
else()
  set(COMPILER_RT_HAS_SANITIZER_COMMON FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON)
  set(COMPILER_RT_HAS_INTERCEPTION TRUE)
else()
  set(COMPILER_RT_HAS_INTERCEPTION FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND ASAN_SUPPORTED_ARCH)
  set(COMPILER_RT_HAS_ASAN TRUE)
else()
  set(COMPILER_RT_HAS_ASAN FALSE)
endif()

if (OS_NAME MATCHES "Linux|FreeBSD|Windows")
  set(COMPILER_RT_ASAN_HAS_STATIC_RUNTIME TRUE)
else()
  set(COMPILER_RT_ASAN_HAS_STATIC_RUNTIME FALSE)
endif()

# TODO: Add builtins support.

if (COMPILER_RT_HAS_SANITIZER_COMMON AND DFSAN_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Linux")
  set(COMPILER_RT_HAS_DFSAN TRUE)
else()
  set(COMPILER_RT_HAS_DFSAN FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND LSAN_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Linux|FreeBSD")
  set(COMPILER_RT_HAS_LSAN TRUE)
else()
  set(COMPILER_RT_HAS_LSAN FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND MSAN_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Linux")
  set(COMPILER_RT_HAS_MSAN TRUE)
else()
  set(COMPILER_RT_HAS_MSAN FALSE)
endif()

if (PROFILE_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Darwin|Linux|FreeBSD|Windows")
  set(COMPILER_RT_HAS_PROFILE TRUE)
else()
  set(COMPILER_RT_HAS_PROFILE FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND TSAN_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Darwin|Linux|FreeBSD")
  set(COMPILER_RT_HAS_TSAN TRUE)
else()
  set(COMPILER_RT_HAS_TSAN FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND UBSAN_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Darwin|Linux|FreeBSD|Windows")
  set(COMPILER_RT_HAS_UBSAN TRUE)
else()
  set(COMPILER_RT_HAS_UBSAN FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND SAFESTACK_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Darwin|Linux|FreeBSD")
  set(COMPILER_RT_HAS_SAFESTACK TRUE)
else()
  set(COMPILER_RT_HAS_SAFESTACK FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND CFI_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Linux")
  set(COMPILER_RT_HAS_CFI TRUE)
else()
  set(COMPILER_RT_HAS_CFI FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND ESAN_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Linux")
  set(COMPILER_RT_HAS_ESAN TRUE)
else()
  set(COMPILER_RT_HAS_ESAN FALSE)
endif()

if (COMPILER_RT_HAS_SANITIZER_COMMON AND SCUDO_SUPPORTED_ARCH AND
    OS_NAME MATCHES "Linux")
  set(COMPILER_RT_HAS_SCUDO TRUE)
else()
  set(COMPILER_RT_HAS_SCUDO FALSE)
endif()

