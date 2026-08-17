/* NPB MG -- generation-double / solve-longdouble. CLASS S.
   RNG/zran3/power/bubble = double (identical RHS location selection).
   solver (u,r,psinv,resid,rprj3,interp,norm2u3,mg3P) = long double.
   v = long double RHS (holds only +-1/0, exact); filled by copying double vd. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))
double randlc(double*,double);
void vranlc(int,double*,double,double[]);
void timer_clear(int);void timer_start(int);void timer_stop(int);long double timer_read(int);
void c_print_results(char*,char,int,int,int,int,int,long double,long double,char*,int,char*,char*,char*,char*,char*,char*,char*,char*,char*);

#define NX_DEFAULT 32
#define NY_DEFAULT 32
#define NZ_DEFAULT 32
#define NIT_DEFAULT 4
#define LM 5
#define LT_DEFAULT 5
#define DEBUG_DEFAULT 0
#define NDIM1 5
#define NDIM2 5
#define NDIM3 5
#define ONE 1
#define NPBVERSION "3.0-standalone"
#define COMPILETIME "generated"
#define CS1 "cc"
#define CS2 "cc"
#define CS3 "-lm"
#define CS4 "-I."
#define CS5 "-O3"
#define CS6 "-O3"
#define CS7 "randdp"

/*--------------------------------------------------------------------
c  Parameter lm (declared and set in "npbparams.h") is the log-base2 of 
c  the edge size max for the partition on a given node, so must be changed 
c  either to save space (if running a small case) or made bigger for larger 
c  cases, for example, 512^3. Thus lm=7 means that the largest dimension 
c  of a partition that can be solved on a node is 2^7 = 128. lm is set 
c  automatically in npbparams.h
c  Parameters ndim1, ndim2, ndim3 are the local problem dimensions. 
c-------------------------------------------------------------------*/

/* parameters */
/* actual dimension including ghost cells for communications */
#define	NM	(2+(2<<(LM-1)))
/* size of rhs array */
#define	NV	(2+(2<<(NDIM1-1))*(2+(2<<(NDIM2-1)))*(2+(2<<(NDIM3-1))))
/* size of residual array */
#define	NR	((8*(NV+(NM*NM)+5*NM+7*LM))/7)
/* size of communication buffer */
#define	NM2	(2*NM*NM)
/* maximum number of levels */
#define	MAXLEVEL	11

/*---------------------------------------------------------------------*/
/* common /mg3/ */
static int nx[MAXLEVEL+1], ny[MAXLEVEL+1], nz[MAXLEVEL+1];
/* common /ClassType/ */
static char Class;
/* common /my_debug/ */
static int debug_vec[8];
/* common /fap/ */
/*static int ir[MAXLEVEL], m1[MAXLEVEL], m2[MAXLEVEL], m3[MAXLEVEL];*/
static int m1[MAXLEVEL+1], m2[MAXLEVEL+1], m3[MAXLEVEL+1];
static int lt, lb;

/*c---------------------------------------------------------------------
c  Set at m=1024, can handle cases up to 1024^3 case
c---------------------------------------------------------------------*/
#define	M	1037

/* common /buffer/ */
/*static double buff[4][NM2];*/


#define r23 (0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5)
#define r46 (r23*r23)
#define t23 (2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0*2.0)
#define t46 (t23*t23)
double randlc(double *x,double a){double t1,t2,t3,t4,a1,a2,x1,x2,z;
 t1=r23*a;a1=(int)t1;a2=a-t23*a1;t1=r23*(*x);x1=(int)t1;x2=(*x)-t23*x1;
 t1=a1*x2+a2*x1;t2=(int)(r23*t1);z=t1-t23*t2;t3=t23*z+a2*x2;t4=(int)(r46*t3);
 (*x)=t3-t46*t4;return(r46*(*x));}
void vranlc(int n,double *xs,double a,double y[]){int i;double x,t1,t2,t3,t4,a1,a2,x1,x2,z;
 t1=r23*a;a1=(int)t1;a2=a-t23*a1;x=*xs;for(i=1;i<=n;i++){t1=r23*x;x1=(int)t1;x2=x-t23*x1;
 t1=a1*x2+a2*x1;t2=(int)(r23*t1);z=t1-t23*t2;t3=t23*z+a2*x2;t4=(int)(r46*t3);x=t3-t46*t4;
 y[i]=r46*x;}*xs=x;}
/* double helpers for zran3 (operates on double vd) */
static void zero3d(double ***z,int n1,int n2,int n3){int i1,i2,i3;
 for(i3=0;i3<n3;i3++)for(i2=0;i2<n2;i2++)for(i1=0;i1<n1;i1++)z[i3][i2][i1]=0.0;}
static void comm3d(double ***u,int n1,int n2,int n3,int kk){int i1,i2,i3;
 for(i3=1;i3<n3-1;i3++){for(i2=1;i2<n2-1;i2++){u[i3][i2][0]=u[i3][i2][n1-2];u[i3][i2][n1-1]=u[i3][i2][1];}
 for(i1=0;i1<n1;i1++){u[i3][0][i1]=u[i3][n2-2][i1];u[i3][n2-1][i1]=u[i3][1][i1];}}
 for(i2=0;i2<n2;i2++)for(i1=0;i1<n1;i1++){u[0][i2][i1]=u[n3-2][i2][i1];u[n3-1][i2][i1]=u[1][i2][i1];}}

/* MG core */
/*--------------------------------------------------------------------
  
  NAS Parallel Benchmarks 3.0 structured OpenMP C versions - MG

  This benchmark is an OpenMP C version of the NPB MG code.
  
  The OpenMP C 2.3 versions are derived by RWCP from the serial Fortran versions 
  in "NPB 2.3-serial" developed by NAS. 3.0 translation is performed by the UVSQ.

  Permission to use, copy, distribute and modify this software for any
  purpose with or without fee is hereby granted.
  This software is provided "as is" without express or implied warranty.

  Information on OpenMP activities at RWCP is available at:

           http://pdplab.trc.rwcp.or.jp/pdperf/Omni/
  
  Information on NAS Parallel Benchmarks 2.3 is available at:
  
           http://www.nas.nasa.gov/NAS/NPB/

--------------------------------------------------------------------*/
/*--------------------------------------------------------------------

  Authors: E. Barszcz
           P. Frederickson
           A. Woo
           M. Yarrow

  OpenMP C version: S. Satoh

  3.0 structure translation: F. Conti
  
--------------------------------------------------------------------*/

