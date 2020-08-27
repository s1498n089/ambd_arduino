/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * common internal API header
 */

#ifndef AVUTIL_INTERNAL_H
#define AVUTIL_INTERNAL_H

#if !defined(DEBUG) && !defined(NDEBUG)
#    define NDEBUG
#endif

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "config.h"
#include "attributes.h"
#include "dict.h"
#include "macros.h"
#include "mem.h"
#include "pixfmt.h"

#if ARCH_X86
#   include "x86/emms.h"
#endif

#ifndef emms_c
#   define emms_c()
#endif

#ifndef attribute_align_arg
#if ARCH_X86_32 && AV_GCC_VERSION_AT_LEAST(4,2)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
#else
#    define attribute_align_arg
#endif
#endif

#if HAVE_PRAGMA_DEPRECATED
#    if defined(__ICL)
#        define FF_DISABLE_DEPRECATION_WARNINGS __pragma(warning(push)) __pragma(warning(disable:1478))
#        define FF_ENABLE_DEPRECATION_WARNINGS  __pragma(warning(pop))
#    elif defined(_MSC_VER)
#        define FF_DISABLE_DEPRECATION_WARNINGS __pragma(warning(push)) __pragma(warning(disable:4996))
#        define FF_ENABLE_DEPRECATION_WARNINGS  __pragma(warning(pop))
#    else
#        define FF_DISABLE_DEPRECATION_WARNINGS _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#        define FF_ENABLE_DEPRECATION_WARNINGS  _Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#    endif
#else
#    define FF_DISABLE_DEPRECATION_WARNINGS
#    define FF_ENABLE_DEPRECATION_WARNINGS
#endif

// Some broken preprocessors need a second expansion
// to be forced to tokenize __VA_ARGS__
#define E1(x) x

/* Check if the hard coded offset of a struct member still matches reality.
 * Induce a compilation failure if not.
 */
#define AV_CHECK_OFFSET(s, m, o) struct check_##o {    \
        int x_##o[offsetof(s, m) == o? 1: -1];         \
    }

#define LOCAL_ALIGNED_A(a, t, v, s, o, ...)             \
    uint8_t la_##v[sizeof(t s o) + (a)];                \
    t (*v) o = (void *)FFALIGN((uintptr_t)la_##v, a)

#define LOCAL_ALIGNED_D(a, t, v, s, o, ...)             \
    DECLARE_ALIGNED(a, t, la_##v) s o;                  \
    t (*v) o = la_##v

#define LOCAL_ALIGNED(a, t, v, ...) LOCAL_ALIGNED_##a(t, v, __VA_ARGS__)

#if HAVE_LOCAL_ALIGNED_8
#   define LOCAL_ALIGNED_8(t, v, ...) E1(LOCAL_ALIGNED_D(8, t, v, __VA_ARGS__,,))
#else
#   define LOCAL_ALIGNED_8(t, v, ...) E1(LOCAL_ALIGNED_A(8, t, v, __VA_ARGS__,,))
#endif

#if HAVE_LOCAL_ALIGNED_16
#   define LOCAL_ALIGNED_16(t, v, ...) E1(LOCAL_ALIGNED_D(16, t, v, __VA_ARGS__,,))
#else
#   define LOCAL_ALIGNED_16(t, v, ...) E1(LOCAL_ALIGNED_A(16, t, v, __VA_ARGS__,,))
#endif

#if HAVE_LOCAL_ALIGNED_32
#   define LOCAL_ALIGNED_32(t, v, ...) E1(LOCAL_ALIGNED_D(32, t, v, __VA_ARGS__,,))
#else
#   define LOCAL_ALIGNED_32(t, v, ...) E1(LOCAL_ALIGNED_A(32, t, v, __VA_ARGS__,,))
#endif

#define FF_ALLOC_OR_GOTO(ctx, p, size, label)\
{\
    p = av_malloc(size);\
    if (!(p) && (size) != 0) {\
        av_log(ctx, AV_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#define FF_ALLOCZ_OR_GOTO(ctx, p, size, label)\
{\
    p = av_mallocz(size);\
    if (!(p) && (size) != 0) {\
        av_log(ctx, AV_LOG_ERROR, "Cannot allocate memory.\n");\
        goto label;\
    }\
}

#include "libm.h"

/**
 * Return NULL if CONFIG_SMALL is true, otherwise the argument
 * without modification. Used to disable the definition of strings
 * (for example AVCodec long_names).
 */
#if CONFIG_SMALL
#   define NULL_IF_CONFIG_SMALL(x) NULL
#else
#   define NULL_IF_CONFIG_SMALL(x) x
#endif


/**
 * Define a function with only the non-default version specified.
 *
 * On systems with ELF shared libraries, all symbols exported from
 * Libav libraries are tagged with the name and major version of the
 * library to which they belong.  If a function is moved from one
 * library to another, a wrapper must be retained in the original
 * location to preserve binary compatibility.
 *
 * Functions defined with this macro will never be used to resolve
 * symbols by the build-time linker.
 *
 * @param type return type of function
 * @param name name of function
 * @param args argument list of function
 * @param ver  version tag to assign function
 */
#if HAVE_SYMVER_ASM_LABEL
#   define FF_SYMVER(type, name, args, ver)                     \
    type ff_##name args __asm__ (EXTERN_PREFIX #name "@" ver);  \
    type ff_##name args
#elif HAVE_SYMVER_GNU_ASM
#   define FF_SYMVER(type, name, args, ver)                             \
    __asm__ (".symver ff_" #name "," EXTERN_PREFIX #name "@" ver);      \
    type ff_##name args;                                                \
    type ff_##name args
#endif

/**
 * Return NULL if a threading library has not been enabled.
 * Used to disable threading functions in AVCodec definitions
 * when not needed.
 */
#if HAVE_THREADS
#   define ONLY_IF_THREADS_ENABLED(x) x
#else
#   define ONLY_IF_THREADS_ENABLED(x) NULL
#endif

/**
 * Log a generic warning message about a missing feature.
 *
 * @param[in] avc a pointer to an arbitrary struct of which the first
 *                field is a pointer to an AVClass struct
 * @param[in] msg string containing the name of the missing feature
 */
void avpriv_report_missing_feature(void *avc,
                                   const char *msg, ...) av_printf_format(2, 3);

/**
 * Log a generic warning message about a missing feature.
 * Additionally request that a sample showcasing the feature be uploaded.
 *
 * @param[in] avc a pointer to an arbitrary struct of which the first field is
 *                a pointer to an AVClass struct
 * @param[in] msg string containing the name of the missing feature
 */
void avpriv_request_sample(void *avc,
                           const char *msg, ...) av_printf_format(2, 3);

#if HAVE_LIBC_MSVCRT
#include <crtversion.h>
#if defined(_VC_CRT_MAJOR_VERSION) && _VC_CRT_MAJOR_VERSION < 14
#pragma comment(linker, "/include:"EXTERN_PREFIX"avpriv_strtod")
#pragma comment(linker, "/include:"EXTERN_PREFIX"avpriv_snprintf")
#endif

#define avpriv_open ff_open
#endif

/**
 * A wrapper for open() setting O_CLOEXEC.
 */
#if !(defined(CONFIG_PLATFORM_8195A)||defined(CONFIG_PLATFORM_8721D))
int avpriv_open(const char *filename, int flags, ...);
#endif

int avpriv_set_systematic_pal2(uint32_t pal[256], enum AVPixelFormat pix_fmt);

#endif /* AVUTIL_INTERNAL_H */