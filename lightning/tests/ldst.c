/* Written by Paulo Cesar Pereira de Andrade, pcpa@mandriva.com.br */

#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <lightning.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>

#define sign_extend		1
#define int_ld_st		1
#define float_ld_st		1
#define double_ld_st		1
#define float_convert		1
#define ptr_ld_st		1

/*
gcc lightning-test.c -lopcodes -lbfd -liberty -ldl -O0 -g3
 */

typedef struct types {
    long		 pad;	/* to ensure all ldx_i/stx_i use an offset */
    signed char		 c;
    unsigned char	 uc;
    signed short	 s;
    unsigned short	 us;
    signed int		 i;
    unsigned int	 ui;
    signed long		 l;
    unsigned long	 ul;
    float		 f;
    double		 d;
    void		*p;
} types_t;

typedef void (*pvv_t)(void);

struct sigaction	 act;
sigjmp_buf		 env;
void			*ip;
void			*addr;

/* use pointers to doubles and longs to ensure stack is aligned, and
 * reduce chances of test being misinterpreted due to stack misaligned */
double			 dd[16];
unsigned long		 ii[16];
int			 oo;

pvv_t			 pvv;
types_t			 t0, t1;
jit_insn		 code[16384];

char			*progname;

void			*codes[128];
int			 lengths[128];
int			 cc;