/* parameters */
#define T_BENCH	1
#define	T_INIT	2

/* global variables */
/* common /grid/ */
static int is1, is2, is3, ie1, ie2, ie3;

/* functions prototypes */
static void setup(int *n1, int *n2, int *n3, int lt);
static void mg3P(long double ****u, long double ***v, long double ****r, long double a[4],
		 long double c[4], int n1, int n2, int n3, int k);
static void psinv( long double ***r, long double ***u, int n1, int n2, int n3,
		   long double c[4], int k);
static void resid( long double ***u, long double ***v, long double ***r,
		   int n1, int n2, int n3, long double a[4], int k );
static void rprj3( long double ***r, int m1k, int m2k, int m3k,
		   long double ***s, int m1j, int m2j, int m3j, int k );
static void interp( long double ***z, int mm1, int mm2, int mm3,
		    long double ***u, int n1, int n2, int n3, int k );
static void norm2u3(long double ***r, int n1, int n2, int n3,
		    long double *rnm2, long double *rnmu, int nx, int ny, int nz);
static void rep_nrm(long double ***u, int n1, int n2, int n3,
		    char *title, int kk);
static void comm3(long double ***u, int n1, int n2, int n3, int kk);
static void zran3(double ***z, int n1, int n2, int n3, int nx, int ny, int k);
static void showall(long double ***z, int n1, int n2, int n3);
static double power( double a, int n );
static void bubble( double ten[M][2], int j1[M][2], int j2[M][2],
		    int j3[M][2], int m, int ind );
static void zero3(long double ***z, int n1, int n2, int n3);
static void nonzero(long double ***z, int n1, int n2, int n3);

/*--------------------------------------------------------------------
      program mg
c-------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

/*-------------------------------------------------------------------------
c k is the current level. It is passed down through subroutine args
c and is NOT global. it is the current iteration
c------------------------------------------------------------------------*/

    int k, it;
    long double t, tinit, mflops;
    int nthreads = 1;

/*-------------------------------------------------------------------------
c These arrays are in common because they are quite large
c and probably shouldn't be allocated on the stack. They
c are always passed as subroutine args. 
c------------------------------------------------------------------------*/
    
    long double ****u, ***v, ****r;
    double ***vd;
    long double a[4], c[4];

    long double rnm2, rnmu;
    long double epsilon = 1.0e-8L;
    int n1, n2, n3, nit;
    long double verify_value;
    boolean verified;

    int i, j, l;
    FILE *fp;

    timer_clear(T_BENCH);
    timer_clear(T_INIT);

    timer_start(T_INIT);

/*----------------------------------------------------------------------
c Read in and broadcast input data
c---------------------------------------------------------------------*/

    printf("\n\n NAS Parallel Benchmarks 3.0 structured OpenMP C version"
	   " - MG Benchmark\n\n");

    fp = fopen("mg.input", "r");
    if (fp != NULL) {
	printf(" Reading from input file mg.input\n");
	fscanf(fp, "%d", &lt);
	while(fgetc(fp) != '\n');
	fscanf(fp, "%d%d%d", &nx[lt], &ny[lt], &nz[lt]);
	while(fgetc(fp) != '\n');
	fscanf(fp, "%d", &nit);
	while(fgetc(fp) != '\n');
	for (i = 0; i <= 7; i++) {
	    fscanf(fp, "%d", &debug_vec[i]);
	}
	fclose(fp);
    } else {
	printf(" No input file. Using compiled defaults\n");
    
	lt = LT_DEFAULT;
	nit = NIT_DEFAULT;
	nx[lt] = NX_DEFAULT;
	ny[lt] = NY_DEFAULT;
	nz[lt] = NZ_DEFAULT;

	for (i = 0; i <= 7; i++) {
	    debug_vec[i] = DEBUG_DEFAULT;
	}
    }

    if ( (nx[lt] != ny[lt]) || (nx[lt] != nz[lt]) ) {
	Class = 'U';
    } else if( nx[lt] == 32 && nit == 4 ) {
	Class = 'S';
    } else if( nx[lt] == 64 && nit == 40 ) {
	Class = 'W';
    } else if( nx[lt] == 256 && nit == 20 ) {
	Class = 'B';
    } else if( nx[lt] == 512 && nit == 20 ) {
	Class = 'C';
    } else if( nx[lt] == 256 && nit == 4 ) {
	Class = 'A';
    } else {
	Class = 'U';
    }

