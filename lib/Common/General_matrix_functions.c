/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 *<LicenseText>
 *
 * CitcomS by Louis Moresi, Shijie Zhong, Lijie Han, Eh Tan,
 * Clint Conrad, Michael Gurnis, and Eun-seo Choi.
 * Copyright (C) 1994-2005, California Institute of Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *</LicenseText>
 * 
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <math.h>
#include <sys/types.h>
#include "element_definitions.h"
#include "global_defs.h"

#ifdef _UNICOS
#include <fortran.h>
#endif

int epsilon[4][4] = {   /* Levi-Cita epsilon */
  {0, 0, 0, 0},
  {0, 1,-1, 1},
  {0,-1, 1,-1},
  {0, 1,-1, 1} };


/*=====================================================================
  Variable dimension matrix allocation  function from numerical recipes
  Note: ANSII consistency requires some additional features !
  =====================================================================  */

double **dmatrix(nrl,nrh,ncl,nch)
     int nrl,nrh,ncl,nch;
{
  int i,nrow = nrh-nrl+1,ncol=nch-ncl+1;
  double **m;

  /* allocate pointer to rows  */
  m=(double **) malloc((nrow+1)* sizeof(double *));
  m+=1;
  m-= nrl;

  /*  allocate rows and set the pointers accordingly   */
  m[nrl] = (double *) malloc((nrow*ncol+1)* sizeof(double));
  m[nrl] += 1;
  m[nrl] -= ncl;

  for(i=nrl+1;i<=nrh;i++)
     m[i] = m[i-1] + ncol;

  return(m);		}


float **fmatrix(nrl,nrh,ncl,nch)
     int nrl,nrh,ncl,nch;
{
  int i,nrow = nrh-nrl+1,ncol=nch-ncl+1;
  float **m;

  /* allocate pointer to rows  */
  m=(float **) malloc((unsigned)((nrow+1)* sizeof(float *)));
  m+=1;
  m-= nrl;

  /*  allocate rows and set the pointers accordingly   */
  m[nrl] = (float *) malloc((unsigned)((nrow*ncol+1)* sizeof(float)));
  m[nrl] += 1;
  m[nrl] -= ncl;

  for(i=nrl+1;i<=nrh;i++)
     m[i] = m[i-1] + ncol;

  return(m);		}


void dfree_matrix(m,nrl,nrh,ncl,nch)
     double **m;
     int nrl,nrh,ncl,nch;
{
  int i;
  for(i=nrh;i>=nrl;i--)
    free((void *)(m[i] + ncl));
  free((void *) (m+nrl));
  return;
}

void ffree_matrix(m,nrl,nrh,ncl,nch)
     float **m;
     int nrl,nrh,ncl,nch;
{
  int i;
  for(i=nrh;i>=nrl;i--)
    free((void *)(m[i] + ncl));
  free((void *) (m+nrl));
  return;
}

/*=============================================================
  Functions to allocate/remove space for variable sized vector.
  =============================================================  */

double *dvector(nl,nh)
     int nl,nh;
{
  double *v;
  v=(double *) malloc((unsigned) ( nh - nl +1)* sizeof(double));
  return( v-nl );  }

float *fvector(nl,nh)
     int nl,nh;
{
  float *v;
  v=(float *) malloc((unsigned) ( nh - nl +1)* sizeof(float));
  return( v-nl );  }

void dfree_vector(v,nl,nh)
     double *v;
     int nl,nh;
{
  free((char*) (v+nl));	}

void ffree_vector(v,nl,nh)
     float *v;
     int nl,nh;
{
  free((char*) (v+nl));	}

int *sivector(nl,nh)
     int nl,nh;
{
  int *v;
  v=(int*) malloc((unsigned)(nh-nl +1) * sizeof(int));
  return (v-nl);
}

void sifree_vector(v,nl,nh)
     int *v;
     int nl,nh;
{ free((char *) (v+nl));    }



void dvcopy(E,A,B,a,b)
     struct All_variables *E;
     double **A,**B;
     int a,b;

{   int i,m;

  for (m=1;m<=E->sphere.caps_per_proc;m++)
    for(i=a;i<=b;i++)
      A[m][i] = B[m][i];

    return; }

void vcopy(A,B,a,b)
     float *A,*B;
     int a,b;

{   int i;

    for(i=a;i<=b;i++)
      A[i] = B[i];

    return; }



/*  ===========================================================
    Iterative solver also using multigrid  ........
    ===========================================================  */