#define warn(name, format, ...)						\
	printf("%s:%d:%s: " format "\n",			\
		__FILE__, __LINE__, name ,##__VA_ARGS__);

/* shift tests may need bottom 7 bits to "properly" test what is mean't to
 * be tested, example: 0x7f(127) and -0x7f(-127) aka unsigned 0x81(129) */
#define E(v, shift)							\
    (((unsigned long)(v) >> (shift)) | ((v) & 0x7f))
#define L(v)				((unsigned long)(v))
#if __WORDSIZE == 64
#  define SLONG_VALUE			0x8000000000000001
#  define BLONG_VALUE			0x8000000000000000
#  define ULONG_VALUE			0x7fffffffffffffff
#  define FLONG_VALUE			0xffffffffffffffff
#  define C(v)				E(v, 56)
#  define S(v)				E(v, 48)
#  define I(v)				E(v, 32)
#else
#  define SLONG_VALUE			0x80000001
#  define BLONG_VALUE			0x80000000
#  define ULONG_VALUE			0x7fffffff
#  define FLONG_VALUE			0xffffffff
#  define C(v)				E(v, 24)
#  define S(v)				E(v, 16)
#  define I(v)				L(v)
#endif

void
check(char *name, int offset, unsigned long *i, double *d, void *p)
{
#if int_ld_st
    if (t0.c  != (signed char)C(*i))
	warn(name, "c: %x\t(%lx %a %p)", t0.c, *i, *d, p);
    if (t0.uc != (unsigned char)C(*i))
	warn(name, "C: %x\t(%lx %a %p)", t0.uc, *i, *d, p);
    if (t0.s  != (signed short)S(*i))
	warn(name, "s: %x\t(%lx %a %p)", t0.s, *i, *d, p);
    if (t0.us != (unsigned short)S(*i))
	warn(name, "S: %x\t(%lx %a %p)", t0.us, *i, *d, p);
    if (t0.i  != (signed int)I(*i))
	warn(name, "i: %x\t(%lx %a %p)", t0.i, *i, *d, p);
    if (t0.ui != (unsigned int)I(*i))
	warn(name, "I: %x\t(%lx %a %p)", t0.ui, *i, *d, p);
    if (t0.l  != (signed long)L(*i))
	warn(name, "l: %lx\t(%lx %a %p)", t0.l, *i, *d, p);
    if (t0.ul != (unsigned long)L(*i))
	warn(name, "L: %lx\t(%lx %a %p)", t0.ul, *i, *d, p);
#endif
#if float_ld_st
    if (t0.f  != (float)*d)
	warn(name, "f: %a\t(%lx %a %p)", t0.f, *i, *d, p);
#endif
#if double_ld_st
    if (t0.d  != *d)
	warn(name, "d: %a\t(%lx %a %p)", t0.d, *i, *d, p);
#endif
#if ptr_ld_st
    if (t0.p  != p)
	warn(name, "p: %p\t(%lx %a %p)", t0.p, *i, *d, p);
#endif
}

void
compare(char *name, int offset)
{
    if (t0.c  != t1.c)			warn(name, "c: %x %x",  t0.c,  t1.c);
    if (t0.uc != t1.uc)			warn(name, "C: %x %x",  t0.uc, t1.uc);
    if (t0.s  != t1.s)			warn(name, "s: %x %x",  t0.s,  t1.s);
    if (t0.us != t1.us)			warn(name, "S: %x %x",  t0.us, t1.us);
    if (t0.i  != t1.i)			warn(name, "i: %x %x",  t0.i,  t1.i);
    if (t0.ui != t1.ui)			warn(name, "I: %x %x",  t0.ui, t1.ui);
    if (t0.l  != t1.l)			warn(name, "l: %x %x",  t0.l,  t1.l);
    if (t0.ul != t1.ul)			warn(name, "L: %x %x",  t0.ul, t1.ul);
    if (t0.f  != t1.f)			warn(name, "f: %a %a",  t0.f,  t1.f);
    if (t0.d  != t1.d)			warn(name, "d: %a %a",  t0.d,  t1.d);
    if (t0.p  != t1.p)			warn(name, "p: %p %p",  t0.p,  t1.p);
}

void
check_c(char *name, int offset)
{
    if (t0.c != t0.s)			warn(name, "cs: %x %x",  t0.c, t0.s);
    if (t0.c != t0.i)			warn(name, "cs: %x %x",  t0.c, t0.i);
    if (t0.c != t0.l)			warn(name, "ci: %x %x",  t0.c, t0.l);
}

void
check_uc(char *name, int offset)
{
    if (t0.uc != t0.us)			warn(name, "CS: %x %x",  t0.uc, t0.us);
    if (t0.uc != t0.ui)			warn(name, "CI: %x %x",  t0.uc, t0.ui);
    if (t0.uc != t0.ul)			warn(name, "CL: %x %x",  t0.uc, t0.ul);
}

void
check_s(char *name, int offset)
{
    if ((signed char)t0.s != t0.c)	warn(name, "sc: %x %x",  t0.s, t0.c);
    if (t0.s != t0.i)			warn(name, "si: %x %x",  t0.s, t0.i);
    if (t0.s != t0.l)			warn(name, "sl: %x %x",  t0.s, t0.l);
}

void
check_us(char *name, int offset)
{
    if ((unsigned char)t0.us != t0.uc)	warn(name, "SC: %x %x",  t0.us, t0.uc);
    if (t0.us != t0.ui)			warn(name, "SI: %x %x",  t0.us, t0.ui);
    if (t0.us != t0.ul)			warn(name, "SL: %x %x",  t0.us, t0.ul);
}

void
check_i(char *name, int offset)
{
    if ((signed char)t0.i != t0.c)	warn(name, "ic: %x %x",  t0.i, t0.c);
    if ((signed short)t0.i != t0.s)	warn(name, "is: %x %x",  t0.i, t0.s);
    if (t0.i != t0.l)			warn(name, "il: %x %x",  t0.i, t0.l);
}

void
check_ui(char *name, int offset)
{
    if ((unsigned char)t0.ui != t0.uc)	warn(name, "IC: %x %x",  t0.ui, t0.uc);
    if ((unsigned short)t0.ui != t0.us)	warn(name, "IS: %x %x",  t0.ui, t0.us);
    if (t0.ui != t0.ul)			warn(name, "IL: %x %x",  t0.ui, t0.ul);
}

void
check_f(char *name, int offset)
{
    if (t0.f != (float)t0.d)		warn(name, "fd: %a %a",  t0.f, t0.d);
}

void
check_d(char *name, int offset)
{
    if ((double)t0.f != t0.d)		warn(name, "fd: %a %a",  t0.f, t0.d);
}

void
stxi(types_t *t, unsigned long i, double d, void *p, int V0, int R0, int F0)
{
    jit_movi_p	(			V0, t);
#if int_ld_st
    jit_movi_i	(			R0, C(i));
    jit_stxi_c	(offsetof(types_t, c),	V0, R0);
    jit_stxi_uc	(offsetof(types_t, uc),	V0, R0);
    jit_movi_i	(			R0, S(i));
    jit_stxi_s	(offsetof(types_t, s),	V0, R0);
    jit_stxi_us	(offsetof(types_t, us),	V0, R0);
    jit_movi_i	(			R0, I(i));
    jit_stxi_i	(offsetof(types_t, i),	V0, R0);
    jit_stxi_ui	(offsetof(types_t, ui),	V0, R0);
    jit_movi_l	(			R0, L(i));
    jit_stxi_l	(offsetof(types_t, l),	V0, R0);
    jit_stxi_ul	(offsetof(types_t, ul),	V0, R0);
#endif
#if float_ld_st
    jit_movi_f	(			F0, (float)d);
    jit_stxi_f	(offsetof(types_t, f),	V0, F0);
#endif
#if double_ld_st
    jit_movi_d	(			F0, d);
    jit_stxi_d	(offsetof(types_t, d),	V0, F0);
#endif
#if ptr_ld_st
    jit_movi_p	(			R0, p);
    jit_stxi_p	(offsetof(types_t, p),	V0, R0);
#endif
}

void
movxi(types_t *ta, types_t *tb, int V0, int V1, int R0, int F0)
{
    jit_movi_p	(			V0, ta);
    jit_movi_p	(			V1, tb);
#if int_ld_st
    jit_ldxi_c	(			R0, V0, offsetof(types_t, c));
    jit_stxi_c	(offsetof(types_t, c),	V1, R0);
    jit_ldxi_uc	(			R0, V0, offsetof(types_t, uc));
    jit_stxi_uc	(offsetof(types_t, uc),	V1, R0);
    jit_ldxi_s	(			R0, V0, offsetof(types_t, s));
    jit_stxi_s	(offsetof(types_t, s),	V1, R0);
    jit_ldxi_us	(			R0, V0, offsetof(types_t, us));
    jit_stxi_us	(offsetof(types_t, us),	V1, R0);
    jit_ldxi_i	(			R0, V0, offsetof(types_t, i));
    jit_stxi_i	(offsetof(types_t, i),	V1, R0);
    jit_ldxi_ui	(			R0, V0, offsetof(types_t, ui));
    jit_stxi_ui	(offsetof(types_t, ui),	V1, R0);
    jit_ldxi_l	(			R0, V0, offsetof(types_t, l));
    jit_stxi_l	(offsetof(types_t, l),	V1, R0);
    jit_ldxi_ul	(			R0, V0, offsetof(types_t, ul));
    jit_stxi_ul	(offsetof(types_t, ul),	V1, R0);
#endif
#if float_ld_st
    jit_ldxi_f	(			F0, V0, offsetof(types_t, f));
    jit_stxi_f	(offsetof(types_t, f),	V1, F0);
#endif
#if double_ld_st
    jit_ldxi_d	(			F0, V0, offsetof(types_t, d));
    jit_stxi_d	(offsetof(types_t, d),	V1, F0);
#endif
#if ptr_ld_st
    jit_ldxi_p	(			R0, V0, offsetof(types_t, p));
    jit_stxi_p	(offsetof(types_t, p),	V1, R0);
#endif
}

void
xi_c(types_t *t, int V0, int R0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_c	(			R0, V0, offsetof(types_t, c));
    jit_stxi_s	(offsetof(types_t, s),	V0, R0);
    jit_stxi_i	(offsetof(types_t, i),	V0, R0);
    jit_stxi_l	(offsetof(types_t, l),	V0, R0);
}

void
xi_uc(types_t *t, int V0, int R0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_uc	(			R0, V0, offsetof(types_t, uc));
    jit_stxi_us	(offsetof(types_t, us),	V0, R0);
    jit_stxi_ui	(offsetof(types_t, ui),	V0, R0);
    jit_stxi_ul	(offsetof(types_t, ul),	V0, R0);
}

void
xi_s(types_t *t, int V0, int R0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_s	(			R0, V0, offsetof(types_t, s));
    jit_stxi_c	(offsetof(types_t, c),	V0, R0);
    jit_stxi_i	(offsetof(types_t, i),	V0, R0);
    jit_stxi_l	(offsetof(types_t, l),	V0, R0);
}

void
xi_us(types_t *t, int V0, int R0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_us	(			R0, V0, offsetof(types_t, us));
    jit_stxi_uc	(offsetof(types_t, uc),	V0, R0);
    jit_stxi_ui	(offsetof(types_t, ui),	V0, R0);
    jit_stxi_ul	(offsetof(types_t, ul),	V0, R0);
}

void
xi_i(types_t *t, int V0, int R0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_i	(			R0, V0, offsetof(types_t, i));
    jit_stxi_c	(offsetof(types_t, c),	V0, R0);
    jit_stxi_s	(offsetof(types_t, s),	V0, R0);
    jit_stxi_l	(offsetof(types_t, l),	V0, R0);
}

void
xi_ui(types_t *t, int V0, int R0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_ui	(			R0, V0, offsetof(types_t, ui));
    jit_stxi_uc	(offsetof(types_t, uc),	V0, R0);
    jit_stxi_us	(offsetof(types_t, us),	V0, R0);
    jit_stxi_ul	(offsetof(types_t, ul),	V0, R0);
}

void
xi_f(types_t *t, int V0, int F0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_f	(			F0, V0, offsetof(types_t, f));
    jit_extr_f_d(F0, F0);
    jit_stxi_d	(offsetof(types_t, d),	V0, F0);
}

void
xi_d(types_t *t, int V0, int F0)
{
    jit_movi_p	(			V0, t);
    jit_ldxi_d	(			F0, V0, offsetof(types_t, d));
    jit_extr_d_f(F0, F0);
    jit_stxi_f	(offsetof(types_t, f),	V0, F0);
}

void
stxr(types_t *t, unsigned long i, double d, void *p,
     int V0, int R0, int R1, int F0)
{
    jit_movi_p	(V0, t);
#if int_ld_st
    jit_movi_i	(R0, C(i));
    jit_movi_i	(R1, offsetof(types_t, c));
    jit_stxr_c	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, uc));
    jit_stxr_uc	(R1, V0, R0);
    jit_movi_i	(R0, S(i));
    jit_movi_i	(R1, offsetof(types_t, s));
    jit_stxr_s	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, us));
    jit_stxr_us	(R1, V0, R0);
    jit_movi_i	(R0, I(i));
    jit_movi_i	(R1, offsetof(types_t, i));
    jit_stxr_i	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ui));
    jit_stxr_ui	(R1, V0, R0);
    jit_movi_l	(R0, L(i));
    jit_movi_i	(R1, offsetof(types_t, l));
    jit_stxr_l	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ul));
    jit_stxr_ul	(R1, V0, R0);
