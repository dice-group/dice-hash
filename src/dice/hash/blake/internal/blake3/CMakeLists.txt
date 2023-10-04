cmake_minimum_required(VERSION 3.16)

# default SIMD compiler flag configuration (can be overriden by toolchains or CLI)
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(BLAKE3_CFLAGS_SSE2 "/arch:SSE2" CACHE STRING "the compiler flags to enable SSE2")
  # MSVC has no dedicated sse4.1 flag (see https://learn.microsoft.com/en-us/cpp/build/reference/arch-x86?view=msvc-170)
  set(BLAKE3_CFLAGS_SSE4.1 "/arch:AVX" CACHE STRING "the compiler flags to enable SSE4.1")
  set(BLAKE3_CFLAGS_AVX2 "/arch:AVX2" CACHE STRING "the compiler flags to enable AVX2")
  set(BLAKE3_CFLAGS_AVX512 "/arch:AVX512" CACHE STRING "the compiler flags to enable AVX512")

elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU"
       OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
       OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  set(BLAKE3_CFLAGS_SSE2 "-msse2" CACHE STRING "the compiler flags to enable SSE2")
  set(BLAKE3_CFLAGS_SSE4.1 "-msse4.1" CACHE STRING "the compiler flags to enable SSE4.1")
  set(BLAKE3_CFLAGS_AVX2 "-mavx2" CACHE STRING "the compiler flags to enable AVX2")
  set(BLAKE3_CFLAGS_AVX512 "-mavx512f -mavx512vl" CACHE STRING "the compiler flags to enable AVX512")
endif()
# architecture lists for which to enable assembly / SIMD sources
set(BLAKE3_AMD64_NAMES amd64 AMD64 x86_64)
set(BLAKE3_X86_NAMES i686 x86 X86)
set(BLAKE3_ARMv8_NAMES aarch64 AArch64 arm64 ARM64 armv8 armv8a)

# optional SIMD sources
macro(BLAKE3_DISABLE_SIMD)
  set(BLAKE3_SIMD_AMD64_ASM OFF)
  set(BLAKE3_SIMD_X86_INTRINSICS OFF)
  set(BLAKE3_SIMD_NEON_INTRINSICS OFF)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    BLAKE3_USE_NEON=0
    BLAKE3_NO_SSE2
    BLAKE3_NO_SSE41
    BLAKE3_NO_AVX2
    BLAKE3_NO_AVX512
  )
endmacro()

set(BLAKE3_SOURCES
        blake3.c
        blake3_dispatch.c
        blake3_portable.c
)

if(CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_AMD64_NAMES OR BLAKE3_USE_AMD64_ASM)
  set(BLAKE3_SIMD_AMD64_ASM ON)

  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    enable_language(ASM_MASM)

    list(APPEND BLAKE3_SOURCES
            blake3_avx2_x86-64_windows_msvc.asm
            blake3_avx512_x86-64_windows_msvc.asm
            blake3_sse2_x86-64_windows_msvc.asm
            blake3_sse41_x86-64_windows_msvc.asm
    )

  elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU"
         OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
         OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    if (WIN32)
      list(APPEND BLAKE3_SOURCES
              blake3_avx2_x86-64_windows_gnu.S
              blake3_avx512_x86-64_windows_gnu.S
              blake3_sse2_x86-64_windows_gnu.S
              blake3_sse41_x86-64_windows_gnu.S
      )

    elseif(UNIX)
      list(APPEND BLAKE3_SOURCES
              blake3_avx2_x86-64_unix.S
              blake3_avx512_x86-64_unix.S
              blake3_sse2_x86-64_unix.S
              blake3_sse41_x86-64_unix.S
      )

    else()
      BLAKE3_DISABLE_SIMD()
    endif()

  else()  
    BLAKE3_DISABLE_SIMD()
  endif()

elseif((CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_X86_NAMES OR BLAKE3_USE_X86_INTRINSICS)
       AND DEFINED BLAKE3_CFLAGS_SSE2
       AND DEFINED BLAKE3_CFLAGS_SSE4.1
       AND DEFINED BLAKE3_CFLAGS_AVX2
       AND DEFINED BLAKE3_CFLAGS_AVX512)
  set(BLAKE3_SIMD_X86_INTRINSICS ON)

  list(APPEND BLAKE3_SOURCES
          blake3_avx2.c
          blake3_avx512.c
          blake3_sse2.c
          blake3_sse41.c
  )
  set_source_files_properties(blake3_avx2.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_AVX2}")
  set_source_files_properties(blake3_avx512.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_AVX512}")
  set_source_files_properties(blake3_sse2.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_SSE2}")
  set_source_files_properties(blake3_sse41.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_SSE4.1}")

elseif(CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_ARMv8_NAMES
        OR ((ANDROID_ABI STREQUAL "armeabi-v7a"
            OR BLAKE3_USE_NEON_INTRINSICS)
          AND (DEFINED BLAKE3_CFLAGS_NEON
            OR CMAKE_SIZEOF_VOID_P EQUAL 8)))
  set(BLAKE3_SIMD_NEON_INTRINSICS ON)

  list(APPEND BLAKE3_SOURCES
          blake3_neon.c
  )
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    BLAKE3_USE_NEON=1
  )

  if (DEFINED BLAKE3_CFLAGS_NEON)
    set_source_files_properties(blake3_neon.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_NEON}")
  endif()

else()
  BLAKE3_DISABLE_SIMD()
endif()

list(TRANSFORM BLAKE3_SOURCES PREPEND src/dice/hash/blake/internal/blake3/)
set(BLAKE3_SOURCES ${BLAKE3_SOURCES} PARENT_SCOPE)