/*--------------------------------------------------------------------
c  Use these for debug info:
c---------------------------------------------------------------------
c     debug_vec(0) = 1 !=> report all norms
c     debug_vec(1) = 1 !=> some setup information
c     debug_vec(1) = 2 !=> more setup information
c     debug_vec(2) = k => at level k or below, show result of resid
c     debug_vec(3) = k => at level k or below, show result of psinv
c     debug_vec(4) = k => at level k or below, show result of rprj
c     debug_vec(5) = k => at level k or below, show result of interp
c     debug_vec(6) = 1 => (unused)
c     debug_vec(7) = 1 => (unused)
c-------------------------------------------------------------------*/

    a[0] = -8.0L/3.0L;
    a[1] =  0.0L;
    a[2] =  1.0L/6.0L;
    a[3] =  1.0L/12.0L;

    if (Class == 'A' || Class == 'S' || Class =='W') {
/*--------------------------------------------------------------------
c     Coefficients for the S(a) smoother
c-------------------------------------------------------------------*/
	c[0] =  -3.0L/8.0L;
	c[1] =  1.0L/32.0L;
	c[2] =  -1.0L/64.0L;
	c[3] =   0.0L;
    } else {
/*--------------------------------------------------------------------
c     Coefficients for the S(b) smoother
c-------------------------------------------------------------------*/
	c[0] =  -3.0L/17.0L;
	c[1] =  1.0L/33.0L;
	c[2] =  -1.0L/61.0L;
	c[3] =   0.0L;
    }
    
    lb = 1;

    setup(&n1,&n2,&n3,lt);
      
    u = (long double ****)malloc((lt+1)*sizeof(long double ***));
    for (l = lt; l >=1; l--) {
	u[l] = (long double ***)malloc(m3[l]*sizeof(long double **));
	for (k = 0; k < m3[l]; k++) {
	    u[l][k] = (long double **)malloc(m2[l]*sizeof(long double *));
	    for (j = 0; j < m2[l]; j++) {
		u[l][k][j] = (long double *)malloc(m1[l]*sizeof(long double));
	    }
	}
    }
    v = (long double ***)malloc(m3[lt]*sizeof(long double **));
    for (k = 0; k < m3[lt]; k++) {
	v[k] = (long double **)malloc(m2[lt]*sizeof(long double *));
	for (j = 0; j < m2[lt]; j++) {
	    v[k][j] = (long double *)malloc(m1[lt]*sizeof(long double));
	}
    }
    vd = (double ***)malloc(m3[lt]*sizeof(double **));
    for (k = 0; k < m3[lt]; k++) {
	vd[k] = (double **)malloc(m2[lt]*sizeof(double *));
	for (j = 0; j < m2[lt]; j++) {
	    vd[k][j] = (double *)malloc(m1[lt]*sizeof(double));
	}
    }
    r = (long double ****)malloc((lt+1)*sizeof(long double ***));
    for (l = lt; l >=1; l--) {
	r[l] = (long double ***)malloc(m3[l]*sizeof(long double **));
	for (k = 0; k < m3[l]; k++) {
	    r[l][k] = (long double **)malloc(m2[l]*sizeof(long double *));
	    for (j = 0; j < m2[l]; j++) {
		r[l][k][j] = (long double *)malloc(m1[l]*sizeof(long double));
	    }
	}
    }

    zero3(u[lt],n1,n2,n3);
    zran3(vd,n1,n2,n3,nx[lt],ny[lt],lt);
    { int _a,_b,_c; for(_a=0;_a<m3[lt];_a++)for(_b=0;_b<m2[lt];_b++)for(_c=0;_c<m1[lt];_c++) v[_a][_b][_c]=(long double)vd[_a][_b][_c]; }

    norm2u3(v,n1,n2,n3,&rnm2,&rnmu,nx[lt],ny[lt],nz[lt]);

/*    printf("\n norms of random v are\n");
    printf(" %4d%19.12Le%19.12Le\n", 0, rnm2, rnmu);
    printf(" about to evaluate resid, k= %d\n", lt);*/

    printf(" Size: %3dx%3dx%3d (class %1c)\n",
	   nx[lt], ny[lt], nz[lt], Class);
    printf(" Iterations: %3d\n", nit);

    resid(u[lt],v,r[lt],n1,n2,n3,a,lt);
    norm2u3(r[lt],n1,n2,n3,&rnm2,&rnmu,nx[lt],ny[lt],nz[lt]);

/*c---------------------------------------------------------------------
c     One iteration for startup
c---------------------------------------------------------------------*/
    mg3P(u,v,r,a,c,n1,n2,n3,lt);
    resid(u[lt],v,r[lt],n1,n2,n3,a,lt);

    setup(&n1,&n2,&n3,lt);

    zero3(u[lt],n1,n2,n3); 

    zran3(vd,n1,n2,n3,nx[lt],ny[lt],lt);
    { int _a,_b,_c; for(_a=0;_a<m3[lt];_a++)for(_b=0;_b<m2[lt];_b++)for(_c=0;_c<m1[lt];_c++) v[_a][_b][_c]=(long double)vd[_a][_b][_c]; }
    

    timer_stop(T_INIT);
    timer_start(T_BENCH);

    resid(u[lt],v,r[lt],n1,n2,n3,a,lt);
    norm2u3(r[lt],n1,n2,n3,&rnm2,&rnmu,nx[lt],ny[lt],nz[lt]);

    for ( it = 1; it <= nit; it++) {
	mg3P(u,v,r,a,c,n1,n2,n3,lt);
	resid(u[lt],v,r[lt],n1,n2,n3,a,lt);
    }
    norm2u3(r[lt],n1,n2,n3,&rnm2,&rnmu,nx[lt],ny[lt],nz[lt]);

#pragma omp parallel
{   
#if defined(_OPENMP)
#pragma omp master  
  nthreads = omp_get_num_threads();
#endif /* _OPENMP */
} /* end parallel */

    timer_stop(T_BENCH);
    t = timer_read(T_BENCH);
    tinit = timer_read(T_INIT);

    verified = FALSE;
    verify_value = 0.0L;

    printf(" Initialization time: %15.3Lf seconds\n", tinit);
    printf(" Benchmark completed\n");

    if (Class != 'U') {
	if (Class == 'S') {
            verify_value = 0.530770700573e-04L;
	} else if (Class == 'W') {
            verify_value = 0.250391406439e-17L;  /* 40 iterations*/
/*				0.183103168997d-044 iterations*/
	} else if (Class == 'A') {
            verify_value = 0.2433365309e-5L;
        } else if (Class == 'B') {
            verify_value = 0.180056440132e-5L;
        } else if (Class == 'C') {
            verify_value = 0.570674826298e-06L;
	}

	if ( fabsl( rnm2 - verify_value ) <= epsilon ) {
            verified = TRUE;
	    printf(" VERIFICATION SUCCESSFUL\n");
	    printf(" L2 Norm is %20.12Le\n", rnm2);
	    printf(" Error is   %20.12Le\n", rnm2 - verify_value);
	} else {
            verified = FALSE;
	    printf(" VERIFICATION FAILED\n");
	    printf(" L2 Norm is             %20.12Le\n", rnm2);
	    printf(" The correct L2 Norm is %20.12Le\n", verify_value);
	}
    } else {
	verified = FALSE;
	printf(" Problem size unknown\n");
	printf(" NO VERIFICATION PERFORMED\n");
    }

    if ( t != 0.0L ) {
	int nn = nx[lt]*ny[lt]*nz[lt];
	mflops = 58.L*nit*nn*1.0e-6L / t;
    } else {
	mflops = 0.0L;
    }

    c_print_results("MG", Class, nx[lt], ny[lt], nz[lt], 
		    nit, nthreads, t, mflops, "          floating point", 
		    verified, NPBVERSION, COMPILETIME,
		    CS1, CS2, CS3, CS4, CS5, CS6, CS7);
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void setup(int *n1, int *n2, int *n3, int lt) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

    int k;

    for ( k = lt-1; k >= 1; k--) {
	nx[k] = nx[k+1]/2;
	ny[k] = ny[k+1]/2;
	nz[k] = nz[k+1]/2;
    }

    for (k = 1; k <= lt; k++) {
	m1[k] = nx[k]+2;
	m2[k] = nz[k]+2;
	m3[k] = ny[k]+2;
    }

    is1 = 1;
    ie1 = nx[lt];
    *n1 = nx[lt]+2;
    is2 = 1;
    ie2 = ny[lt];
    *n2 = ny[lt]+2;
    is3 = 1;
    ie3 = nz[lt];
    *n3 = nz[lt]+2;

    if (debug_vec[1] >=  1 ) {
	printf(" in setup, \n");
	printf("  lt  nx  ny  nz  n1  n2  n3 is1 is2 is3 ie1 ie2 ie3\n");
	printf("%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d\n",
	       lt,nx[lt],ny[lt],nz[lt],*n1,*n2,*n3,is1,is2,is3,ie1,ie2,ie3);
    }
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void mg3P(long double ****u, long double ***v, long double ****r, long double a[4],
		 long double c[4], int n1, int n2, int n3, int k) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     multigrid V-cycle routine