#endif
#if float_ld_st
    jit_movi_f	(F0, (float)d);
    jit_movi_i	(R1, offsetof(types_t, f));
    jit_stxr_f	(R1, V0, F0);
#endif
#if double_ld_st
    jit_movi_d	(F0, d);
    jit_movi_i	(R1, offsetof(types_t, d));
    jit_stxr_d	(R1, V0, F0);
#endif
#if ptr_ld_st
    jit_movi_p	(R0, p);
    jit_movi_i	(R1, offsetof(types_t, p));
    jit_stxr_p	(R1, V0, R0);
#endif
}

void
movxr(types_t *ta, types_t *tb, int V0, int V1, int R0, int R1, int F0)
{
    jit_movi_p	(V0, ta);
    jit_movi_p	(V1, tb);
#if int_ld_st
    jit_movi_i	(R1, offsetof(types_t, c));
    jit_ldxr_c	(R0, V0, R1);
    jit_stxr_c	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, uc));
    jit_ldxr_uc	(R0, V0, R1);
    jit_stxr_uc	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, s));
    jit_ldxr_s	(R0, V0, R1);
    jit_stxr_s	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, us));
    jit_ldxr_us	(R0, V0, R1);
    jit_stxr_us	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, i));
    jit_ldxr_i	(R0, V0, R1);
    jit_stxr_i	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, ui));
    jit_ldxr_ui	(R0, V0, R1);
    jit_stxr_ui	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, l));
    jit_ldxr_l	(R0, V0, R1);
    jit_stxr_l	(R1, V1, R0);
    jit_movi_i	(R1, offsetof(types_t, ul));
    jit_ldxr_ul	(R0, V0, R1);
    jit_stxr_ul	(R1, V1, R0);
