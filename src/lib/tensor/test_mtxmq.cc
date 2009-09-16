#include <tensor/tensor.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <xmmintrin.h>

#include <tensor/mtxmq.h>

using namespace madness;


#if !HAVE_POSIX_MEMALIGN
#include <sys/errno.h>
static inline int posix_memalign(void **memptr, std::size_t alignment, std::size_t size){
  *memptr=malloc(size);
  if (*memptr) return 0;
  else return ENOMEM;
}
#elif MISSING_POSIX_MEMALIGN_PROTO
  extern "C"  int posix_memalign(void **memptr, std::size_t alignment, std::size_t size);
#endif

/*  extern "C"  int posix_memalign(void **memptr, std::size_t alignment, std::size_t size);
 */

#ifdef TIME_DGEMM
#ifndef FORTRAN_INTEGER
#define FORTRAN_INTEGER long
#endif
typedef FORTRAN_INTEGER integer;
extern "C" void dgemm_(const char *transa, const char *transb,
                   const integer *m, const integer *n, const integer *k,
                   const double *alpha, const double *a, const integer *lda,
                   const double *b, const integer *ldb, const double *beta,
                   double *c, const integer *ldc, int la, int lb);
void mTxm_dgemm(long ni, long nj, long nk, double* c, const double* a, const double*b ) {
  integer fni=ni;
  integer fnj=nj;
  integer fnk=nk;
  double one=1.0;
  dgemm_("n","t",&fnj,&fni,&fnk,&one,b,&fnj,a,&fni,&one,c,&fnj,1,1);
}

#endif

double ran()
{
  static unsigned long seed = 76521;

  seed = seed *1812433253 + 12345;

  return ((double) (seed & 0x7fffffff)) * 4.6566128752458e-10;
}

void ran_fill(int n, double *a) {
    while (n--) *a++ = ran();
}

void mTxm(long dimi, long dimj, long dimk,
          double* c, const double* a, const double* b) {
    int i, j, k;
    for (k=0; k<dimk; k++) {
        for (j=0; j<dimj; j++) {
            for (i=0; i<dimi; i++) {
                c[i*dimj+j] += a[k*dimi+i]*b[k*dimj+j];
            }
        }
    }
}

long long rdtsc() {
  long long x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}

void crap(double rate, double fastest, long long start) {
    if (rate == 0) printf("darn compiler bug %e %e %lld\n",rate,fastest,start);
}


void timer(const char* s, long ni, long nj, long nk, double *a, double *b, double *c) {
  double fastest=0.0, fastest_dgemm=0.0;

  double nflop = 2.0*ni*nj*nk;
  long loop;
  for (loop=0; loop<30; loop++) {
    double rate;
    long long start = rdtsc();
    mTxmq(ni,nj,nk,c,a,b);
    start = rdtsc() - start;
    rate = nflop/start;
    crap(rate,fastest,start);
    if (rate > fastest) fastest = rate;
  }
#ifdef TIME_DGEMM
  for (loop=0; loop<30; loop++) {
    double rate;
    long long start = rdtsc();
    mTxm_dgemm(ni,nj,nk,c,a,b);
    start = rdtsc() - start;
    rate = nflop/start;
    crap(rate,fastest_dgemm,start);
    if (rate > fastest_dgemm) fastest_dgemm = rate;
  }
#endif
  printf("%20s %3ld %3ld %3ld %8.2f %8.2f\n",s, ni,nj,nk, fastest, fastest_dgemm);
}

void trantimer(const char* s, long ni, long nj, long nk, double *a, double *b, double *c) {
  double fastest=0.0, fastest_dgemm=0.0;

  double nflop = 3.0*2.0*ni*nj*nk;
  long loop;
  for (loop=0; loop<30; loop++) {
    double rate;
    long long start = rdtsc();
    mTxmq(ni,nj,nk,c,a,b);
    mTxmq(ni,nj,nk,a,c,b);
    mTxmq(ni,nj,nk,c,a,b);
    start = rdtsc() - start;
    rate = nflop/start;
    crap(rate,fastest,start);
    if (rate > fastest) fastest = rate;
  }
#ifdef TIME_DGEMM
  for (loop=0; loop<30; loop++) {
    double rate;
    long long start = rdtsc();
    mTxm_dgemm(ni,nj,nk,c,a,b);
    mTxm_dgemm(ni,nj,nk,a,c,b);
    mTxm_dgemm(ni,nj,nk,c,a,b);
    start = rdtsc() - start;
    rate = nflop/start;
    crap(rate,fastest_dgemm,start);
    if (rate > fastest_dgemm) fastest_dgemm = rate;
  }
#endif
  printf("%20s %3ld %3ld %3ld %8.2f %8.2f\n",s, ni,nj,nk, fastest, fastest_dgemm);
}

int main() {
    const long nimax=30*30;
    const long njmax=100;
    const long nkmax=100;
    long ni, nj, nk, i, m;

    double *a, *b, *c, *d;
    posix_memalign((void **) &a, 16, nkmax*nimax*sizeof(double));
    posix_memalign((void **) &b, 16, nkmax*njmax*sizeof(double));
    posix_memalign((void **) &c, 16, nimax*njmax*sizeof(double));
    posix_memalign((void **) &d, 16, nimax*njmax*sizeof(double));

    ran_fill(nkmax*nimax, a);
    ran_fill(nkmax*njmax, b);


/*     ni = nj = nk = 2; */
/*     for (i=0; i<ni*nj; i++) d[i] = c[i] = 0.0; */
/*     mTxm (ni,nj,nk,c,a,b); */
/*     mTxmq(ni,nj,nk,d,a,b); */
/*     for (i=0; i<ni; i++) { */
/*       long j; */
/*       for (j=0; j<nj; j++) { */
/* 	printf("%2ld %2ld %.6f %.6f\n", i, j, c[i*nj+j], d[i*nj+j]); */
/*       } */
/*     } */
/*     return 0; */

    printf("Starting to test ... \n");
    for (ni=2; ni<60; ni+=2) {
        for (nj=2; nj<100; nj+=6) {
            for (nk=2; nk<100; nk+=6) {
                for (i=0; i<ni*nj; i++) d[i] = c[i] = 0.0;
                mTxm (ni,nj,nk,c,a,b);
                mTxmq(ni,nj,nk,d,a,b);
                for (i=0; i<ni*nj; i++) {
                    double err = fabs(d[i]-c[i]);
                    /* This test is sensitive to the compilation options.
                       Be sure to have the reference code above compiled
                       -msse2 -fpmath=sse if using GCC.  Otherwise, to
                       pass the test you may need to change the threshold
                       to circa 1e-13.
                    */
                    if (err > 1e-15) {
                        printf("test_mtxmq: error %ld %ld %ld %e\n",ni,nj,nk,err);
                        exit(1);
                    }
                }
            }
        }
    }
    printf("... OK!\n");

    for (ni=2; ni<60; ni+=2) timer("(m*m)T*(m*m)", ni,ni,ni,a,b,c);
    for (m=2; m<=30; m+=2) timer("(m*m,m)T*(m*m)", m*m,m,m,a,b,c);
    for (m=2; m<=30; m+=2) trantimer("tran(m,m,m)", m*m,m,m,a,b,c);
    for (m=2; m<=20; m+=2) timer("(20*20,20)T*(20,m)", 20*20,m,20,a,b,c);
    return 0;
}