c-------------------------------------------------------------------*/

    int j;

/*--------------------------------------------------------------------
c     down cycle.
c     restrict the residual from the find grid to the coarse
c-------------------------------------------------------------------*/

    for (k = lt; k >= lb+1; k--) {
	j = k-1;
	rprj3(r[k], m1[k], m2[k], m3[k],
	      r[j], m1[j], m2[j], m3[j], k);
    }

    k = lb;
/*--------------------------------------------------------------------
c     compute an approximate solution on the coarsest grid
c-------------------------------------------------------------------*/
    zero3(u[k], m1[k], m2[k], m3[k]);
    psinv(r[k], u[k], m1[k], m2[k], m3[k], c, k);

    for (k = lb+1; k <= lt-1; k++) {
	j = k-1;
/*--------------------------------------------------------------------
c        prolongate from level k-1  to k
c-------------------------------------------------------------------*/
	zero3(u[k], m1[k], m2[k], m3[k]);
	interp(u[j], m1[j], m2[j], m3[j],
	       u[k], m1[k], m2[k], m3[k], k);
/*--------------------------------------------------------------------
c        compute residual for level k
c-------------------------------------------------------------------*/
	resid(u[k], r[k], r[k], m1[k], m2[k], m3[k], a, k);
/*--------------------------------------------------------------------
c        apply smoother
c-------------------------------------------------------------------*/
	psinv(r[k], u[k], m1[k], m2[k], m3[k], c, k);
    }

    j = lt - 1;
    k = lt;
    interp(u[j], m1[j], m2[j], m3[j], u[lt], n1, n2, n3, k);
    resid(u[lt], v, r[lt], n1, n2, n3, a, k);
    psinv(r[lt], u[lt], n1, n2, n3, c, k);
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void psinv( long double ***r, long double ***u, int n1, int n2, int n3,
		   long double c[4], int k) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     psinv applies an approximate inverse as smoother:  u = u + Cr
c
c     This  implementation costs  15A + 4M per result, where
c     A and M denote the costs of Addition and Multiplication.  
c     Presuming coefficient c(3) is zero (the NPB assumes this,
c     but it is thus not a general case), 2A + 1M may be eliminated,
c     resulting in 13A + 3M.
c     Note that this vectorizes, and is also fine for cache 
c     based machines.  
c-------------------------------------------------------------------*/

    int i3, i2, i1;
    long double r1[M], r2[M];
#pragma omp parallel for default(shared) private(i1,i2,i3,r1,r2)   
    for (i3 = 1; i3 < n3-1; i3++) {
	for (i2 = 1; i2 < n2-1; i2++) {
            for (i1 = 0; i1 < n1; i1++) {
		r1[i1] = r[i3][i2-1][i1] + r[i3][i2+1][i1]
		    + r[i3-1][i2][i1] + r[i3+1][i2][i1];
		r2[i1] = r[i3-1][i2-1][i1] + r[i3-1][i2+1][i1]
		    + r[i3+1][i2-1][i1] + r[i3+1][i2+1][i1];
	    }
            for (i1 = 1; i1 < n1-1; i1++) {
		u[i3][i2][i1] = u[i3][i2][i1]
		    + c[0] * r[i3][i2][i1]
		    + c[1] * ( r[i3][i2][i1-1] + r[i3][i2][i1+1]
			       + r1[i1] )
		    + c[2] * ( r2[i1] + r1[i1-1] + r1[i1+1] );
/*--------------------------------------------------------------------
c  Assume c(3) = 0    (Enable line below if c(3) not= 0)
c---------------------------------------------------------------------
c    >                     + c(3) * ( r2(i1-1) + r2(i1+1) )
c-------------------------------------------------------------------*/
	    }
	}
    }

/*--------------------------------------------------------------------
c     exchange boundary points
c-------------------------------------------------------------------*/
    comm3(u,n1,n2,n3,k);

    if (debug_vec[0] >= 1 ) {
	rep_nrm(u,n1,n2,n3,"   psinv",k);
    }

    if ( debug_vec[3] >= k ) {
	showall(u,n1,n2,n3);
    }
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void resid( long double ***u, long double ***v, long double ***r,
		   int n1, int n2, int n3, long double a[4], int k ) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     resid computes the residual:  r = v - Au
c
c     This  implementation costs  15A + 4M per result, where
c     A and M denote the costs of Addition (or Subtraction) and 
c     Multiplication, respectively. 
c     Presuming coefficient a(1) is zero (the NPB assumes this,
c     but it is thus not a general case), 3A + 1M may be eliminated,
c     resulting in 12A + 3M.
c     Note that this vectorizes, and is also fine for cache 
c     based machines.  
c-------------------------------------------------------------------*/

    int i3, i2, i1;
    long double u1[M], u2[M];
#pragma omp parallel for default(shared) private(i1,i2,i3,u1,u2)
    for (i3 = 1; i3 < n3-1; i3++) {
	for (i2 = 1; i2 < n2-1; i2++) {
            for (i1 = 0; i1 < n1; i1++) {
		u1[i1] = u[i3][i2-1][i1] + u[i3][i2+1][i1]
		       + u[i3-1][i2][i1] + u[i3+1][i2][i1];
		u2[i1] = u[i3-1][i2-1][i1] + u[i3-1][i2+1][i1]
		       + u[i3+1][i2-1][i1] + u[i3+1][i2+1][i1];
	    }
	    for (i1 = 1; i1 < n1-1; i1++) {
		r[i3][i2][i1] = v[i3][i2][i1]
		    - a[0] * u[i3][i2][i1]
/*--------------------------------------------------------------------
c  Assume a(1) = 0      (Enable 2 lines below if a(1) not= 0)
c---------------------------------------------------------------------
c    >                     - a(1) * ( u(i1-1,i2,i3) + u(i1+1,i2,i3)
c    >                              + u1(i1) )
c-------------------------------------------------------------------*/
		- a[2] * ( u2[i1] + u1[i1-1] + u1[i1+1] )
		      - a[3] * ( u2[i1-1] + u2[i1+1] );
	    }
	}
    }