#endif
#if float_ld_st
    jit_movi_i	(R1, offsetof(types_t, f));
    jit_ldxr_f	(F0, V0, R1);
    jit_stxr_f	(R1, V1, F0);
#endif
#if double_ld_st
    jit_movi_i	(R1, offsetof(types_t, d));
    jit_ldxr_d	(F0, V0, R1);
    jit_stxr_d	(R1, V1, F0);
#endif
#if ptr_ld_st
    jit_movi_i	(R1, offsetof(types_t, p));
    jit_ldxr_p	(R0, V0, R1);
    jit_stxr_p	(R1, V1, R0);
#endif
}

void
xr_c(types_t *t, int V0, int R0, int R1)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R1, offsetof(types_t, c));
    jit_ldxr_c	(R0, V0, R1);
    jit_movi_i	(R1, offsetof(types_t, s));
    jit_stxr_s	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, i));
    jit_stxr_i	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, l));
    jit_stxr_l	(R1, V0, R0);
}

void
xr_uc(types_t *t, int V0, int R0, int R1)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R1, offsetof(types_t, uc));
    jit_ldxr_uc	(R0, V0, R1);
    jit_movi_i	(R1, offsetof(types_t, us));
    jit_stxr_us	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ui));
    jit_stxr_ui	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ul));
    jit_stxr_ul	(R1, V0, R0);
}