int solve_del2_u(E,d0,F,acc,high_lev)
     struct All_variables *E;
     double **d0;
     double **F;
     double acc;
     int high_lev;
{
  void assemble_del2_u();
  void e_assemble_del2_u();
  void n_assemble_del2_u();
  void strip_bcs_from_residual();
  void gauss_seidel();
  void jacobi();

  double conj_grad();
  double multi_grid();
  double global_vdot();

  int count,counts,cycles,convergent,valid;
  int i, neq, gneq,m;

  double CPU_time0(),initial_time,time;
  double residual,prior_residual,r0;
  double *D1[NCS], *r[NCS], *Au[NCS];

  gneq  = E->mesh.NEQ[high_lev];
  gneq  = E->mesh.neq;
  neq  = E->lmesh.NEQ[high_lev];

  for (m=1;m<=E->sphere.caps_per_proc;m++)
      for(i=0;i<neq;i++)  {
	d0[m][i] = 0.0;
      }

  r0=residual=sqrt(global_vdot(E,F,F,high_lev)/gneq);

  prior_residual=2*residual;
  count = 0;
  initial_time=CPU_time0();

  if (!(E->control.NMULTIGRID || E->control.EMULTIGRID)) {
    cycles = E->control.v_steps_low;
    residual = conj_grad(E,d0,F,acc,&cycles,high_lev);
    valid = (residual < acc)? 1:0;
  }

  else  {

    counts =0;
    if(E->parallel.me==0) fprintf(stderr,"resi = %.6e for iter %d acc %.6e\n",residual,counts,acc);
    if(E->parallel.me==0) fprintf(E->fp,"resi = %.6e for iter %d acc %.6e\n",residual,counts,acc);
    valid = (residual < acc)?1:0;

    while (!valid) {
      residual=multi_grid(E,d0,F,acc,high_lev);
      valid = (residual < acc)?1:0;
      counts ++;
      if(E->parallel.me==0) fprintf(stderr,"resi = %.6e for iter %d acc %.6e\n",residual,counts,acc);
      if(E->parallel.me==0) fprintf(E->fp,"resi = %.6e for iter %d acc %.6e\n",residual,counts,acc);
    }

    cycles = counts;
  }


  /* Convergence check .....
     We should give it a chance to recover if it briefly diverges initially, and
     don't worry about slower convergence if it is close to the answer   */

  if((count > 0) &&
     (residual > r0*2.0)  ||
     (fabs(residual-prior_residual) < acc*0.1 && (residual > acc * 10.0))   )
    convergent=0;
  else {
    convergent=1;
    prior_residual=residual;
  }

  if(E->control.print_convergence&&E->parallel.me==0)   {
    fprintf(E->fp,"%s residual (%03d) = %.3e from %.3e to %.3e in %5.2f secs \n",
	    (convergent ? " * ":"!!!"),cycles,residual,r0,acc,CPU_time0()-initial_time);
    fflush(E->fp);
  }

  count++;


  E->control.total_iteration_cycles += count;
  E->control.total_v_solver_calls += 1;

  return(valid);
}

/* =================================
   recursive multigrid function ....
   ================================= */