/*--------------------------------------------------------------------
c     exchange boundary data
c--------------------------------------------------------------------*/
    comm3(r,n1,n2,n3,k);

    if (debug_vec[0] >= 1 ) {
	rep_nrm(r,n1,n2,n3,"   resid",k);
    }

    if ( debug_vec[2] >= k ) {
	showall(r,n1,n2,n3);
    }
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void rprj3( long double ***r, int m1k, int m2k, int m3k,
		   long double ***s, int m1j, int m2j, int m3j, int k ) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     rprj3 projects onto the next coarser grid, 
c     using a trilinear Finite Element projection:  s = r' = P r
c     
c     This  implementation costs  20A + 4M per result, where
c     A and M denote the costs of Addition and Multiplication.  
c     Note that this vectorizes, and is also fine for cache 
c     based machines.  
c-------------------------------------------------------------------*/

    int j3, j2, j1, i3, i2, i1, d1, d2, d3;

    long double x1[M], y1[M], x2, y2;


    if (m1k == 3) {
        d1 = 2;
    } else {
        d1 = 1;
    }

    if (m2k == 3) {
        d2 = 2;
    } else {
        d2 = 1;
    }

    if (m3k == 3) {
        d3 = 2;
    } else {
        d3 = 1;
    }
#pragma omp parallel for default(shared) private(j1,j2,j3,i1,i2,i3,x1,y1,x2,y2)
    for (j3 = 1; j3 < m3j-1; j3++) {
	i3 = 2*j3-d3;
/*C        i3 = 2*j3-1*/
	for (j2 = 1; j2 < m2j-1; j2++) {
            i2 = 2*j2-d2;
/*C           i2 = 2*j2-1*/

            for (j1 = 1; j1 < m1j; j1++) {
		i1 = 2*j1-d1;
/*C             i1 = 2*j1-1*/
		x1[i1] = r[i3+1][i2][i1] + r[i3+1][i2+2][i1]
		    + r[i3][i2+1][i1] + r[i3+2][i2+1][i1];
		y1[i1] = r[i3][i2][i1] + r[i3+2][i2][i1]
		    + r[i3][i2+2][i1] + r[i3+2][i2+2][i1];
	    }

            for (j1 = 1; j1 < m1j-1; j1++) {
		i1 = 2*j1-d1;
/*C             i1 = 2*j1-1*/
		y2 = r[i3][i2][i1+1] + r[i3+2][i2][i1+1]
		    + r[i3][i2+2][i1+1] + r[i3+2][i2+2][i1+1];
		x2 = r[i3+1][i2][i1+1] + r[i3+1][i2+2][i1+1]
		    + r[i3][i2+1][i1+1] + r[i3+2][i2+1][i1+1];
		s[j3][j2][j1] =
		    0.5L * r[i3+1][i2+1][i1+1]
		    + 0.25L * ( r[i3+1][i2+1][i1] + r[i3+1][i2+1][i1+2] + x2)
		    + 0.125L * ( x1[i1] + x1[i1+2] + y2)
		    + 0.0625L * ( y1[i1] + y1[i1+2] );
	    }
	}
    }
    comm3(s,m1j,m2j,m3j,k-1);

    if (debug_vec[0] >= 1 ) {
	rep_nrm(s,m1j,m2j,m3j,"   rprj3",k-1);
    }

    if (debug_vec[4] >= k ) {
	showall(s,m1j,m2j,m3j);
    }
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void interp( long double ***z, int mm1, int mm2, int mm3,
		    long double ***u, int n1, int n2, int n3, int k ) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     interp adds the trilinear interpolation of the correction
c     from the coarser grid to the current approximation:  u = u + Qu'
c     
c     Observe that this  implementation costs  16A + 4M, where
c     A and M denote the costs of Addition and Multiplication.  
c     Note that this vectorizes, and is also fine for cache 
c     based machines.  Vector machines may get slightly better 
c     performance however, with 8 separate "do i1" loops, rather than 4.
c-------------------------------------------------------------------*/

    int i3, i2, i1, d1, d2, d3, t1, t2, t3;