void
xr_s(types_t *t, int V0, int R0, int R1)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R1, offsetof(types_t, s));
    jit_ldxr_s	(R0, V0, R1);
    jit_movi_i	(R1, offsetof(types_t, c));
    jit_stxr_c	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, i));
    jit_stxr_i	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, l));
    jit_stxr_l	(R1, V0, R0);
}

void
xr_us(types_t *t, int V0, int R0, int R1)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R1, offsetof(types_t, us));
    jit_ldxr_us	(R0, V0, R1);
    jit_movi_i	(R1, offsetof(types_t, uc));
    jit_stxr_uc	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ui));
    jit_stxr_ui	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ul));
    jit_stxr_ul	(R1, V0, R0);
}

void
xr_i(types_t *t, int V0, int R0, int R1)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R1, offsetof(types_t, i));
    jit_ldxr_i	(R0, V0, R1);
    jit_movi_i	(R1, offsetof(types_t, c));
    jit_stxr_c	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, s));
    jit_stxr_s	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, l));
    jit_stxr_l	(R1, V0, R0);
}

void
xr_ui(types_t *t, int V0, int R0, int R1)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R1, offsetof(types_t, ui));
    jit_ldxr_ui	(R0, V0, R1);
    jit_movi_i	(R1, offsetof(types_t, uc));
    jit_stxr_uc	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, us));
    jit_stxr_us	(R1, V0, R0);
    jit_movi_i	(R1, offsetof(types_t, ul));
    jit_stxr_ul	(R1, V0, R0);
}

void
xr_f(types_t *t, int V0, int F0, int R0)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R0, offsetof(types_t, f));
    jit_ldxr_f	(F0, V0, R0);
    jit_extr_f_d(F0, F0);
    jit_movi_i	(R0, offsetof(types_t, d));
    jit_stxr_d	(R0, V0, F0);
}

void
xr_d(types_t *t, int V0, int F0, int R0)
{
    jit_movi_p	(V0, t);
    jit_movi_i	(R0, offsetof(types_t, d));
    jit_ldxr_d	(F0, V0, R0);
    jit_extr_d_f(F0, F0);
    jit_movi_i	(R0, offsetof(types_t, f));
    jit_stxr_f	(R0, V0, F0);
}

void
str(types_t *t, unsigned long i, double d, void *p, int V0, int R0, int F0)
{
    jit_movi_i	(R0, C(i));
#if int_ld_st
    jit_movi_p	(V0, (char *)t + offsetof(types_t, c));
    jit_str_c	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, uc));
    jit_str_uc	(V0, R0);
    jit_movi_i	(R0, S(i));
    jit_movi_p	(V0, (char *)t + offsetof(types_t, s));
    jit_str_s	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, us));
    jit_str_us	(V0, R0);
    jit_movi_i	(R0, I(i));
    jit_movi_p	(V0, (char *)t + offsetof(types_t, i));
    jit_str_i	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ui));
    jit_str_ui	(V0, R0);
    jit_movi_l	(R0, L(i));
    jit_movi_p	(V0, (char *)t + offsetof(types_t, l));
    jit_str_l	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ul));
    jit_str_ul	(V0, R0);
#endif
#if float_ld_st
    jit_movi_f	(F0, d);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, f));
    jit_str_f	(V0, F0);
#endif
#if double_ld_st
    jit_movi_d	(F0, d);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, d));
    jit_str_d	(V0, F0);
#endif
#if ptr_ld_st
    jit_movi_p	(R0, p);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, p));
    jit_str_p	(V0, R0);
#endif
}