double multi_grid(E,d1,F,acc,hl)
     struct All_variables *E;
     double **d1;
     double **F;
     double acc;
     int hl;  /* higher level of two */
{
    double residual,AudotAu;
    void interp_vector();
    void project_vector();
    int m,i,j,Vn,Vnmax,cycles;
    double alpha,beta;
    void jacobi();
    void gauss_seidel();
    void element_gauss_seidel();
    void e_assemble_del2_u();
    void strip_bcs_from_residual();
    void n_assemble_del2_u();

    double conj_grad(),global_vdot();

    FILE *fp;
    char filename[1000];
    int lev,ic,ulev,dlev;

    const int levmin = E->mesh.levmin;
    const int levmax = E->mesh.levmax;

    double time1,time,CPU_time0();
    double *res[MAX_LEVELS][NCS],*AU[MAX_LEVELS][NCS];
    double *vel[MAX_LEVELS][NCS],*del_vel[MAX_LEVELS][NCS];
    double *rhs[MAX_LEVELS][NCS],*fl[MAX_LEVELS][NCS];
				/* because it's recursive, need a copy at
				    each level */

    for(i=E->mesh.levmin;i<=E->mesh.levmax;i++)
      for(m=1;m<=E->sphere.caps_per_proc;m++)    {
	del_vel[i][m]=(double *)malloc((E->lmesh.NEQ[i] + 2)*sizeof(double));
	AU[i][m] = (double *)malloc((E->lmesh.NEQ[i] + 2)*sizeof(double));
	vel[i][m]=(double *)malloc((E->lmesh.NEQ[i]+2)*sizeof(double));
	res[i][m]=(double *)malloc((E->lmesh.NEQ[i]+2)*sizeof(double));
	if (i<E->mesh.levmax)
	  fl[i][m]=(double *)malloc((E->lmesh.NEQ[i]+2)*sizeof(double));
      }

    Vnmax = E->control.mg_cycle;

        /* Project residual onto all the lower levels */

    project_vector(E,levmax,F,fl[levmax-1],1);
    strip_bcs_from_residual(E,fl[levmax-1],levmax-1);
    for(lev=levmax-1;lev>levmin;lev--) {
        project_vector(E,lev,fl[lev],fl[lev-1],1);
        strip_bcs_from_residual(E,fl[lev-1],lev-1);
       }

        /* Solve for the lowest level */

/*    time=CPU_time0(); */
    cycles = E->control.v_steps_low;

    gauss_seidel(E,vel[levmin],fl[levmin],AU[levmin],acc*0.01,&cycles,levmin,0);

    for(lev=levmin+1;lev<=levmax;lev++) {
      time=CPU_time0();

                         /* Utilize coarse solution and smooth at this level */
      interp_vector(E,lev-1,vel[lev-1],vel[lev]);
      strip_bcs_from_residual(E,vel[lev],lev);

      if (lev==levmax)
        for(m=1;m<=E->sphere.caps_per_proc;m++)
          for(j=0;j<E->lmesh.NEQ[lev];j++)
             res[lev][m][j]=F[m][j];
      else
        for(m=1;m<=E->sphere.caps_per_proc;m++)
          for(j=0;j<E->lmesh.NEQ[lev];j++)
             res[lev][m][j]=fl[lev][m][j];

      for(Vn=1;Vn<=Vnmax;Vn++)   {
                                        /*    Downward stoke of the V    */
        for (dlev=lev;dlev>=levmin+1;dlev--)   {

                                      /* Pre-smoothing  */
          cycles=((dlev==levmax)?E->control.v_steps_high:E->control.down_heavy);
          ic = ((dlev==lev)?1:0);
          gauss_seidel(E,vel[dlev],res[dlev],AU[dlev],0.01,&cycles,dlev,ic);

          for(m=1;m<=E->sphere.caps_per_proc;m++)
             for(i=0;i<E->lmesh.NEQ[dlev];i++)  {
               res[dlev][m][i]  = res[dlev][m][i] - AU[dlev][m][i];
	       }

          project_vector(E,dlev,res[dlev],res[dlev-1],1);
          strip_bcs_from_residual(E,res[dlev-1],dlev-1);
          }

                                        /*    Bottom of the V    */
       cycles = E->control.v_steps_low;
       gauss_seidel(E,vel[levmin],res[levmin],AU[levmin],acc*0.01,&cycles,levmin,0);
                                        /*    Upward stoke of the V    */
        for (ulev=levmin+1;ulev<=lev;ulev++)   {
            cycles=((ulev==levmax)?E->control.v_steps_high:E->control.up_heavy);

            interp_vector(E,ulev-1,vel[ulev-1],del_vel[ulev]);
            strip_bcs_from_residual(E,del_vel[ulev],ulev);
            gauss_seidel(E,del_vel[ulev],res[ulev],AU[ulev],0.01,&cycles,ulev,1);

            AudotAu = global_vdot(E,AU[ulev],AU[ulev],ulev);
            alpha = global_vdot(E,AU[ulev],res[ulev],ulev)/AudotAu;

            for(m=1;m<=E->sphere.caps_per_proc;m++)
              for(i=0;i<E->lmesh.NEQ[ulev];i++)   {
               vel[ulev][m][i] += alpha*del_vel[ulev][m][i];
               }

            if (ulev ==levmax)
              for(m=1;m<=E->sphere.caps_per_proc;m++)
                for(i=0;i<E->lmesh.NEQ[ulev];i++)   {
                  res[ulev][m][i] -= alpha*AU[ulev][m][i];
                  }

            }
        }
      }

   for(m=1;m<=E->sphere.caps_per_proc;m++)
     for(j=0;j<E->lmesh.NEQ[levmax];j++)   {
        F[m][j]=res[levmax][m][j];
        d1[m][j]+=vel[levmax][m][j];
        }

     residual = sqrt(global_vdot(E,F,F,hl)/E->mesh.NEQ[hl]);

      for(i=E->mesh.levmin;i<=E->mesh.levmax;i++)
        for(m=1;m<=E->sphere.caps_per_proc;m++) {
	  free((double*) del_vel[i][m]);
	  free((double*) AU[i][m]);
	  free((double*) vel[i][m]);
	  free((double*) res[i][m]);
	  if (i<E->mesh.levmax)
	    free((double*) fl[i][m]);
	  }


    return(residual);
}


/*  ===========================================================
    Conjugate gradient relaxation for the matrix equation Kd = f
    Returns the residual reduction after itn iterations ...
    ===========================================================  */