/*
c note that m = 1037 in globals.h but for this only need to be
c 535 to handle up to 1024^3
c      integer m
c      parameter( m=535 )
*/
    long double z1[M], z2[M], z3[M];

    if ( n1 != 3 && n2 != 3 && n3 != 3 ) {
#pragma omp parallel for default(shared) private(i1,i2,i3,z1,z2,z3)
	for (i3 = 0; i3 < mm3-1; i3++) {
            for (i2 = 0; i2 < mm2-1; i2++) {
		for (i1 = 0; i1 < mm1; i1++) {
		    z1[i1] = z[i3][i2+1][i1] + z[i3][i2][i1];
		    z2[i1] = z[i3+1][i2][i1] + z[i3][i2][i1];
		    z3[i1] = z[i3+1][i2+1][i1] + z[i3+1][i2][i1] + z1[i1];
		}
		for (i1 = 0; i1 < mm1-1; i1++) {
		    u[2*i3][2*i2][2*i1] = u[2*i3][2*i2][2*i1]
			+z[i3][i2][i1];
		    u[2*i3][2*i2][2*i1+1] = u[2*i3][2*i2][2*i1+1]
			+0.5L*(z[i3][i2][i1+1]+z[i3][i2][i1]);
		}
		for (i1 = 0; i1 < mm1-1; i1++) {
		    u[2*i3][2*i2+1][2*i1] = u[2*i3][2*i2+1][2*i1]
			+0.5L * z1[i1];
		    u[2*i3][2*i2+1][2*i1+1] = u[2*i3][2*i2+1][2*i1+1]
			+0.25L*( z1[i1] + z1[i1+1] );
		}
		for (i1 = 0; i1 < mm1-1; i1++) {
		    u[2*i3+1][2*i2][2*i1] = u[2*i3+1][2*i2][2*i1]
			+0.5L * z2[i1];
		    u[2*i3+1][2*i2][2*i1+1] = u[2*i3+1][2*i2][2*i1+1]
			+0.25L*( z2[i1] + z2[i1+1] );
		}
		for (i1 = 0; i1 < mm1-1; i1++) {
		    u[2*i3+1][2*i2+1][2*i1] = u[2*i3+1][2*i2+1][2*i1]
			+0.25L* z3[i1];
		    u[2*i3+1][2*i2+1][2*i1+1] = u[2*i3+1][2*i2+1][2*i1+1]
			+0.125L*( z3[i1] + z3[i1+1] );
		}
	    }
	}
    } else {
	if (n1 == 3) {
            d1 = 2;
            t1 = 1;
	} else {
            d1 = 1;
            t1 = 0;
	}
         
	if (n2 == 3) {
            d2 = 2;
            t2 = 1;
	} else {
            d2 = 1;
            t2 = 0;
	}
         
	if (n3 == 3) {
            d3 = 2;
            t3 = 1;
	} else {
            d3 = 1;
            t3 = 0;
	}
         
#pragma omp parallel default(shared) private(i1,i2,i3)
    {
#pragma omp for
	for ( i3 = d3; i3 <= mm3-1; i3++) {
            for ( i2 = d2; i2 <= mm2-1; i2++) {
		for ( i1 = d1; i1 <= mm1-1; i1++) {
		    u[2*i3-d3-1][2*i2-d2-1][2*i1-d1-1] =
			u[2*i3-d3-1][2*i2-d2-1][2*i1-d1-1]
			+z[i3-1][i2-1][i1-1];
		}
		for ( i1 = 1; i1 <= mm1-1; i1++) {
		    u[2*i3-d3-1][2*i2-d2-1][2*i1-t1-1] =
			u[2*i3-d3-1][2*i2-d2-1][2*i1-t1-1]
			+0.5L*(z[i3-1][i2-1][i1]+z[i3-1][i2-1][i1-1]);
		}
	    }
            for ( i2 = 1; i2 <= mm2-1; i2++) {
		for ( i1 = d1; i1 <= mm1-1; i1++) {
		    u[2*i3-d3-1][2*i2-t2-1][2*i1-d1-1] =
			u[2*i3-d3-1][2*i2-t2-1][2*i1-d1-1]
			+0.5L*(z[i3-1][i2][i1-1]+z[i3-1][i2-1][i1-1]);
		}
		for ( i1 = 1; i1 <= mm1-1; i1++) {
		    u[2*i3-d3-1][2*i2-t2-1][2*i1-t1-1] =
			u[2*i3-d3-1][2*i2-t2-1][2*i1-t1-1]
			+0.25L*(z[i3-1][i2][i1]+z[i3-1][i2-1][i1]
			       +z[i3-1][i2][i1-1]+z[i3-1][i2-1][i1-1]);
		}
	    }
	}
#pragma omp for nowait
	for ( i3 = 1; i3 <= mm3-1; i3++) {
            for ( i2 = d2; i2 <= mm2-1; i2++) {
		for ( i1 = d1; i1 <= mm1-1; i1++) {
		    u[2*i3-t3-1][2*i2-d2-1][2*i1-d1-1] =
			u[2*i3-t3-1][2*i2-d2-1][2*i1-d1-1]
			+0.5L*(z[i3][i2-1][i1-1]+z[i3-1][i2-1][i1-1]);
		}
		for ( i1 = 1; i1 <= mm1-1; i1++) {
		    u[2*i3-t3-1][2*i2-d2-1][2*i1-t1-1] =
			u[2*i3-t3-1][2*i2-d2-1][2*i1-t1-1]
			+0.25L*(z[i3][i2-1][i1]+z[i3][i2-1][i1-1]
			       +z[i3-1][i2-1][i1]+z[i3-1][i2-1][i1-1]);
		}
	    }
	    for ( i2 = 1; i2 <= mm2-1; i2++) {
		for ( i1 = d1; i1 <= mm1-1; i1++) {
		    u[2*i3-t3-1][2*i2-t2-1][2*i1-d1-1] =
			u[2*i3-t3-1][2*i2-t2-1][2*i1-d1-1]
			+0.25L*(z[i3][i2][i1-1]+z[i3][i2-1][i1-1]
			       +z[i3-1][i2][i1-1]+z[i3-1][i2-1][i1-1]);
		}
		for ( i1 = 1; i1 <= mm1-1; i1++) {
		    u[2*i3-t3-1][2*i2-t2-1][2*i1-t1-1] =
			u[2*i3-t3-1][2*i2-t2-1][2*i1-t1-1]
			+0.125L*(z[i3][i2][i1]+z[i3][i2-1][i1]
				+z[i3][i2][i1-1]+z[i3][i2-1][i1-1]
				+z[i3-1][i2][i1]+z[i3-1][i2-1][i1]
				+z[i3-1][i2][i1-1]+z[i3-1][i2-1][i1-1]);
		}
	    }
	}
    }
    }//end #pragma omp parallel
    if (debug_vec[0] >= 1 ) {
	rep_nrm(z,mm1,mm2,mm3,"z: inter",k-1);
	rep_nrm(u,n1,n2,n3,"u: inter",k);
    }

    if ( debug_vec[5] >= k ) {
	showall(z,mm1,mm2,mm3);
	showall(u,n1,n2,n3);
    }
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void norm2u3(long double ***r, int n1, int n2, int n3,
		    long double *rnm2, long double *rnmu, int nx, int ny, int nz) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     norm2u3 evaluates approximations to the L2 norm and the
c     uniform (or L-infinity or Chebyshev) norm, under the
c     assumption that the boundaries are periodic or zero.  Add the
c     boundaries in with half weight (quarter weight on the edges
c     and eighth weight at the corners) for inhomogeneous boundaries.
c-------------------------------------------------------------------*/

    long double s = 0.0L;
    int i3, i2, i1, n;
    long double a = 0.0L, tmp = 0.0L;

    n = nx*ny*nz;