void
movr(types_t *ta, types_t *tb, int V0, int V1, int R0, int F0)
{
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, c));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, c));
#if int_ld_st
    jit_ldr_c	(R0, V0);
    jit_str_c	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, uc));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, uc));
    jit_ldr_uc	(R0, V0);
    jit_str_uc	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, s));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, s));
    jit_ldr_s	(R0, V0);
    jit_str_s	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, us));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, us));
    jit_ldr_us	(R0, V0);
    jit_str_us	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, i));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, i));
    jit_ldr_i	(R0, V0);
    jit_str_i	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, ui));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, ui));
    jit_ldr_ui	(R0, V0);
    jit_str_ui	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, l));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, l));
    jit_ldr_l	(R0, V0);
    jit_str_l	(V1, R0);
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, ul));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, ul));
    jit_ldr_ul	(R0, V0);
    jit_str_ul	(V1, R0);
#endif
#if float_ld_st
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, f));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, f));
    jit_ldr_f	(F0, V0);
    jit_str_f	(V1, F0);
#endif
#if double_ld_st
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, d));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, d));
    jit_ldr_d	(F0, V0);
    jit_str_d	(V1, F0);
#endif
#if ptr_ld_st
    jit_movi_p	(V0, (char*)ta + offsetof(types_t, p));
    jit_movi_p	(V1, (char*)tb + offsetof(types_t, p));
    jit_ldr_p	(R0, V0);
    jit_str_p	(V1, R0);
#endif
}

void
r_c(types_t *t, int V0, int R0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, c));
    jit_ldr_c	(R0, V0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, s));
    jit_str_s	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, i));
    jit_str_i	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, l));
    jit_str_l	(V0, R0);
}

void
r_uc(types_t *t, int V0, int R0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, uc));
    jit_ldr_uc	(R0, V0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, us));
    jit_str_us	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ui));
    jit_str_ui	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ul));
    jit_str_ul	(V0, R0);
}

void
r_s(types_t *t, int V0, int R0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, s));
    jit_ldr_s	(R0, V0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, c));
    jit_str_c	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, i));
    jit_str_i	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, l));
    jit_str_l	(V0, R0);
}

void
r_us(types_t *t, int V0, int R0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, us));
    jit_ldr_us	(R0, V0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, uc));
    jit_str_uc	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ui));
    jit_str_ui	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ul));
    jit_str_ul	(V0, R0);
}

void
r_i(types_t *t, int V0, int R0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, i));
    jit_ldr_i	(R0, V0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, c));
    jit_str_c	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, s));
    jit_str_s	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, l));
    jit_str_l	(V0, R0);
}

void
r_ui(types_t *t, int V0, int R0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ui));
    jit_ldr_ui	(R0, V0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, uc));
    jit_str_uc	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, us));
    jit_str_us	(V0, R0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, ul));
    jit_str_ul	(V0, R0);
}

void
r_f(types_t *t, int V0, int F0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, f));
    jit_ldr_f	(F0, V0);
    jit_extr_f_d(F0, F0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, d));
    jit_str_d	(V0, F0);
}

void
r_d(types_t *t, int V0, int F0)
{
    jit_movi_p	(V0, (char *)t + offsetof(types_t, d));
    jit_ldr_d	(F0, V0);
    jit_extr_d_f(F0, F0);
    jit_movi_p	(V0, (char *)t + offsetof(types_t, f));
    jit_str_f	(V0, F0);
}

void
call_check(char *name, unsigned long i, double d, void *p, int R0, int F0)
{
    /* increase buffer size */
    assert(cc < sizeof(codes) / sizeof(codes[0]));
    assert(oo < sizeof(ii) / sizeof(ii[0]));
    dd[oo] = d;
    ii[oo] = i;
    jit_prepare	  (5);
    jit_movi_p	  (R0, p);
    jit_pusharg_p (R0);
    jit_movi_p	  (R0, dd + oo);
    jit_pusharg_p (R0);
    jit_movi_p	  (R0, ii + oo);
    jit_pusharg_p (R0);
    jit_movi_i	  (R0, cc);
    jit_pusharg_i (R0);
    jit_movi_p	  (R0, name);
    jit_pusharg_p (R0);
    jit_finish	  (check);
    ++oo;
    ++cc;
}

