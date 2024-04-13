#ifndef __ASM_H
#define __ASM_H

#define PR_(sym) __##sym

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define HIDDEN_(sym) .hidden PR_(sym)
#else
#define HIDDEN_(sym)
#endif

#define FN_(sym) \
    .global PR_(sym); \
    HIDDEN_(sym); \
    .type PR_(sym),@function;
#define SIZE(sym) \
    .size PR_(sym),.-PR_(sym); \
    .align 8;

#ifdef __CET__
#if defined(DO_MMX_ASM)
#define ENDBR_ endbr32
#elif defined(DO_AMD64_ASM)
#define ENDBR_ endbr64
#endif
#else
#define ENDBR_
#endif

#endif                          /* __ASM_H */