#pragma omp parallel for default(shared) private(i1,i2,i3,a) reduction(+:s) reduction(max:tmp)
    for (i3 = 1; i3 < n3-1; i3++) {
	for (i2 = 1; i2 < n2-1; i2++) {
            for (i1 = 1; i1 < n1-1; i1++) {
		s = s + r[i3][i2][i1] * r[i3][i2][i1];
		a = fabsl(r[i3][i2][i1]);
		if (a > tmp) tmp = a;
	    }
	}
    }
    *rnmu = tmp;
	*rnm2 = sqrtl(s/(long double)n);
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void rep_nrm(long double ***u, int n1, int n2, int n3,
		    char *title, int kk) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     report on norm
c-------------------------------------------------------------------*/

    long double rnm2, rnmu;


    norm2u3(u,n1,n2,n3,&rnm2,&rnmu,nx[kk],ny[kk],nz[kk]);
    printf(" Level%2d in %8s: norms =%21.14Le%21.14Le\n",
	   kk, title, rnm2, rnmu);
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void comm3(long double ***u, int n1, int n2, int n3, int kk) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     comm3 organizes the communication on all borders 
c-------------------------------------------------------------------*/

    int i1, i2, i3;

    /* axis = 1 */
#pragma omp parallel default(shared) private(i1,i2,i3)
{
#pragma omp for
    for ( i3 = 1; i3 < n3-1; i3++) {
	for ( i2 = 1; i2 < n2-1; i2++) {
	    u[i3][i2][n1-1] = u[i3][i2][1];
	    u[i3][i2][0] = u[i3][i2][n1-2];
	}
//    }

    /* axis = 2 */
//#pragma omp for
//    for ( i3 = 1; i3 < n3-1; i3++) {
	for ( i1 = 0; i1 < n1; i1++) {
	    u[i3][n2-1][i1] = u[i3][1][i1];
	    u[i3][0][i1] = u[i3][n2-2][i1];
	}
    }

    /* axis = 3 */
#pragma omp for nowait
    for ( i2 = 0; i2 < n2; i2++) {
	for ( i1 = 0; i1 < n1; i1++) {
	    u[n3-1][i2][i1] = u[1][i2][i1];
	    u[0][i2][i1] = u[n3-2][i2][i1];
	}
    }
}//end #pragma omp parallel
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void zran3(double ***z, int n1, int n2, int n3, int nx, int ny, int k) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     zran3  loads +1 at ten randomly chosen points,
c     loads -1 at a different ten random points,
c     and zero elsewhere.
c-------------------------------------------------------------------*/

#define MM	10
#define	A	pow(5.0,13)
#define	X	314159265.e0    
    
    int i0, m0, m1;
    int i1, i2, i3, d1, e1, e2, e3;
    double xx, x0, x1, a1, a2, ai;

    double ten[MM][2], best;
    int i, j1[MM][2], j2[MM][2], j3[MM][2];
    int jg[4][MM][2];

    double rdummy;

    a1 = power( A, nx );
    a2 = power( A, nx*ny );

    zero3d(z,n1,n2,n3);

    i = is1-1+nx*(is2-1+ny*(is3-1));

    ai = power( A, i );
    d1 = ie1 - is1 + 1;
    e1 = ie1 - is1 + 2;
    e2 = ie2 - is2 + 2;
    e3 = ie3 - is3 + 2;
    x0 = X;
    rdummy = randlc( &x0, ai );
    
    for (i3 = 1; i3 < e3; i3++) {
	x1 = x0;
	for (i2 = 1; i2 < e2; i2++) {
            xx = x1;
            vranlc( d1, &xx, A, &(z[i3][i2][0]));
            rdummy = randlc( &x1, a1 );
	}
	rdummy = randlc( &x0, a2 );
    }

/*--------------------------------------------------------------------
c       call comm3(z,n1,n2,n3)
c       call showall(z,n1,n2,n3)
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     each processor looks for twenty candidates
c-------------------------------------------------------------------*/
    for (i = 0; i < MM; i++) {
	ten[i][1] = 0.0;
	j1[i][1] = 0;
	j2[i][1] = 0;
	j3[i][1] = 0;
	ten[i][0] = 1.0;
	j1[i][0] = 0;
	j2[i][0] = 0;
	j3[i][0] = 0;
    }
    for (i3 = 1; i3 < n3-1; i3++) {
	for (i2 = 1; i2 < n2-1; i2++) {
            for (i1 = 1; i1 < n1-1; i1++) {
		if ( z[i3][i2][i1] > ten[0][1] ) {
		    ten[0][1] = z[i3][i2][i1];
		    j1[0][1] = i1;
		    j2[0][1] = i2;
		    j3[0][1] = i3;
		    bubble( ten, j1, j2, j3, MM, 1 );
		}
		if ( z[i3][i2][i1] < ten[0][0] ) {
		    ten[0][0] = z[i3][i2][i1];
		    j1[0][0] = i1;
		    j2[0][0] = i2;
		    j3[0][0] = i3;
		    bubble( ten, j1, j2, j3, MM, 0 );
		}
	    }
	}
    }

/*--------------------------------------------------------------------
c     Now which of these are globally best?
c-------------------------------------------------------------------*/
    i1 = MM - 1;
    i0 = MM - 1;
    for (i = MM - 1 ; i >= 0; i--) {
	best = z[j3[i1][1]][j2[i1][1]][j1[i1][1]];
	if (best == z[j3[i1][1]][j2[i1][1]][j1[i1][1]]) {
            jg[0][i][1] = 0;
            jg[1][i][1] = is1 - 1 + j1[i1][1];
            jg[2][i][1] = is2 - 1 + j2[i1][1];
            jg[3][i][1] = is3 - 1 + j3[i1][1];
            i1 = i1-1;
	} else {
            jg[0][i][1] = 0;
            jg[1][i][1] = 0;
            jg[2][i][1] = 0;
            jg[3][i][1] = 0;
	}
	ten[i][1] = best;
	best = z[j3[i0][0]][j2[i0][0]][j1[i0][0]];
	if (best == z[j3[i0][0]][j2[i0][0]][j1[i0][0]]) {
            jg[0][i][0] = 0;
            jg[1][i][0] = is1 - 1 + j1[i0][0];
            jg[2][i][0] = is2 - 1 + j2[i0][0];
            jg[3][i][0] = is3 - 1 + j3[i0][0];
            i0 = i0-1;
	} else {
            jg[0][i][0] = 0;
            jg[1][i][0] = 0;
            jg[2][i][0] = 0;
            jg[3][i][0] = 0;
	}
	ten[i][0] = best;
    }
    m1 = i1+1;
    m0 = i0+1;