void
call_one(char *name, void (*function)(char*, int), int R0)
{
    assert(cc < sizeof(codes) / sizeof(codes[0]));
    jit_prepare	 (2);
    jit_movi_i	 (R0, cc);
    jit_pusharg_i(R0);
    jit_movi_p	 (R0, name);
    jit_pusharg_p(R0);
    jit_finish	 (function);
    ++cc;
}

enum {
    STXI,	MOVXI,
#if sign_extend
    XI_C,	XI_UC,
    XI_S,	XI_US,
    XI_I,	XI_UI,
#endif
#if float_convert
    XI_F,	XI_D,
#endif
    STXR,	MOVXR,
#if sign_extend
    XR_C,	XR_UC,
    XR_S,	XR_US,
    XR_I,	XR_UI,
#endif
#if float_convert
    XR_F,	XR_D,
#endif
    STR,	MOVR,
#if sign_extend
    R_C,	R_UC,
    R_S,	R_US,
    R_I,	R_UI,
#endif
#if float_convert
    R_F,	R_D,
#endif
    LAST_ONE,
};

static char *names[] = {
    "stxi",	"movxi",
#if sign_extend
    "xi_c",	"xi_uc",
    "xi_s",	"xi_us",
    "xi_i",	"xi_ui",
#endif
#if float_convert
    "xi_f",	"xi_d",
#endif
    "stxr",	"movxr",
#if sign_extend
    "xr_c",	"xr_uc",
    "xr_s",	"xr_us",
    "xr_i",	"xr_ui",
#endif
#if float_convert
    "xr_f",	"xr_d",
#endif
    "str",	"movr",
#if sign_extend
    "x_c",	"x_uc",
    "x_s",	"x_us",
    "x_i",	"x_ui",
#endif
#if float_convert
    "x_f",	"x_d",
#endif
};

void
expand(types_t *ta, types_t *tb, unsigned long i, double d, void *p,
       int V0, int V1, int R0, int R1, int F0)
{
#define record_code()							\
    codes[cc] = (char *)jit_get_label()
#define record_length()							\
    lengths[cc] = (char *)jit_get_label() - (char *)codes[cc]
#define record(something)						\
    do {								\
	record_code();							\
	something;							\
	record_length();						\
    } while (0)
    record(	stxi	(ta, i, d, p,		V0, R0, F0));
    call_check		(names[STXI], i, d, p,	R0, F0);
    record(	movxi	(ta, tb,		V0, V1, R0, F0));
    call_one		(names[MOVXI], compare,	R0);
#if sign_extend
    record(	xi_c	(ta, V0, R0));
    call_one		(names[XI_C], check_c,	R0);
    record(	xi_uc	(ta, V0, R0));
    call_one		(names[XI_UC], check_uc,R0);
    record(	xi_s	(ta, V0, R0));
    call_one		(names[XI_S], check_s,	R0);
    record(	xi_us	(ta, V0, R0));
    call_one		(names[XI_US], check_us,R0);
    record(	xi_i	(ta, V0, R0));
    call_one		(names[XI_I], check_i,	R0);
    record(	xi_ui	(ta, V0, R0));
    call_one		(names[XI_UI], check_ui,R0);
#endif
#if float_convert
    record(	xi_f	(ta, V0, F0));
    call_one		(names[XI_F], check_f,	R0);
    record(	xi_d	(ta, V0, F0));
    call_one		(names[XI_D], check_d,	R0);
#endif

    record(	stxr	(ta, i, d, p,		V0, R0, R1, F0));
    call_check		(names[STXR], i, d, p,	R0, F0);
    record(	movxr	(ta, tb,		V0, V1, R0, R1, F0));
    call_one		(names[MOVXR], compare,	R0);
#if sign_extend
    record(	xr_c	(ta, V0, R0, R1));
    call_one		(names[XR_C], check_c,	R0);
    record(	xr_uc	(ta, V0, R0, R1));
    call_one		(names[XR_UC], check_uc,	R0);
    record(	xr_s	(ta, V0, R0, R1));
    call_one		(names[XR_S], check_s,	R0);
    record(	xr_us	(ta, V0, R0, R1));
    call_one		(names[XR_US], check_us,	R0);
    record(	xr_i	(ta, V0, R0, R1));
    call_one		(names[XR_I], check_i,	R0);
    record(	xr_ui	(ta, V0, R0, R1));
    call_one		(names[XR_UI], check_ui,	R0);
#endif
#if float_convert
    record(	xr_f	(ta, V0, F0, R0));
    call_one		(names[XR_F], check_f,	R0);
    record(	xr_d	(ta, V0, F0, R0));
    call_one		(names[XR_D], check_d,	R0);
#endif

    record(	str	(ta, i, d, p,		V0, R0, F0));
    call_check		(names[STR], i, d, p,	R0, F0);
    record(	movr	(ta, tb,		V0, V1, R0, F0));
    call_one		(names[MOVR], compare,	R0);
#if sign_extend
    record(	r_c	(ta, V0, R0));
    call_one		(names[R_C], check_c,	R0);
    record(	r_uc	(ta, V0, R0));
    call_one		(names[R_UC], check_uc,	R0);
    record(	r_s	(ta, V0, R0));
    call_one		(names[R_S], check_s,	R0);
    record(	r_us	(ta, V0, R0));
    call_one		(names[R_US], check_us,	R0);
    record(	r_i	(ta, V0, R0));
    call_one		(names[R_I], check_i,	R0);
    record(	r_ui	(ta, V0, R0));
    call_one		(names[R_UI], check_ui,	R0);
#endif
#if float_convert
    record(	r_f	(ta, V0, F0));
    call_one		(names[R_F], check_f,	R0);
    record(	r_d	(ta, V0, F0));
    call_one		(names[R_D], check_d,	R0);
#endif
}