double conj_grad(E,d0,F,acc,cycles,level)
     struct All_variables *E;
     double **d0;
     double **F;
     double acc;
     int *cycles;
     int level;
{
    double *r0[NCS],*r1[NCS],*r2[NCS];
    double *z0[NCS],*z1[NCS],*z2[NCS];
    double *p1[NCS],*p2[NCS];
    double *Ap[NCS];
    double *shuffle[NCS];

    int m,count,i,steps;
    double residual;
    double alpha,beta,dotprod,dotr1z1,dotr0z0;

    double CPU_time0(),time;

    void parallel_process_termination();
    void assemble_del2_u();
    void strip_bcs_from_residual();
    double global_vdot();

    const int mem_lev=E->mesh.levmax;
    const int high_neq = E->lmesh.NEQ[level];

    steps = *cycles;

    for(m=1;m<=E->sphere.caps_per_proc;m++)    {
      r0[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      r1[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      r2[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      z0[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      z1[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      p1[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      p2[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
      Ap[m] = (double *)malloc((1+E->lmesh.NEQ[mem_lev])*sizeof(double));
    }

    for(m=1;m<=E->sphere.caps_per_proc;m++)
      for(i=0;i<high_neq;i++) {
	r1[m][i] = F[m][i];
        d0[m][i] = 0.0;
	}

    residual = sqrt(global_vdot(E,r1,r1,level)/E->mesh.NEQ[level]);

    assert(residual != 0.0  /* initial residual for CG = 0.0 */);
    count = 0;

    while (((residual > acc) && (count < steps)) || count == 0)  {

    for(m=1;m<=E->sphere.caps_per_proc;m++)
      for(i=0;i<high_neq;i++)
         z1[m][i] = E->BI[level][m][i] * r1[m][i];

	dotr1z1 = global_vdot(E,r1,z1,level);

	if (0==count)
          for(m=1;m<=E->sphere.caps_per_proc;m++)
	    for(i=0;i<high_neq;i++)
	      p2[m][i] = z1[m][i];
	else {
	    assert(dotr0z0 != 0.0 /* in head of conj_grad */);
	    beta = dotr1z1/dotr0z0;
            for(m=1;m<=E->sphere.caps_per_proc;m++)
	      for(i=0;i<high_neq;i++)
		p2[m][i] = z1[m][i] + beta * p1[m][i];
	}

    dotr0z0 = dotr1z1;

	assemble_del2_u(E,p2,Ap,level,1);

	dotprod=global_vdot(E,p2,Ap,level);

	if(0.0==dotprod)
	    alpha=1.0e-3;
	else
	    alpha = dotr1z1/dotprod;

        for(m=1;m<=E->sphere.caps_per_proc;m++)
          for(i=0;i<high_neq;i++) {
	    d0[m][i] += alpha * p2[m][i];
	    r2[m][i] = r1[m][i] - alpha * Ap[m][i];
	    }

	residual = sqrt(global_vdot(E,r2,r2,level)/E->mesh.NEQ[level]);

        for(m=1;m<=E->sphere.caps_per_proc;m++)    {
	  shuffle[m] = r0[m]; r0[m] = r1[m]; r1[m] = r2[m]; r2[m] = shuffle[m];
	  shuffle[m] = z0[m]; z0[m] = z1[m]; z1[m] = shuffle[m];
	  shuffle[m] = p1[m]; p1[m] = p2[m]; p2[m] = shuffle[m];
          }

	count++;
	/* end of while-loop */

    }

    *cycles=count;

    strip_bcs_from_residual(E,d0,level);

    for(m=1;m<=E->sphere.caps_per_proc;m++)    {
      free((double*) r0[m]);
      free((double*) r1[m]);
      free((double*) r2[m]);
      free((double*) z0[m]);
      free((double*) z1[m]);
      free((double*) p1[m]);
      free((double*) p2[m]);
      free((double*) Ap[m]);
    }

    return(residual);   }


/* ========================================================================================
   An element by element version of the gauss-seidel routine. Initially this is a test
   platform, we want to know if it handles discontinuities any better than the node/equation
   versions
   =========================================================================================*/

void element_gauss_seidel(E,d0,F,Ad,acc,cycles,level,guess)
    struct All_variables *E;
    double **d0;
    double **F,**Ad;
    double acc;
    int *cycles;
    int level;
    int guess;
{
    int count,i,j,k,l,m,ns,nc,d,steps,loc;
    int p1,p2,p3,q1,q2,q3;
    int e,eq,node,node1;
    int element,eqn1,eqn2,eqn3,eqn11,eqn12,eqn13;

    void e_assemble_del2_u();
    void n_assemble_del2_u();
    void strip_bcs_from_residual();
    void exchange_id_d();

    double U1[24],AD1[24],F1[24];
    double w1,w2,w3;
    double w11,w12,w13;
    double w[24];

    double *dd[NCS],*elt_k;
    int *vis[NCS];

    const int dims=E->mesh.nsd;
    const int ends=enodes[dims];
    const int n=loc_mat_size[E->mesh.nsd];
    const int neq=E->lmesh.NEQ[level];
    const int nel=E->lmesh.NEL[level];
    const int nno=E->lmesh.NNO[level];


    steps=*cycles;

    for (m=1;m<=E->sphere.caps_per_proc;m++) {
      dd[m] = (double *)malloc((neq+1)*sizeof(double));
      vis[m] = (int *)malloc((nno+1)*sizeof(int));
    }
    elt_k=(double *)malloc((24*24)*sizeof(double));

    if(guess){
	e_assemble_del2_u(E,d0,Ad,level,1);
    }
    else {
      for (m=1;m<=E->sphere.caps_per_proc;m++)
	for(i=0;i<neq;i++)
	     Ad[m][i]=d0[m][i]=0.0;
    }

   count=0;
   while (count <= steps)      {
     for (m=1;m<=E->sphere.caps_per_proc;m++) {
	for(i=1;i<=nno;i++)
	    vis[m][i]=0;

	for(e=1;e<=nel;e++) {

	    elt_k = E->elt_k[level][m][e].k;

	    for(i=1;i<=ends;i++)  {
		node=E->IEN[level][m][e].node[i];
		p1=(i-1)*dims;
		w[p1] = w[p1+1] = w[p1+2] = 1.0;
		if(E->NODE[level][m][node] & VBX)
		    w[p1] = 0.0;
		if(E->NODE[level][m][node] & VBY)
		    w[p1+1] = 0.0;
		if(E->NODE[level][m][node] & VBZ)
		    w[p1+2] = 0.0;

	        }


	    for(i=1;i<=ends;i++)   {
		node=E->IEN[level][m][e].node[i];
		if(!vis[m][node])
		    continue;

		eqn1=E->ID[level][m][node].doff[1];
		eqn2=E->ID[level][m][node].doff[2];
                eqn3=E->ID[level][m][node].doff[3];
		p1=(i-1)*dims*n;
		p2=p1+n;
		p3=p2+n;


		/* update Au */
		for(j=1;j<=ends;j++) {
		    node1=E->IEN[level][m][e].node[j];

	            eqn11=E->ID[level][m][node1].doff[1];
		    eqn12=E->ID[level][m][node1].doff[2];
		    eqn13=E->ID[level][m][node1].doff[3];
		    q1=(j-1)*3;

                    Ad[m][eqn11] += w[q1]*(elt_k[p1+q1] * dd[m][eqn1] + elt_k[p2+q1] * dd[m][eqn2] + elt_k[p3+q1] * dd[m][eqn3]);
                    Ad[m][eqn12] += w[q1+1]*(elt_k[p1+q1+1] * dd[m][eqn1] + elt_k[p2+q1+1] * dd[m][eqn2] + elt_k[p3+q1+1] * dd[m][eqn3]);
	            Ad[m][eqn13] += w[q1+2]*(elt_k[p1+q1+2] * dd[m][eqn1] + elt_k[p2+q1+2] * dd[m][eqn2] + elt_k[p3+q1+2] * dd[m][eqn3]);

		   }
	        }


	    for(i=1;i<=ends;i++)   {
		node=E->IEN[level][m][e].node[i];
		if(vis[m][node])
		    continue;

		eqn1=E->ID[level][m][node].doff[1];
		eqn2=E->ID[level][m][node].doff[2];
		eqn3=E->ID[level][m][node].doff[3];
		p1=(i-1)*dims*n;
		p2=p1+n;
		p3=p2+n;

		/* update dd, d0 */
		d0[m][eqn1] += (dd[m][eqn1] = w[(i-1)*dims]*(F[m][eqn1]-Ad[m][eqn1])*E->BI[level][m][eqn1]);
		d0[m][eqn2] += (dd[m][eqn2] = w[(i-1)*dims+1]*(F[m][eqn2]-Ad[m][eqn2])*E->BI[level][m][eqn2]);
		d0[m][eqn3] += (dd[m][eqn3] = w[(i-1)*dims+2]*(F[m][eqn3]-Ad[m][eqn3])*E->BI[level][m][eqn3]);

		vis[m][node]=1;

		/* update Au */
		for(j=1;j<=ends;j++) {
		   node1=E->IEN[level][m][e].node[j];

		   eqn11=E->ID[level][m][node1].doff[1];
		   eqn12=E->ID[level][m][node1].doff[2];
		   eqn13=E->ID[level][m][node1].doff[3];
		   q1=(j-1)*3;
		   q2=q1+1;
		   q3=q1+2;

		   Ad[m][eqn11] += w[q1]*(elt_k[p1+q1] * dd[m][eqn1] + elt_k[p2+q1] * dd[m][eqn2] + elt_k[p3+q1] * dd[m][eqn3]);
		   Ad[m][eqn12] += w[q2]*(elt_k[p1+q2] * dd[m][eqn1] + elt_k[p2+q2] * dd[m][eqn2] + elt_k[p3+q2] * dd[m][eqn3]);
		   Ad[m][eqn13] += w[q3]*(elt_k[p1+q3] * dd[m][eqn1] + elt_k[p2+q3] * dd[m][eqn2] + elt_k[p3+q3] * dd[m][eqn3]);
		   }
	       }

	    }          /* end for el  */
	 }               /* end for m */

        exchange_id_d(E, Ad, level);
        exchange_id_d(E, d0, level);

	/* completed cycle */

	    count++;

    }

    for (m=1;m<=E->sphere.caps_per_proc;m++) {
      free((double*) dd[m]);
      free((int*) vis[m]);
    }
    free((double*) elt_k);

    return;
}

void jacobi(E,d0,F,Ad,acc,cycles,level,guess)
     struct All_variables *E;
     double **d0;
     double **F,**Ad;
     double acc;
     int *cycles;
     int level;
     int guess;
{

    int count,i,j,k,l,m,ns,steps;
    int *C;
    int eqn1,eqn2,eqn3,gneq;

    void n_assemble_del2_u();
    void exchange_id_d();

    double sum1,sum2,sum3,residual,global_vdot(),U1,U2,U3;

    double *r1[NCS];

    higher_precision *B1,*B2,*B3;


    const int dims=E->mesh.nsd;
    const int ends=enodes[dims];
    const int n=loc_mat_size[E->mesh.nsd];
    const int neq=E->lmesh.NEQ[level];
    const int num_nodes=E->lmesh.NNO[level];
    const int nox=E->lmesh.NOX[level];
    const int noz=E->lmesh.NOY[level];
    const int noy=E->lmesh.NOZ[level];
    const int max_eqn=14*dims;

    gneq = E->mesh.NEQ[level];

    steps=*cycles;

    for (m=1;m<=E->sphere.caps_per_proc;m++)
      r1[m] = (double *)malloc((E->lmesh.neq+1)*sizeof(double));

    if(guess) {
      for (m=1;m<=E->sphere.caps_per_proc;m++)
	d0[m][neq]=0.0;
      n_assemble_del2_u(E,d0,Ad,level,1);
    }
    else
      for (m=1;m<=E->sphere.caps_per_proc;m++)
	for(i=0;i<neq;i++) {
	    d0[m][i]=Ad[m][i]=0.0;
	}

    for (m=1;m<=E->sphere.caps_per_proc;m++)
      for(i=0;i<neq;i++)
        r1[m][i]=F[m][i]-Ad[m][i];


    count = 0;

   while (count < steps)   {
      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(i=1;i<=E->lmesh.NNO[level];i++)  {
	    eqn1=E->ID[level][m][i].doff[1];
	    eqn2=E->ID[level][m][i].doff[2];
	    eqn3=E->ID[level][m][i].doff[3];
            d0[m][eqn1] += r1[m][eqn1]*E->BI[level][m][eqn1];
            d0[m][eqn2] += r1[m][eqn2]*E->BI[level][m][eqn2];
            d0[m][eqn3] += r1[m][eqn3]*E->BI[level][m][eqn3];
            }

      for (m=1;m<=E->sphere.caps_per_proc;m++)
	for(i=0;i<neq;i++)
	    Ad[m][i]=0.0;

      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(i=1;i<=E->lmesh.NNO[level];i++)  {
	    eqn1=E->ID[level][m][i].doff[1];
	    eqn2=E->ID[level][m][i].doff[2];
	    eqn3=E->ID[level][m][i].doff[3];
	    U1 = d0[m][eqn1];
	    U2 = d0[m][eqn2];
	    U3 = d0[m][eqn3];

            C=E->Node_map[level][m]+(i-1)*max_eqn;
	    B1=E->Eqn_k1[level][m]+(i-1)*max_eqn;
	    B2=E->Eqn_k2[level][m]+(i-1)*max_eqn;
 	    B3=E->Eqn_k3[level][m]+(i-1)*max_eqn;

            for(j=3;j<max_eqn;j++)  {
               Ad[m][eqn1] += B1[j]*d0[m][C[j]];
               Ad[m][eqn2] += B2[j]*d0[m][C[j]];
               Ad[m][eqn3] += B3[j]*d0[m][C[j]];
               }

	    for(j=0;j<max_eqn;j++) {
		    Ad[m][C[j]]  += B1[j]*U1 +  B2[j]*U2 +  B3[j]*U3;
		}
	    }       /* end for i and m */

      exchange_id_d(E, Ad, level);

      for (m=1;m<=E->sphere.caps_per_proc;m++)
	for(i=0;i<neq;i++)
	    r1[m][i] = F[m][i] - Ad[m][i];

   /*   residual = sqrt(global_vdot(E,r1,r1,level))/gneq;

   if(E->parallel.me==0)fprintf(stderr,"residuall =%.5e for %d\n",residual,count);
*/	count++;
    }

   *cycles=count;

    for (m=1;m<=E->sphere.caps_per_proc;m++)
      free((double*) r1[m]);

    return;

    }


/* ============================================================================
   Multigrid Gauss-Seidel relaxation scheme which requires the storage of local
   information, otherwise some other method is required. NOTE this is a bit worse
   than real gauss-seidel because it relaxes all the equations for a node at one
   time (Jacobi at a node). It does the job though.
   ============================================================================ */

void gauss_seidel(E,d0,F,Ad,acc,cycles,level,guess)
     struct All_variables *E;
     double **d0;
     double **F,**Ad;
     double acc;
     int *cycles;
     int level;
     int guess;
{

    int count,i,j,k,l,m,ns,steps;
    int *C;
    int eqn1,eqn2,eqn3;

    void parallel_process_termination();
    void n_assemble_del2_u();

    void exchange_id_d();
    double U1,U2,U3,UU;
    double sor,residual,global_vdot();

    higher_precision *B1,*B2,*B3;


    const int dims=E->mesh.nsd;
    const int ends=enodes[dims];
    const int n=loc_mat_size[E->mesh.nsd];
    const int neq=E->lmesh.NEQ[level];
    const int num_nodes=E->lmesh.NNO[level];
    const int nox=E->lmesh.NOX[level];
    const int noz=E->lmesh.NOY[level];
    const int noy=E->lmesh.NOZ[level];
    const int gneq  = E->mesh.neq;
    const int max_eqn=14*dims;

    const double zeroo = 0.0;

    steps=*cycles;
    sor = 1.3;

    if(guess) {
      for (m=1;m<=E->sphere.caps_per_proc;m++)
	d0[m][neq]=0.0;
      n_assemble_del2_u(E,d0,Ad,level,1);
    }
    else
      for (m=1;m<=E->sphere.caps_per_proc;m++)
	for(i=0;i<=neq;i++) {
	    d0[m][i]=Ad[m][i]=zeroo;
	}

    count = 0;


    while (count < steps) {
      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(j=0;j<=E->lmesh.NEQ[level];j++)
          E->temp[m][j] = zeroo;

      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(i=1;i<=E->lmesh.NNO[level];i++)
          if(E->NODE[level][m][i] & OFFSIDE)   {

	    eqn1=E->ID[level][m][i].doff[1];
	    eqn2=E->ID[level][m][i].doff[2];
	    eqn3=E->ID[level][m][i].doff[3];
	    E->temp[m][eqn1] = (F[m][eqn1] - Ad[m][eqn1])*E->BI[level][m][eqn1];
	    E->temp[m][eqn2] = (F[m][eqn2] - Ad[m][eqn2])*E->BI[level][m][eqn2];
	    E->temp[m][eqn3] = (F[m][eqn3] - Ad[m][eqn3])*E->BI[level][m][eqn3];
	    E->temp1[m][eqn1] = Ad[m][eqn1];
	    E->temp1[m][eqn2] = Ad[m][eqn2];
	    E->temp1[m][eqn3] = Ad[m][eqn3];
            }

      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(i=1;i<=E->lmesh.NNO[level];i++)     {

	    eqn1=E->ID[level][m][i].doff[1];
	    eqn2=E->ID[level][m][i].doff[2];
	    eqn3=E->ID[level][m][i].doff[3];
            C=E->Node_map[level][m]+(i-1)*max_eqn;
	    B1=E->Eqn_k1[level][m]+(i-1)*max_eqn;
	    B2=E->Eqn_k2[level][m]+(i-1)*max_eqn;
 	    B3=E->Eqn_k3[level][m]+(i-1)*max_eqn;

                 /* Ad on boundaries differs after the following operation, but
                  no communications are needed yet, because boundary Ad will
                  not be used for the G-S iterations for interior nodes */

            for(j=3;j<max_eqn;j++)  {
                 UU = E->temp[m][C[j]];
                 Ad[m][eqn1] += B1[j]*UU;
                 Ad[m][eqn2] += B2[j]*UU;
                 Ad[m][eqn3] += B3[j]*UU;
                 }

            if (!(E->NODE[level][m][i]&OFFSIDE))   {
               E->temp[m][eqn1] = (F[m][eqn1] - Ad[m][eqn1])*E->BI[level][m][eqn1];
               E->temp[m][eqn2] = (F[m][eqn2] - Ad[m][eqn2])*E->BI[level][m][eqn2];
               E->temp[m][eqn3] = (F[m][eqn3] - Ad[m][eqn3])*E->BI[level][m][eqn3];
	       }

                 /* Ad on boundaries differs after the following operation */
	    for(j=0;j<max_eqn;j++)
		    Ad[m][C[j]]  += B1[j]*E->temp[m][eqn1]
                                 +  B2[j]*E->temp[m][eqn2]
                                 +  B3[j]*E->temp[m][eqn3];

	    d0[m][eqn1] += E->temp[m][eqn1];
	    d0[m][eqn2] += E->temp[m][eqn2];
	    d0[m][eqn3] += E->temp[m][eqn3];
  	    }

      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(i=1;i<=E->lmesh.NNO[level];i++)
          if(E->NODE[level][m][i] & OFFSIDE)   {
	    eqn1=E->ID[level][m][i].doff[1];
	    eqn2=E->ID[level][m][i].doff[2];
	    eqn3=E->ID[level][m][i].doff[3];
	    Ad[m][eqn1] -= E->temp1[m][eqn1];
	    Ad[m][eqn2] -= E->temp1[m][eqn2];
	    Ad[m][eqn3] -= E->temp1[m][eqn3];
	    }

      exchange_id_d(E, Ad, level);

      for (m=1;m<=E->sphere.caps_per_proc;m++)
 	for(i=1;i<=E->lmesh.NNO[level];i++)
          if(E->NODE[level][m][i] & OFFSIDE)   {
	    eqn1=E->ID[level][m][i].doff[1];
	    eqn2=E->ID[level][m][i].doff[2];
	    eqn3=E->ID[level][m][i].doff[3];
	    Ad[m][eqn1] += E->temp1[m][eqn1];
	    Ad[m][eqn2] += E->temp1[m][eqn2];
	    Ad[m][eqn3] += E->temp1[m][eqn3];
	    }


	count++;

/*     for (m=1;m<=E->sphere.caps_per_proc;m++)
	for(i=0;i<neq;i++)          {
	   F[m][i] -= Ad[m][i];
	   Ad[m][i] = 0.0;
	  }
*/
      }

  /*   parallel_process_termination();
*/
    *cycles=count;
    return;

}

void print_elt_k(E,a)
     struct All_variables *E;
     double a[24*24];

{ int l,ll,n;

  printf("elt k is ...\n");


  n = loc_mat_size[E->mesh.nsd];

  for(l=0;l<n;l++)
    { fprintf(stderr,"\n");fflush(stderr);
      for(ll=0;ll<n;ll++)
	{ fprintf(stderr,"%s%.3e ",a[ll*n+l] >= 0.0 ? "+" : "",a[ll*n+l]);
	  fflush(stderr);
	}
    }
  fprintf(stderr,"\n"); fflush(stderr);

  return; }


double cofactor(A,i,j,n)
     double A[4][4];
     int i,j,n;

{ int k,l,p,q;
  double determinant();
  double **dmatrix();

  double B[4][4]; /* because of recursive behaviour of det/cofac, need to use
			       new copy of B at each 'n' level of this routine */

  if (n>3) printf("Error, no cofactors for matrix more than 3x3\n");

  p=q=1;

  for(k=1;k<=n;k++)    {
     if(k==i) continue;
     for(l=1;l<=n;l++)      {
	   if (l==j) continue;
           B[p][q]=A[k][l];
	   q++ ;
	   }
     q=1;p++;
     }


  return(epsilon[i][j]*determinant(B,n-1));


}


/* Fast (conditional) determinant for 3x3 or 2x2 ... otherwise calls general routine */

double determinant(A,n)
     double A[4][4];
     int n;

{ double gen_determinant();

  switch (n)
    { case 1:
	return(A[1][1]);
      case 2:
	return(A[1][1]*A[2][2]-A[1][2]*A[2][1]);
      case 3:
	return(A[1][1]*(A[2][2]*A[3][3]-A[2][3]*A[3][2])-
	       A[1][2]*(A[2][1]*A[3][3]-A[2][3]*A[3][1])+
	       A[1][3]*(A[2][1]*A[3][2]-A[2][2]*A[3][1]));
      default:
	return(1);
/*	return(gen_determinant(A,n)); */
      }
}


/* recursive function to determine matrix determinant */

double gen_determinant(A,n)
     double **A;
     int n;

{ double det;
  double cofactor();

  int i;


  if(n==1) return(A[1][1]); /* need a way to break the recursion */

  det=0.0;
  for(i=1;i<=n;i++)
    det += A[1][i]*cofactor(A,1,i,n);

  return(det);
}


 float area_of_4node(x1,y1,x2,y2,x3,y3,x4,y4)
 float x1,y1,x2,y2,x3,y3,x4,y4;

 {
 float area;

 area = fabs(0.5*(x1*(y2-y4)+x2*(y4-y1)+x4*(y1-y2)))
      + fabs(0.5*(x2*(y3-y4)+x3*(y4-y2)+x4*(y2-y3)));

 return area;
 }

/* =====================================*/
 double sphere_h(l,m,t,f,ic)
 int l,m,ic;
 double t,f;
 {

 double plgndr_a(),sphere_hamonics;

 sphere_hamonics = 0.0;
 if (ic==0)
    sphere_hamonics = cos(m*f)*plgndr_a(l,m,t);
 else if (m)
    sphere_hamonics = sin(m*f)*plgndr_a(l,m,t);

 return sphere_hamonics;
 }

/* =====================================*/
 double plgndr_a(l,m,t)
 int l,m;
 double t;
 {

  int i,ll;
  double x,fact,pll,pmm,pmmp1,somx2,plgndr;
  const double two=2.0;
  const double one=1.0;

  x = cos(t);
  pmm=one;
  if(m>0) {
    somx2=sqrt((one-x)*(one+x));
    fact = one;
    for (i=1;i<=m;i++)   {
      pmm = -pmm*fact*somx2;
      fact = fact + two;
      }
    }

  if (l==m)
     plgndr = pmm;
  else  {
     pmmp1 = x*(2*m+1)*pmm;
     if(l==m+1)
       plgndr = pmmp1;
     else   {
       for (ll=m+2;ll<=l;ll++)  {
         pll = (x*(2*ll-1)*pmmp1-(ll+m-1)*pmm)/(ll-m);
         pmm = pmmp1;
         pmmp1 = pll;
         }
       plgndr = pll;
       }
     }

 return plgndr;
 }

/* =====================================
 =====================================*/
 double modified_plgndr_a(l,m,t)
 int l,m;
 double t;
 {

  int i,ll;
  double x,fact1,fact2,fact,pll,pmm,pmmp1,somx2,plgndr;
  const double three=3.0;
  const double two=2.0;
  const double one=1.0;

  x = cos(t);
  pmm=one;
  if(m>0) {
    somx2=sqrt((one-x)*(one+x));
    fact1= three;
    fact2= two;
    for (i=1;i<=m;i++)   {
      fact=sqrt(fact1/fact2);
      pmm = -pmm*fact*somx2;
      fact1+=  two;
      fact2+=  two;
      }
    }

  if (l==m)
     plgndr = pmm;
  else  {
     pmmp1 = x*sqrt(two*m+three)*pmm;
     if(l==m+1)
       plgndr = pmmp1;
     else   {
       for (ll=m+2;ll<=l;ll++)  {
	 fact1= sqrt((4.0*ll*ll-one)*(double)(ll-m)/(double)(ll+m));
	 fact2= sqrt((2.0*ll+one)*(ll-m)*(ll+m-one)*(ll-m-one)
		     /(double)((two*ll-three)*(ll+m)));
         pll = ( x*fact1*pmmp1-fact2*pmm)/(ll-m);
         pmm = pmmp1;
         pmmp1 = pll;
         }
       plgndr = pll;
       }
     }

 plgndr /= sqrt(4.0*M_PI);

 if (m!=0) plgndr *= sqrt(two);

 return plgndr;
 }

 /* ===================================  */
  double sqrt_multis(jj,ii)
  int ii,jj;
 {
  int i;
  double sqrt_multisa;

  sqrt_multisa = 1.0;
  if(jj>ii)
    for (i=jj;i>ii;i--)
      sqrt_multisa *= 1.0/sqrt((double)i);

  return sqrt_multisa;
  }

 /* ===================================  */
  double multis(ii)
  int ii;
 {
  int i;
  double multisa;

  multisa = 1.0;
  if (ii)
    for (i=2;i<=ii;i++)
      multisa *= (double)i;

  return multisa;
  }


 /* ===================================  */
 int int_multis(ii)
 int ii;
 {
 int i,multisa;

 multisa = 1;
 if (ii)
   for (i=2;i<=ii;i++)
     multisa *= i;

 return multisa;
 }