/*    printf(" negative charges at");
    for (i = 0; i < MM; i++) {
	if (i%5 == 0) printf("\n");
	printf(" (%3d,%3d,%3d)", jg[1][i][0], jg[2][i][0], jg[3][i][0]);
    }
    printf("\n positive charges at");
    for (i = 0; i < MM; i++) {
	if (i%5 == 0) printf("\n");
	printf(" (%3d,%3d,%3d)", jg[1][i][1], jg[2][i][1], jg[3][i][1]);
    }
    printf("\n small random numbers were\n");
    for (i = MM-1; i >= 0; i--) {
	printf(" %15.8Le", ten[i][0]);
    }
    printf("\n and they were found on processor number\n");
    for (i = MM-1; i >= 0; i--) {
	printf(" %4d", jg[0][i][0]);
    }
    printf("\n large random numbers were\n");
    for (i = MM-1; i >= 0; i--) {
	printf(" %15.8Le", ten[i][1]);
    }
    printf("\n and they were found on processor number\n");
    for (i = MM-1; i >= 0; i--) {
	printf(" %4d", jg[0][i][1]);
    }
    printf("\n");*/

#pragma omp parallel for private(i2, i1)    
    for (i3 = 0; i3 < n3; i3++) {
	for (i2 = 0; i2 < n2; i2++) {
            for (i1 = 0; i1 < n1; i1++) {
		z[i3][i2][i1] = 0.0;
	    }
	}
    }
    for (i = MM-1; i >= m0; i--) {
	z[j3[i][0]][j2[i][0]][j1[i][0]] = -1.0;
    }
    for (i = MM-1; i >= m1; i--) {
	z[j3[i][1]][j2[i][1]][j1[i][1]] = 1.0;
    } 
    comm3d(z,n1,n2,n3,k);

/*--------------------------------------------------------------------
c          call showall(z,n1,n2,n3)
c-------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void showall(long double ***z, int n1, int n2, int n3) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

    int i1,i2,i3;
    int m1, m2, m3;

    m1 = min(n1,18);
    m2 = min(n2,14);
    m3 = min(n3,18);

    printf("\n");
    for (i3 = 0; i3 < m3; i3++) {
	for (i1 = 0; i1 < m1; i1++) {
	    for (i2 = 0; i2 < m2; i2++) {
		printf("%6.3Lf", z[i3][i2][i1]);
	    }
	    printf("\n");
	}
	printf(" - - - - - - - \n");
    }
    printf("\n");
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static double power( double a, int n ) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     power  raises an integer, disguised as a double
c     precision real, to an integer power
c-------------------------------------------------------------------*/
    double aj;
    int nj;
    double rdummy;
    double power;

    power = 1.0;
    nj = n;
    aj = a;

    while (nj != 0) {
	if( (nj%2) == 1 ) rdummy =  randlc( &power, aj );
	rdummy = randlc( &aj, aj );
	nj = nj/2;
    }
    
    return (power);
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void bubble( double ten[M][2], int j1[M][2], int j2[M][2],
		    int j3[M][2], int m, int ind ) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
c     bubble        does a bubble sort in direction dir
c-------------------------------------------------------------------*/

    double temp;
    int i, j_temp;

    if ( ind == 1 ) {
	for (i = 0; i < m-1; i++) {
            if ( ten[i][ind] > ten[i+1][ind] ) {

		temp = ten[i+1][ind];
		ten[i+1][ind] = ten[i][ind];
		ten[i][ind] = temp;

		j_temp = j1[i+1][ind];
		j1[i+1][ind] = j1[i][ind];
		j1[i][ind] = j_temp;

		j_temp = j2[i+1][ind];
		j2[i+1][ind] = j2[i][ind];
		j2[i][ind] = j_temp;

		j_temp = j3[i+1][ind];
		j3[i+1][ind] = j3[i][ind];
		j3[i][ind] = j_temp;
	    } else {
		return;
	    }
	}
    } else {
	for (i = 0; i < m-1; i++) {
            if ( ten[i][ind] < ten[i+1][ind] ) {

		temp = ten[i+1][ind];
		ten[i+1][ind] = ten[i][ind];
		ten[i][ind] = temp;

		j_temp = j1[i+1][ind];
		j1[i+1][ind] = j1[i][ind];
		j1[i][ind] = j_temp;

		j_temp = j2[i+1][ind];
		j2[i+1][ind] = j2[i][ind];
		j2[i][ind] = j_temp;

		j_temp = j3[i+1][ind];
		j3[i+1][ind] = j3[i][ind];
		j3[i][ind] = j_temp;
	    } else {
		return;
	    }
	}
    }
}

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

static void zero3(long double ***z, int n1, int n2, int n3) {

/*--------------------------------------------------------------------
c-------------------------------------------------------------------*/

    int i1, i2, i3;
#pragma omp parallel for private(i1,i2,i3)
    for (i3 = 0;i3 < n3; i3++) {
	for (i2 = 0; i2 < n2; i2++) {
            for (i1 = 0; i1 < n1; i1++) {
		z[i3][i2][i1] = 0.0L;
	    }
	}
    }
}

/*---- end of program ------------------------------------------------*/

#include <sys/time.h>
static long double _wt(void){struct timeval tv;static long s=-1;gettimeofday(&tv,(void*)0);if(s<0)s=tv.tv_sec;return (long double)(tv.tv_sec-s)+(long double)1.0e-6L*(long double)tv.tv_usec;}
static long double _st[64],_el[64];
void timer_clear(int n){_el[n]=(long double)0;}
void timer_start(int n){_st[n]=_wt();}
void timer_stop(int n){long double x=_wt();_el[n]+=x-_st[n];}
long double timer_read(int n){return _el[n];}
void c_print_results(char*nm,char cl,int n1,int n2,int n3,int ni,int nt,long double t,long double mo,char*ot,int ps,char*ve,char*ct,char*cc,char*cl2,char*cli,char*ci,char*cf,char*clf,char*rd){
 printf("\n\n %s Benchmark Completed\n",nm);
 printf(" Class           =             %12c\n",cl);
 printf(" Size            =        %4dx%4dx%4d\n",n1,n2,n3);
 printf(" Iterations      =             %12d\n",ni);
 printf(" Verification    =             %12s\n",ps?"SUCCESSFUL":"UNSUCCESSFUL");
 printf(" Precision       =             %12s\n","longdouble");
 printf("\n");}