void
test(int V0, int V1, int R0, int R1, int F0)
{
    oo = cc = 0;
    jit_set_ip(code);
    jit_prolog(0);
    expand(&t0, &t1, ULONG_VALUE, 0.5, &t0, V0, V1, R0, R1, F0);
    expand(&t0, &t1, SLONG_VALUE, -0.5, &t0, V0, V1, R0, R1, F0);
    expand(&t0, &t1, BLONG_VALUE, M_PI, &t1, V0, V1, R0, R1, F0);
    expand(&t0, &t1, FLONG_VALUE, 3.40282347e+38, (void *)0xdeadbeef,
	   V0, V1, R0, R1, F0);
    jit_ret();

    /* increase buffer size */
    assert((char *)jit_get_label() - (char *)code < sizeof(code));

    jit_flush_code(code, jit_get_ip().ptr);
    pvv = (pvv_t)code;
    (*pvv)();
}

void
segv_handler(int unused, siginfo_t *info, void *also_unused)
{
    ip = __builtin_return_address(0);
    addr = info->si_addr;
    siglongjmp(env, 1);
}

int
main(int argc, char *argv[])
{
    sigset_t		set;
    int		 R[6] = {
	JIT_V0,		JIT_V1,		JIT_V2,
	JIT_R0,		JIT_R1,		JIT_R2
    };
    int		 F[6] = {
	JIT_FPR0,	JIT_FPR1,	JIT_FPR2,
	JIT_FPR3,	JIT_FPR4,	JIT_FPR5
    };
    char	*r[6] = { "V0", "V1", "V2", "R0", "R1", "R2" };
    char	*f[6] = { "F0", "F1", "F2", "F3", "F4", "F5" };
    int		 V0, V1, R0, R1, FPR;

    progname = argv[0];

    act.sa_sigaction = segv_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &act, NULL);
    if (sigsetjmp(env, 1)) {
	int	offset;

	printf("SIGSEGV: __builtin_return_address(0) = %p - info->si_addr = %p\n", ip, addr);
	for (offset = 0; offset < LAST_ONE; offset++) {
	    printf("%s...\n", names[offset]);
	    /* disassemble(codes[offset], lengths[offset]); */
	}
	fflush(stderr);
	abort();
    }

    for (V0 = 0; V0 < 6; V0++) {
	for (V1 = 0; V1 < 6; V1++) {
	    if (V1 == V0)
		continue;
	    for (R0 = 0; R0 < 6; R0++) {
		if (R0 == V0 || R0 == V1)
		    continue;
		for (R1 = 0; R1 < 6; R1++) {
		    if (R1 == R0 || R1 == V0 || R1 == V1)
			continue;
		    for (FPR = 0; FPR < 6; FPR++) {
#if 0
			printf("%s %s %s %s %s\n",
			       r[V0], r[V1], r[R0], r[R1], f[FPR]);
#endif
			test(R[V0], R[V1], R[R0], R[R1], F[FPR]);
		    }
		}
	    }
	}
    }

    return (0);
}
