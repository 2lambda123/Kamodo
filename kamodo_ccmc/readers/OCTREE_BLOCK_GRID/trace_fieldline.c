/*******************************************************************/
/* trace_fieldline - library of field line tracer routines         */
/* for use with IDL  - various grids and coordinate systems        */
/* Author: Lutz Rastaetter, CCMC at NASA GSFC                      */ 
/* Modification history:                                           */
/*                        Feb. 24, 2004: use symbolic type         */
/*                             IDL_LONG to interface with multiple */
/*                             versions of IDL                     */
/*******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fl_extern.h"

#define debug_main 0
#define debug_trace 1
#define debug_iout 100
/* #define report_csh */

void dipole_axis(float *n1, float *n2, float *n3, float *tilt){
  // compute dipole axis orientation from 2-element tilt angle array
  // dipole axis orientation
  *n1 =-sin(tilt[0]);
  *n2 = cos(tilt[0])*(-sin(tilt[1]));
  *n3 = cos(tilt[0])*  cos(tilt[1]);
}

void b_dipole(float *bx, float *by, float *bz,
	      float x, float y, float z,
	      float n1, float n2, float n3,
	      float bdp){
  float rr, rn3, bdp_rr5;
  rr = x*x+y*y+z*z;
  if (rr >= 1.){
    bdp_rr5 = bdp*pow(rr,-2.5);
    rn3 = 3*(n1*x+n2*y+n3*z);
    *bx = bdp_rr5*(x*rn3-n1*rr);
    *by = bdp_rr5*(y*rn3-n2*rr);
    *bz = bdp_rr5*(z*rn3-n3*rr);
  } else {
    // regularized dipole (constant inside body)
    *bx = 2*bdp*n1;
    *by = 2*bdp*n2;
    *bz = 2*bdp*n3;
  }
}

void hunt(float *xx, IDL_LONG n, float x, int *jlo)
{
/* zero-offset version of Numerical Recipies' "hunt" */
  int jm,jhi,inc,ascnd,n1;

  n1=n-1;
  
/*  *jlo +=1; */                       /* zero-offset-input -> unit-offset */
  ascnd=(xx[n1] >= xx[0]);         /* ascending order? */
  if (*jlo <=0 || *jlo > n1) {     /* no useful input -> bisection */
    *jlo = 0;
    jhi = n1+1;
  } else {
    inc = 1;                           /* hunting increment */
    if ((x >= xx[*jlo]) == ascnd) {     /* hunt up */
      if (*jlo >= n1) return;
      jhi = (*jlo)+1;
      while ((x > xx[jhi]) == ascnd) { /* not done yet */
	inc += inc;                    /* double increment */
	jhi = (*jlo)+inc;
	if (jhi > n1) { 
	  jhi = n1+1;                 /* Done hunting since off end of table */
	  break;
	}                            /* try again */
      }                    /* done hunting, value bracketed */
    } else {
      if (*jlo == 1) {               /* hunt down */
	jlo = 0;
	return;
      }
      jhi = (*jlo)--;
      while(x < xx[*jlo] == ascnd){     /* not done yet */
	jhi = (*jlo);
	inc *= 2;
	if (inc >= jhi) {          /* off end of table */
	  *jlo = 0;
	  break;
	}
	else *jlo = jhi-inc;
      }                           /* try again */
    }                             /* hunt done */
  }                               /* hunt done, begin final bisection */
  while( jhi-(*jlo) != 1) {
    jm = (jhi+(*jlo))/2;
    if ((x >= xx[jm]) == ascnd)
      *jlo = jm;
    else
      jhi = jm;
  }
}

void hunt_in_block(float xx, float yy, float zz, int *ix, int *iy, int *iz,
                   float *x, float *y, float *z,
                   IDL_LONG NX, IDL_LONG NY, IDL_LONG NZ)
{
/* the calls to hunt modify values at pointer locations
   ix, iy, iz, respectively */
    hunt(x,NX,xx,ix);
    hunt(y,NY,yy,iy);
    hunt(z,NZ,zz,iz);
}


#define DEBUG_MAIN 1
IDL_LONG main_b_analytic_idl(int argc, char* argv[]){
    
  IDL_LONG *Step_max,*status,nf,*v_indices;
  int trace_method;
  float  mag_dipole_strength, 
    *mag_dipole_axis,
    mirror_dipole_strength,
    mirror_dipole_axis[3],
    *mirror_dipole_xyz, *b_sw,
/* outputs */
    *v_mag, *flx, *fly, *flz, *mag_dipole_tilt,
    x_start, y_start, z_start,
    *xrange, *yrange, *zrange,
    delta, r_end, bdp=0., tilt=0., deg2rad=1.;
  
  v_mag = NULL;
  
  mag_dipole_strength = ((float*)argv[0])[0];
  mag_dipole_axis = (float*)argv[1];
  mirror_dipole_xyz = (float*)argv[2];
  mirror_dipole_strength = ((float*)argv[3])[0];
  b_sw = (float*)argv[4];

  flx = (float*)argv[5];
  fly = (float*)argv[6];
  flz = (float*)argv[7];
  v_mag = (float*)argv[8];
  
  xrange = (float*)argv[9];
  yrange = (float*)argv[10];
  zrange = (float*)argv[11];

  MISSING = ((float*)argv[12])[0];
  delta = ((float*)argv[13])[0];
  Step_max = (IDL_LONG*)argv[14];
  r_end = ((float*)argv[15])[0];
  status = (IDL_LONG*)argv[16];
  /* assign values */
  mirror_dipole_axis[0] = -mag_dipole_axis[0];
  mirror_dipole_axis[1] =  mag_dipole_axis[1];
  mirror_dipole_axis[2] =  mag_dipole_axis[2];
  x_start = flx[0];
  y_start = fly[0];
  z_start = flz[0];    
  if (debug_main){
    printf("NX: %ld NY: %ld NZ: %ld N_blks: %ld\n", NX, NY, NZ, N_blks);
    printf("Delta: %f step_max: %ld r_end: %f\n", delta, Step_max[0], r_end);
    printf("Start: X %f Y %f Z %f\n", x_start, y_start, z_start);
  }
  
  if (argc == 17 ) 
    status[0]=trace_fieldline_analytic_b(
				     x_start, y_start, z_start, 
				     r_end,
				     xrange, yrange, zrange,
				     flx, fly, flz, 
				     v_mag, 
				     Step_max, 
				     delta, 
				     mag_dipole_strength, 
				     mag_dipole_axis,
				     mirror_dipole_strength, 
				     mirror_dipole_axis,
				     mirror_dipole_xyz,
				     b_sw); 
  else fprintf(stderr, "incorrect number of arguments: %i\n", argc);
  return(0);

}


IDL_LONG main_octree_idl(int argc, char* argv[])
{
/* interface for IDL's call_external for trace_fieldline_octree */
/* input variables:
v1,v2,v3: component array of vector variable
x,y,z: arrays of grid positions - NX,NY or NZ elements, respectively
x_bx1,y_by1,z_bz1: x,y,z positions for respective B-components if
                 different from regulart x,y,z
xr,yr,zr: arrays of block ranges -  (2,n4) elements
n1,n2,n3,n4: spatial dimensions of grid (n1,n2,n3 for X,Y,Z,
             n4 for number of blocks: n4>1: AMR grid; n4=1: product grid)
delta: iteration step length along stream line
max_iter: maximum number of iteration steps
r_min: minimum radius around Earth where to stop integration
status: type of field line - exit status of trace_fieldline subroutine.

return data
===========
status: flag indicating type of field line
fx,fy,fz: arrays holding x,y,z of vertices along streamline (arrays at least max_step long
*/

//    IDL_LONG NX,NY,NZ,NB,*Step_max,*status,nf,*v_indices;
    IDL_LONG *Step_max, *status, nf, *v_indices;
    int trace_method;
    float
      //      *v1, *v2, *v3,
      *v_mag, *flx, *fly, *flz, *mag_dipole_tilt,
      x_start, y_start, z_start,
      delta, r_end, bdp=0., tilt=0., deg2rad=1.;

    v_mag = NULL;
    
    fields = (float*)argv[0];
    nf = *((IDL_LONG*)argv[1]);
    v_indices = (IDL_LONG*)argv[2];
    x_blk = (float*)argv[3];
    y_blk = (float*)argv[4];
    z_blk = (float*)argv[5];
    octree_blocklist = (octree_block*)argv[6];
    numparents_at_AMRlevel = (IDL_LONG*)argv[7];
    block_at_AMRlevel = (IDL_LONG *)argv[8];
    
    NX = *((IDL_LONG*)argv[9]);
    NY = *((IDL_LONG*)argv[10]);
    NZ = *((IDL_LONG*)argv[11]);
    N_blks = *((IDL_LONG*)argv[12]);
    delta = *((float*)argv[13]);
    r_end = *((float*)argv[14]);
    Step_max = ((IDL_LONG*)argv[15]);
    bdp = *((float*)argv[16]);
    tilt = *((float*)argv[17]);
    mag_dipole_tilt = (float*)argv[17];
    flx = (float*)argv[18];
    fly = (float*)argv[19];
    flz = (float*)argv[20];
    status = ((IDL_LONG*)argv[21]);
    deg2rad = *((float*)argv[22]);

    x_start = flx[0];
    y_start = fly[0];
    z_start = flz[0];    
    if (debug_main){
      printf("NX: %ld NY: %ld NZ: %ld N_blks: %ld\n", NX, NY, NZ, N_blks);
      printf("Delta: %f step_max: %ld r_end: %f\n", delta, Step_max[0], r_end);
      printf("Start: X %f Y %f Z %f\n", x_start, y_start, z_start);
    }
    float *bx, *by, *bz;
    bx = fields+v_indices[0]*NX*NY*NZ*N_blks;
    by = fields+v_indices[1]*NX*NY*NZ*N_blks;
    bz = fields+v_indices[2]*NX*NY*NZ*N_blks;
    
    if (argc == 23 ) 
        status[0]=trace_fieldline_octree(
            x_start, y_start, z_start, r_end,
	    bx, by, bz,
            flx, fly, flz, 
            Step_max, delta, bdp, mag_dipole_tilt,deg2rad); 
    else fprintf(stderr,"incorrect number of arguments: %i\n",argc);
    return(0);    
}

IDL_LONG main_idl(int argc, char* argv[])
{
/* interface for IDL's call_external */
/*  Arguments: argc: 17
 argv[] = [float * v1, float * v2, float * v3,
           float * x,  float * y, float * z,
           float * xr,  float * yr, float * zr,
           long n1, long n2, long n3, long n4,
           float delta, float r_min, long * max_iter, long * status,
           float * fx, float * fy, float * fz]
or
 argv[] = [float * v1, float * v2, float * v3,
           float * x,  float * y, float * z,
           float *x_bx1, float *y_by1, float *z_bz1, 
           float * xr,  float * yr, float * zr,
           long n1, long n2, long n3, long n4,
           float delta, float r_min, long * max_iter, long * status,
           float * fx, float * fy, float * fz]
input variables:
v1,v2,v3: component array of vector variable
x,y,z: arrays of grid positions - NX,NY or NZ elements, respectively
x_bx1,y_by1,z_bz1: x,y,z positions for respective B-components if
                 different from regulart x,y,z
xr,yr,zr: arrays of block ranges -  (2,n4) elements
n1,n2,n3,n4: spatial dimensions of grid (n1,n2,n3 for X,Y,Z,
             n4 for number of blocks: n4>1: AMR grid; n4=1: product grid)
delta: iteration step length along stream line
max_iter: maximum number of iteration steps
r_min: minimum radius around Earth where to stop integration
status: type of field line - exit status of trace_fieldline subroutine.

return data
===========
status: flag indicating type of field line
fx,fy,fz: arrays holding x,y,z of vertices along streamline (arrays at least max_step long
*/

/* bdp and tilt not needed
   deg2rad: degree-to-radians conversion factor needed */

  IDL_LONG NX, NY, NZ, NB, *Step_max, *status, usePolarity, nf, *v_indices;
    int trace_method;
    float
      *x, *y, *z,*v_mag,*fields,
      *x_bx1, *y_by1, *z_bz1, *xr, *yr, *zr, *flx, *fly, *flz, *mag_dipole_tilt,
        x_start, y_start, z_start,
        XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX, delta, r_end, bdp=0., tilt=0., deg2rad=1.;

    v_mag = NULL;
    
    switch (argc) {
        case 21: {
/* MAS, spherical, non-staggered grid */
	  fields = (float*)argv[0];
	  nf = *((IDL_LONG*)argv[1]);
	  v_indices = (IDL_LONG*)argv[2];
/*          v1=(float*)argv[0]; 
            v2=(float*)argv[1];
            v3=(float*)argv[2]; */
            x = (float*)argv[3];
            y = (float*)argv[4];
            z = (float*)argv[5];
            x_bx1 = x;
            y_by1 = y;
            z_bz1 = z;
            xr = (float*)argv[6];
            yr = (float*)argv[7];
            zr = (float*)argv[8];
            NX = *((IDL_LONG*)argv[9]);
            NY = *((IDL_LONG*)argv[10]);
            NZ = *((IDL_LONG*)argv[11]);
            NB = *((IDL_LONG*)argv[12]);
            delta = *((float*)argv[13]);
            r_end = *((float*)argv[14]);
            Step_max = ((IDL_LONG*)argv[15]);
            flx = (float*)argv[16];
            fly = (float*)argv[17];
            flz = (float*)argv[18];
            status = ((IDL_LONG*)argv[19]);
            deg2rad = *((float*)argv[20]);
            trace_method = 2;
	    usePolarity = -1;
            break;
        }
        case 22: {
/* other models, cartesian, non-staggered grid */
	    fields = (float*)argv[0];
	    nf = *((IDL_LONG*)argv[1]);
	    v_indices = (IDL_LONG*)argv[2];
/*          v1=(float*)argv[0];
            v2=(float*)argv[1];
            v3=(float*)argv[2]; */
            x = (float*)argv[3];
            y = (float*)argv[4];
            z = (float*)argv[5];
            xr = (float*)argv[6];
            yr = (float*)argv[7];
            zr = (float*)argv[8];
            NX = *((IDL_LONG*)argv[9]);
            NY = *((IDL_LONG*)argv[10]);
            NZ = *((IDL_LONG*)argv[11]);
            NB = *((IDL_LONG*)argv[12]);
            delta = *((float*)argv[13]);
            r_end = *((float*)argv[14]);
            Step_max = ((IDL_LONG*)argv[15]);
            bdp = *((float*)argv[16]);
            tilt = *((float*)argv[17]);
            mag_dipole_tilt = (float*)argv[17];
            flx = (float*)argv[18];
            fly = (float*)argv[19];
            flz = (float*)argv[20];
            status = ((IDL_LONG*)argv[21]);
            trace_method = 0;
            break;
        }
        case 23: {
/* other models, cartesian, non-staggered grid, return magnitude of vector */
	    fields = (float*)argv[0];
	    nf = *((IDL_LONG*)argv[1]);
	    v_indices = (IDL_LONG*)argv[2];
/*          v1=(float*)argv[0];
            v2=(float*)argv[1];
            v3=(float*)argv[2]; */
            x = (float*)argv[3];
            y = (float*)argv[4];
            z = (float*)argv[5];
            xr = (float*)argv[6];
            yr = (float*)argv[7];
            zr = (float*)argv[8];
            NX = *((IDL_LONG*)argv[9]);
            NY = *((IDL_LONG*)argv[10]);
            NZ = *((IDL_LONG*)argv[11]);
            NB = *((IDL_LONG*)argv[12]);
            delta = *((float*)argv[13]);
            r_end = *((float*)argv[14]);
            Step_max = ((IDL_LONG*)argv[15]);
            bdp = *((float*)argv[16]);
            tilt = *((float*)argv[17]);
            mag_dipole_tilt = (float*)argv[17];
            flx = (float*)argv[18];
            fly = (float*)argv[19];
            flz = (float*)argv[20];
            v_mag = (float*)argv[21]; /* array of vector magnitude */
            status = ((IDL_LONG*)argv[22]);
            trace_method = 0;
            break;
        }
        case 25: {
/* MAS/ENLIL, spherical, staggered grid */
	    fields = (float*)argv[0];
	    nf = *((IDL_LONG*)argv[1]);
	    v_indices = (IDL_LONG*)argv[2];
/*          v1=(float*)argv[0]; 
            v2=(float*)argv[1];
            v3=(float*)argv[2]; */
            x = (float*)argv[3];
            y = (float*)argv[4];
            z = (float*)argv[5];
            x_bx1 = (float*)argv[6];
            y_by1 = (float*)argv[7];
            z_bz1 = (float*)argv[8];
            xr = (float*)argv[9];
            yr = (float*)argv[10];
            zr = (float*)argv[11];
            NX = *((IDL_LONG*)argv[12]);
            NY = *((IDL_LONG*)argv[13]);
            NZ = *((IDL_LONG*)argv[14]);
            NB = *((IDL_LONG*)argv[15]);
            delta = *((float*)argv[16]);
            r_end = *((float*)argv[17]);
            Step_max = ((IDL_LONG*)argv[18]);
/*            bdp=*((float*)argv[19]);
              tilt=*((float*)argv[20]); */
            flx = (float*)argv[19];
            fly = (float*)argv[20];
            flz = (float*)argv[21];
            status = ((IDL_LONG*)argv[22]);
            deg2rad = *((float*)argv[23]);
            usePolarity = *((IDL_LONG*)argv[24]);
	    /*	    usePolarity=0; */
            trace_method = 2;
            break;
        }
    case 26: {
/* UCLA-GGCM, cartesian, staggered grid */
	    fields = (float*)argv[0];
	    nf = *((IDL_LONG*)argv[1]);
	    v_indices = (IDL_LONG*)argv[2];
/*          v1=(float*)argv[0];
            v2=(float*)argv[1];
            v3=(float*)argv[2]; */
            x = (float*)argv[3];
            y = (float*)argv[4];
            z = (float*)argv[5];
            x_bx1 = (float*)argv[6];
            y_by1 = (float*)argv[7];
            z_bz1 = (float*)argv[8];
            xr = (float*)argv[9];
            yr = (float*)argv[10];
            zr = (float*)argv[11];
            NX = *((IDL_LONG*)argv[12]);
            NY = *((IDL_LONG*)argv[13]);
            NZ = *((IDL_LONG*)argv[14]);
            NB = *((IDL_LONG*)argv[15]);
            delta = *((float*)argv[16]);
            r_end = *((float*)argv[17]);
            Step_max = ((IDL_LONG*)argv[18]);
            bdp = *((float*)argv[19]);
            tilt = *((float*)argv[20]);
            mag_dipole_tilt = (float*)argv[20];
            flx = (float*)argv[21];
            fly = (float*)argv[22];
            flz = (float*)argv[23];
            v_mag = (float*)argv[24];
            status = ((IDL_LONG*)argv[25]);
            trace_method = 1;
            break;
        }

        default: {
            return(-3);
        }
    }
    
        
    x_start = flx[0];
    y_start = fly[0];
    z_start = flz[0];    
    if (debug_main){
      printf("NX: %ld NY: %ld NZ: %ld NB: %ld\n", NX, NY, NZ, NB);
      printf("Delta: %f step_max: %ld r_end: %f\n", delta, Step_max[0], r_end);
      printf("Start: X %f Y %f Z %f\n", x_start, y_start, z_start);
    }
    
    if (trace_method == 0){
/* other models with single grid for vector field components */    
       status[0] = trace_fieldline(
           x_start, y_start, z_start,
           r_end, NX, NY, NZ, NB, x, y, z,
	   /*           xr, yr, zr, v1, v2, v3, */
           xr, yr, zr, fields, nf, v_indices,
           flx, fly, flz, v_mag,
           Step_max, delta, bdp, mag_dipole_tilt);
   }
/* UCLA-GGCM with staggered grids for vector field components */    
   if (trace_method == 1){
       status[0]=trace_fieldline_staggered(
           x_start, y_start, z_start,
           r_end, NX, NY, NZ, NB, x, y, z,
           x_bx1, y_by1, z_bz1,
           xr, yr, zr, fields,nf,v_indices,
           flx, fly, flz, v_mag,
           Step_max, delta, bdp, mag_dipole_tilt);
   }
/* MAS coronal MHD model - spherical grid staggered/non-staggered */   
   if (trace_method == 2){
     if (debug_main){
       fprintf(stderr,"X0: %f Y0: %f Z0: %f r_end: %f\nRanges x: %f %f y: %f %f z: %f %f\n",
	       x_start, y_start, z_start, r_end, xr[0], xr[1], yr[0], yr[1], zr[0], zr[1]);
     }
       status[0]=trace_fieldline_spherical_staggered(
           x_start, y_start, z_start, /* x,y,z are actually r, phi and theta */
           r_end, NX, NY, NZ, NB, x, y, z,
           x_bx1, y_by1, z_bz1, /* may be identical to "x","y","z" */
           xr, yr, zr, fields, nf, v_indices,
           flx, fly, flz,
           Step_max, delta, deg2rad, usePolarity);
   }

   if (Step_max[0] <= 0) {
       status[0] = -2L;
   } else {
       if (debug_main) {
           printf("End: X %f Y %f Z %f  step: %ld  status: %ld\n",
                  flx[Step_max[0]], fly[Step_max[0]], flz[Step_max[0]],
                  Step_max[0], status[0]);
       }
   }
   
   return(0);
}


/**********************************************************************/
/* trace_fieldline                                                    */
/*                                                                    */
/* trace fieldline through an adaptive block grid with blocks of       */
/* dimensions NX,NY,NZ and number of blocks N_blks                    */
/* integration method: Runge-Kutta-type of order dn^2                 */
/* Parameters:                                                        */
/*   r_end: radius where should end sqrt(x^2+y^2+z*z)>r_end           */
/*   NX,NY,NZ: 
/*   x_blk,y_blk,z_blk, cell center locations in x,y,z for all blocks */
/*   xr_blk,y_blk,zr_blk, spatial coverage of blocks                  */
/*   x_start,y_start,z_start: start point coordinates                 */
/*   b_x,b_y,b_z: field used to trace flow lines of (may be magnetic  */
/*     field, velocity or current density as defined by the driver    */
/*     program                                                        */
/*   flx,fly,flz: array to return results in (field line vertices)    */
/*   dn iteration step as fraction of cell size in block              */
/*   bdp: dipole strength (Earth: -31100; set to 0 if using full      */
/*        magnetic field b)                                           */
/*   tilt: dipole tilt; analytic expression for dipole used with      */
/*        field b1 instead of full b field to improve interpolation   */
/**********************************************************************/


IDL_LONG trace_fieldline(float x_start, float y_start, float z_start,
                    float r_end,
                    IDL_LONG NX, IDL_LONG NY, IDL_LONG NZ, IDL_LONG N_blks,
                    float *x_blk,  float *y_blk,  float *z_blk,
                    float *xr_blk, float *yr_blk, float *zr_blk,
                    float *fields, IDL_LONG nf, IDL_LONG *v_indices,
                    float *flx, float *fly, float *flz, float *v_mag,
                    IDL_LONG *step_max,
                    float dn, float bdp, float *tilt)

{
    long int ivv, NV, NVV, i_block, step, ib;
    int ix, iy, iz, status;
    float x, y, z, x2, y2, z2, r, dx_blk, dy_blk, dz_blk, m_x, m_y, m_z, bx, by, bz, b, dt;
    float *x_blk_tmp, *y_blk_tmp, *z_blk_tmp, *b_x_tmp, *b_y_tmp, *b_z_tmp;
    float bx_dip, by_dip, bz_dip, xx1, zz1, rr, bdp_rr5;
    float n1, n2, n3;
    float *b_x, *b_y, *b_z;

    dipole_axis(&n1, &n2, &n3, tilt);

    b_x = fields+v_indices[0];
    b_y = fields+v_indices[1];
    b_z = fields+v_indices[2];

    NV = NX*NY;
    NVV = NV*NZ;
    x = x_start;
    y = y_start;
    z = z_start;
    i_block = 0;
/*    if (debug_trace == 1)
        printf("NX: %ld NV %ld\n",NX,NV);  */
    i_block = find_block(x, y, z, xr_blk, yr_blk, zr_blk, N_blks, i_block);
    b_x_tmp = b_x+i_block*NVV*nf;
    b_y_tmp = b_y+i_block*NVV*nf;
    b_z_tmp = b_z+i_block*NVV*nf;
    x_blk_tmp = x_blk+NX*i_block;
    y_blk_tmp = y_blk+NY*i_block;
    z_blk_tmp = z_blk+NZ*i_block;

/*    if (i_block == -1){
        printf("Block 0:\n");
        printf("XR: %f %f\n",xr_blk[0],xr_blk[1]);
        printf("YR: %f %f\n",yr_blk[0],yr_blk[1]);
        printf("ZR: %f %f\n",zr_blk[0],zr_blk[1]);
        }  */
    if (debug_trace)
        printf("trace_fieldline::Fieldline start: %f %f %f I_block: %ld N_blks: %ld R_end: %f Mu_dip: %f tilt %f %f\n",
               x_start, y_start, z_start, i_block, N_blks, r_end, bdp, tilt[0], tilt[1]);
    ix = -1;
    iy = -1;
    iz = -1;
    ib = 0;
    step = 0;
    flx[0] = x_start;
    fly[0] = y_start;
    flz[0] = z_start;
    bx_dip = 0.;
    by_dip = 0.;
    bz_dip = 0.;
    
    while ((i_block >=0) && (i_block < N_blks)
           && (x*x+y*y+z*z >= r_end*r_end)
           && (step < step_max[0]) ){
      long ivv0,ivv1,ivv2,ivv3,ivv4,ivv5,ivv6,ivv7;
/* iterate */
        find_in_block(x, y, z, &ix, &iy, &iz,
                      x_blk_tmp, y_blk_tmp, z_blk_tmp,
                      NX, NY, NZ);
                      
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            printf("I_blk: %ld IX: %ld IY: %ld IZ: %ld NX: %ld x_blk: %f %f N_blks: %ld \n",
                   i_block, ix, iy, iz, NX, x_blk[ix+i_block*NX], x_blk[ix+i_block*NX+1], N_blks);
        dx_blk = x_blk_tmp[ix+1]-x_blk_tmp[ix];
        dy_blk = y_blk_tmp[iy+1]-y_blk_tmp[iy];
        dz_blk = z_blk_tmp[iz+1]-z_blk_tmp[iz];
        if (debug_trace == 1 && ( (step % debug_iout) == 0)) printf("dx: %f %f %f\n",
								    dx_blk, dy_blk, dz_blk);
/* iteration step */
        dt = dx_blk;
        if (dy_blk < dt){dt=dy_blk;}
        if (dz_blk < dt){dt=dz_blk;}
        dt = dt*dn;
/* interpolation weights */
        m_x = (x-x_blk_tmp[ix])/dx_blk;
        m_y = (y-y_blk_tmp[iy])/dy_blk;
        m_z = (z-z_blk_tmp[iz])/dz_blk;
        if ( (abs(m_x) > 2. ) || (abs(m_x) > 2. ) || (abs(m_x) > 2. ) ) {
        printf("m_x: %f %f %f %f %f %f\n", m_x, m_y, m_z, dx_blk, dy_blk, dz_blk);
        printf("Step: %ld xyz: %f %f %f %ld %ld %ld\nxyz_blk %f %f %f  %f %f %f\n",
	       step, x, y, z, ix, iy, iz,
               x_blk_tmp[ix], y_blk_tmp[iy], z_blk_tmp[iz],
               x_blk_tmp[ix+1], y_blk_tmp[iy+1], z_blk_tmp[iz+1]);
            return(-1);
        }
        if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x, y, z, n1, n2, n3, bdp);
	}
	ivv0 = nf*(ix  +iy*NX+iz*NV);
	ivv1 = nf*(ix+1+iy*NX+iz*NV);
	ivv2 = nf*(ix  +(iy+1)*NX+iz*NV);
	ivv3 = nf*(ix+1+(iy+1)*NX+iz*NV);
	ivv4 = nf*(ix  +iy*NX+(iz+1)*NV);
	ivv5 = nf*(ix+1+iy*NX+(iz+1)*NV);
	ivv6 = nf*(ix  +(iy+1)*NX+(iz+1)*NV);
	ivv7 = nf*(ix+1+(iy+1)*NX+(iz+1)*NV);
        bx =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_x_tmp[ivv0]
                    +  m_x *b_x_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_x_tmp[ivv2]
                    +    m_x *b_x_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_x_tmp[ivv4]
                    +   m_x *b_x_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_x_tmp[ivv6]
                    +   m_x *b_x_tmp[ivv7]
                    )
                )+bx_dip;   
        by =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_y_tmp[ivv0]
                    +  m_x *b_y_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_y_tmp[ivv2]
                    +    m_x *b_y_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_y_tmp[ivv4]
                    +   m_x *b_y_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_y_tmp[ivv6]
                    +   m_x *b_y_tmp[ivv7]
                    )
                )+by_dip;   
        bz =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_z_tmp[ivv0]
                    +  m_x *b_z_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_z_tmp[ivv2]
                    +    m_x *b_z_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_z_tmp[ivv4]
                    +   m_x *b_z_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_z_tmp[ivv6]
                    +   m_x *b_z_tmp[ivv7]
                    )
                )+bz_dip;   
        b = sqrt(bx*bx+by*by+bz*bz);
        if (b < 1e-5) b = 1e-5;
        x2 = x+dt/2*bx/b;
        y2 = y+dt/2*by/b;
        z2 = z+dt/2*bz/b;
        if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x2, y2, z2, n1, n2, n3, bdp);
	}
        find_in_block(x2, y2, z2, &ix, &iy, &iz,
                      x_blk_tmp, y_blk_tmp, z_blk_tmp,
                      NX, NY, NZ);
        dx_blk = x_blk_tmp[ix+1]-x_blk_tmp[ix];
        dy_blk = y_blk_tmp[iy+1]-y_blk_tmp[iy];
        dz_blk = z_blk_tmp[iz+1]-z_blk_tmp[iz];
        m_x = (x2-x_blk_tmp[ix])/dx_blk;
        m_y = (y2-y_blk_tmp[iy])/dy_blk;
        m_z = (z2-z_blk_tmp[iz])/dz_blk;
	ivv0 = nf*(ix  +iy*NX+iz*NV);
	ivv1 = nf*(ix+1+iy*NX+iz*NV);
	ivv2 = nf*(ix  +(iy+1)*NX+iz*NV);
	ivv3 = nf*(ix+1+(iy+1)*NX+iz*NV);
	ivv4 = nf*(ix  +iy*NX+(iz+1)*NV);
	ivv5 = nf*(ix+1+iy*NX+(iz+1)*NV);
	ivv6 = nf*(ix  +(iy+1)*NX+(iz+1)*NV);
	ivv7 = nf*(ix+1+(iy+1)*NX+(iz+1)*NV);
        bx =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_x_tmp[ivv0]
                    +  m_x *b_x_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_x_tmp[ivv2]
                    +    m_x *b_x_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_x_tmp[ivv4]
                    +   m_x *b_x_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_x_tmp[ivv6]
                    +   m_x *b_x_tmp[ivv7]
                    )
                )+bx_dip;   
        by =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_y_tmp[ivv0]
                    +  m_x *b_y_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_y_tmp[ivv2]
                    +    m_x *b_y_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_y_tmp[ivv4]
                    +   m_x *b_y_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_y_tmp[ivv6]
                    +   m_x *b_y_tmp[ivv7]
                    )
                )+by_dip;   
        bz =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_z_tmp[ivv0]
                    +  m_x *b_z_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_z_tmp[ivv2]
                    +    m_x *b_z_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_z_tmp[ivv4]
                    +   m_x *b_z_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_z_tmp[ivv6]
                    +   m_x *b_z_tmp[ivv7]
                    )
                )+bz_dip;   
        b = sqrt(bx*bx+by*by+bz*bz);
        if (b < 1e-5) b=1e-5;
/* full step with interpolated B at position after half step */
        x2 = x+dt*bx/b;
        y2 = y+dt*by/b;
        z2 = z+dt*bz/b;
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            printf("Step: %ld X: %f Y %f Z %f, X2: %f Y2: %f Z2: %f\n",
                   step, x, y, z, x2, y2, z2);
        step++;
/* save new postion and set up next iteration */
        flx[step] = x2;
        fly[step] = y2;
        flz[step] = z2;
        if (v_mag != NULL) {v_mag[step]=b;}
        x = x2;
        y = y2;
        z = z2;
        i_block = find_block(x, y, z, xr_blk, yr_blk, zr_blk, N_blks, i_block);
        b_x_tmp = b_x+i_block*NVV*nf;
        b_y_tmp = b_y+i_block*NVV*nf;
        b_z_tmp = b_z+i_block*NVV*nf;
        x_blk_tmp = x_blk+NX*i_block;
        y_blk_tmp = y_blk+NY*i_block;
        z_blk_tmp = z_blk+NZ*i_block;
    }
    status=0; /* fieldline has returned to sphere with radius r_end */
/* reached boundary of box / failed to find block for current position */
    if (x*x+y*y+z*z >= r_end*r_end) status=1;
/* ran out of allowed iterations */
    if (step == step_max[0]) status=2;
    
    step_max[0] = step; 
    
    if (debug_trace)
        printf("Fieldline end: %f %f %f I_block: %ld Status: %ld Step: %ld, step_max: %ld r at end %f\n",
           x,y,z,i_block,status,step,step_max[0],sqrt(x*x+y*y+z*z));

    return(status);
}


IDL_LONG trace_fieldline_analytic_b(float x_start,float y_start,float z_start,
				    float r_end,
				    float *xr_blk, float *yr_blk, float *zr_blk,
				    float *flx, float *fly, float *flz, float *v_mag,
				    IDL_LONG *step_max,
				    float dn,
				    float mag_dipole_strength,
				    float *mag_dipole_axis,
				    float mirror_dipole_strength,
				    float *mirror_dipole_axis,
				    float *mirror_dipole_xyz,
				    float *b_sw)
{
    long int ivv, NV, NVV, i_block, step, ib;
    int ix, iy, iz, status;
    float x, y, z, x2, y2, z2, r, dx_blk, dy_blk, dz_blk, m_x, m_y, m_z, bx, by, bz, b, dt;
    float *x_blk_tmp, *y_blk_tmp, *z_blk_tmp, *b_x_tmp, *b_y_tmp, *b_z_tmp;
    float b_dip[3], b_mirror_dip[3], rr;

    x = x_start;
    y = y_start;
    z = z_start;

    if (debug_trace)
        printf("trace_fieldline::Fieldline start: %f %f %f R_end: %f \nDipole: mu: %f Axis %f %f %f\n Mirror Dipole: mu: %f Axis %f %f %f\nDelta: %f\n",
               x_start, y_start, z_start, r_end,
	       mag_dipole_strength,
	       mag_dipole_axis[0], mag_dipole_axis[1], mag_dipole_axis[2],
	       mirror_dipole_strength,
	       mirror_dipole_axis[0], mag_dipole_axis[1], mag_dipole_axis[2],dn);

    step = 0;
    rr = r_end*r_end;
    i_block = 0;
    if (((x*x+y*y+z*z) < rr) 
       || x < xr_blk[0] || x > xr_blk[1] 
       || y < yr_blk[0] || y > yr_blk[1] 
       || z < zr_blk[0] || z > zr_blk[1] ){i_block = -1;}
    dt = dn/10; /* virtual grid of 1/10 R_E */
    while ((i_block >= 0)
           && (step < step_max[0]) ){
      /* analytic field at x,y,z */
      b_dipole(b_dip,b_dip+1,b_dip+2,
	       x, y, z,
	       mag_dipole_axis[0],
	       mag_dipole_axis[1],
	       mag_dipole_axis[2],
	       mag_dipole_strength);
      b_dipole(b_mirror_dip, b_mirror_dip+1, b_mirror_dip+2,
	       x-mirror_dipole_xyz[0],
	       y-mirror_dipole_xyz[1],
	       z-mirror_dipole_xyz[2],
	       mirror_dipole_axis[0],
	       mirror_dipole_axis[1],
	       mirror_dipole_axis[2],
	       mirror_dipole_strength);

      bx = b_dip[0]+b_mirror_dip[0]+b_sw[0];
      by = b_dip[1]+b_mirror_dip[1]+b_sw[1];
      bz = b_dip[2]+b_mirror_dip[2]+b_sw[2];
      b = sqrt(bx*bx+by*by+bz*bz);
      /* half step with B at previous position */
      if (b < 1e-5) b = 1e-5;
      x2 = x+dt/2*bx/b;
      y2 = y+dt/2*by/b;
      z2 = z+dt/2*bz/b;
      /* analytic field at x2,y2,z2 */
      b_dipole(b_dip, b_dip+1, b_dip+2,
	       x2, y2, z2,
	       mag_dipole_axis[0],
	       mag_dipole_axis[1],
	       mag_dipole_axis[2],
	       mag_dipole_strength);
      b_dipole(b_mirror_dip, b_mirror_dip+1, b_mirror_dip+2,
	       x2-mirror_dipole_xyz[0],
	       y2-mirror_dipole_xyz[1],
	       z2-mirror_dipole_xyz[2],
	       mirror_dipole_axis[0],
	       mirror_dipole_axis[1],
	       mirror_dipole_axis[2],
	       mirror_dipole_strength);
      bx = b_dip[0]+b_mirror_dip[0]+b_sw[0];
      by = b_dip[1]+b_mirror_dip[1]+b_sw[1];
      bz = b_dip[2]+b_mirror_dip[2]+b_sw[2];
      b=sqrt(bx*bx+by*by+bz*bz);
      /* full step with B at position after half step */
      x2 = x+dt*bx/b;
      y2 = y+dt*by/b;
      z2 = z+dt*bz/b;
      if (debug_trace == 1 && ( (step % debug_iout) == 0))
	printf("Delta: %f Step: %ld RR: %f X: %f Y %f Z %f, X2: %f Y2: %f Z2: %f BX: %f BY: %f BZ: %f\n",
	       dt,step,rr,x,y,z,x2,y2,z2,bx,by,bz);
      step++;
      x = x2;
      y = y2;
      z = z2;
      flx[step] = x;
      fly[step] = y;
      flz[step] = z;
      if (v_mag != NULL) {v_mag[step]=b;}
      if (((x*x+y*y+z*z) < rr) 
       || x < xr_blk[0] || x > xr_blk[1] 
       || y < yr_blk[0] || y > yr_blk[1] 
       || z < zr_blk[0] || z > zr_blk[1] ){i_block = -1;}
    }
    status = 0; /* fieldline has returned to sphere with radius r_end */
/* reached boundary of box / failed to find block for current position */
    if (x*x+y*y+z*z >= r_end*r_end) status = 1;
/* ran out of allowed iterations */
    if (step == step_max[0]) status = 2;
    
    step_max[0] = step; 
    
    if (debug_trace)
        printf("Fieldline end: %f %f %f I_block: %ld Status: %ld Step: %ld, step_max: %ld r at end %f\n",
           x, y, z, i_block, status, step, step_max[0], sqrt(x*x+y*y+z*z));

    return(status);
}



IDL_LONG trace_fieldline_staggered(float x_start,float y_start,float z_start,
                    float r_end,
                    IDL_LONG NX, IDL_LONG NY, IDL_LONG NZ, IDL_LONG N_blks,
                    float *x, float *y, float *z,
                    float *x_bx, float *y_by, float *z_bz,
                    float *xr,float *yr, float *zr,
/*                    float *b_x, float *b_y, float *b_z, */
                    float *fields, IDL_LONG nf, IDL_LONG *v_indices,
                    float *flx, float *fly, float *flz, float *v_mag,
                    IDL_LONG *step_max,
                    float dn, float bdp,float *tilt)
{
    long int ivv, NV, NVV, i_block, step,ib;
    int ix, iy, iz, status, ix_bx, iy_by, iz_bz;
    float xx, yy, zz, x2, y2, z2, r, dx, dy, dz, m_x, m_y, m_z, bx, by, bz, b, dt;
    float *x_tmp,*y_tmp,*z_tmp,*b_x_tmp, *b_y_tmp, *b_z_tmp;
    float *x_bx_tmp, *y_by_tmp, *z_bz_tmp, dx_bx, dy_by, dz_bz, m_x_bx, m_y_by, m_z_bz;
    float bx_dip, by_dip, bz_dip, xx1, zz1, rr, rr5, bdp_rr5;
    float *b_x, *b_y, *b_z;
    float n1, n2, n3;

    b_x = fields+v_indices[0];
    b_y = fields+v_indices[1];
    b_z = fields+v_indices[2];

    dipole_axis(&n1, &n2, &n3, tilt);
    
    NV = NX*NY;
    NVV = NV*NZ;
    xx = x_start;
    yy = y_start;
    zz = z_start;
    i_block = -1;
/*    if (debug_trace == 1)
        printf("NX: %ld NV %ld\n",NX,NV);  */
    i_block = find_block(xx,yy,zz,xr,yr,zr,N_blks,i_block);
    b_x_tmp = b_x+i_block*NVV*nf;
    b_y_tmp = b_y+i_block*NVV*nf;
    b_z_tmp = b_z+i_block*NVV*nf;
    x_tmp = x+NX*i_block;
    y_tmp = y+NY*i_block;
    z_tmp = z+NZ*i_block;
    x_bx_tmp = x_bx+NX*i_block;
    y_by_tmp = y_by+NY*i_block;
    z_bz_tmp = z_bz+NZ*i_block;
    if (i_block == -1){
        printf("No block found for position: X: %f Y: %f Z; %f\n", xx, yy, zz);
        return(-1);
    } 
    if (debug_trace)
      printf("Fieldline_staggered start: %f %f %f I_block: %ld N_blks: %ld R_end: %f\n",
           x_start, y_start, z_start, i_block, N_blks, r_end);
    ix = -1;
    iy = -1;
    iz = -1;
    ix_bx = -1;
    iy_by = -1;
    iz_bz = -1;
    ib = 0;
    step = 0;
    flx[0] = x_start;
    fly[0] = y_start;
    flz[0] = z_start;
    while ((i_block >=0) && (i_block < N_blks)
           && (xx*xx+yy*yy+zz*zz >= r_end*r_end)
           && (step < step_max[0]) ){
      long ivv0, ivv1, ivv2, ivv3, ivv4, ivv5, ivv6, ivv7;
/* iterate */
/* half step */
        find_in_block(xx, yy, zz, &ix, &iy, &iz,
                      x_tmp, y_tmp, z_tmp,
                      NX-1, NY-1, NZ-1);
        find_in_block(xx, yy, zz, &ix_bx, &iy_by, &iz_bz,
                      x_bx_tmp, y_by_tmp, z_bz_tmp,
                      NX-1, NY-1, NZ-1);
                      
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            printf("I_blk: %ld IX: %ld IY: %ld IZ: %ld NX: %ld x_blk: %f %f N_blks: %ld \n",
                   i_block,ix,iy,iz,NX,x[ix+i_block*NX],x[ix+i_block*NX+1],N_blks);
        dx = x_tmp[ix+1]-x_tmp[ix];
        dy = y_tmp[iy+1]-y_tmp[iy];
        dz = z_tmp[iz+1]-z_tmp[iz];
        if (debug_trace == 1 && ( (step % debug_iout) == 0)) printf("dx: %f %f %f\n", dx, dy, dz);
/* iteration step */
        dt = dx;
        if (dy < dt){dt = dy;}
        if (dz < dt){dt = dz;}
        dt=dt*dn;
        m_x = (xx-x_tmp[ix])/dx;
        m_y = (yy-y_tmp[iy])/dy;
        m_z = (zz-z_tmp[iz])/dz;
        dx_bx = x_bx_tmp[ix_bx+1]-x_bx_tmp[ix_bx];
        dy_by = y_by_tmp[iy_by+1]-y_by_tmp[iy_by];
        dz_bz = z_bz_tmp[iz_bz+1]-z_bz_tmp[iz_bz];
        m_x_bx = (xx-x_bx_tmp[ix_bx])/dx_bx;
        m_y_by = (yy-y_by_tmp[iy_by])/dy_by;
        m_z_bz = (zz-z_bz_tmp[iz_bz])/dz_bz;
/*        printf("m_x: %f %f %f %f %f %f\n",m_x,m_y,m_z,dx,dy,dz);
 */
        if ( (abs(m_x) > 2. ) || (abs(m_y) > 2. ) || (abs(m_z) > 2. ) ) {
        printf("m_x: %f %f %f %f %f %f\n",m_x,m_y,m_z,dx,dy,dz);
        printf("Step: %ld xyz: %f %f %f %ld %ld %ld\nxyz_blk %f %f %f  %f %f %f\n",
	       step, x, y, z, ix, iy, iz,
               x_tmp[ix], y_tmp[iy], z_tmp[iz],
               x_tmp[ix+1], y_tmp[iy+1], z_tmp[iz+1]);
            return(-1);
        }
        if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, xx, yy, zz, n1, n2, n3, bdp);
	}        
	ivv0 = nf*(ix  +iy*NX+iz*NV);
	ivv1 = nf*(ix+1+iy*NX+iz*NV);
	ivv2 = nf*(ix  +(iy+1)*NX+iz*NV);
	ivv3 = nf*(ix+1+(iy+1)*NX+iz*NV);
	ivv4 = nf*(ix  +iy*NX+(iz+1)*NV);
	ivv5 = nf*(ix+1+iy*NX+(iz+1)*NV);
	ivv6 = nf*(ix  +(iy+1)*NX+(iz+1)*NV);
	ivv7 = nf*(ix+1+(iy+1)*NX+(iz+1)*NV);
        bx =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_x_tmp[ivv0]
                    +  m_x *b_x_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_x_tmp[ivv2]
                    +    m_x *b_x_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_x_tmp[ivv4]
                    +   m_x *b_x_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_x_tmp[ivv6]
                    +   m_x *b_x_tmp[ivv7]
                    )
                )+bx_dip;   
        by =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_y_tmp[ivv0]
                    +  m_x *b_y_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_y_tmp[ivv2]
                    +    m_x *b_y_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_y_tmp[ivv4]
                    +   m_x *b_y_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_y_tmp[ivv6]
                    +   m_x *b_y_tmp[ivv7]
                    )
                )+by_dip;   
        bz =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_z_tmp[ivv0]
                    +  m_x *b_z_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_z_tmp[ivv2]
                    +    m_x *b_z_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_z_tmp[ivv4]
                    +   m_x *b_z_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_z_tmp[ivv6]
                    +   m_x *b_z_tmp[ivv7]
                    )
                )+bz_dip;   
        b = sqrt(bx*bx+by*by+bz*bz);
        if (b < 1e-5) b = 1e-5;
/* do half step here and update grid positions for interpolation */
        x2 = xx+dt/2*bx/b;
        y2 = yy+dt/2*by/b;
        z2 = zz+dt/2*bz/b;
        if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x2, y2, z2, n1, n2, n3, bdp);
	}        
        find_in_block(x2, y2, z2, &ix, &iy, &iz,
                      x_tmp, y_tmp,z_tmp,
                      NX-1, NY-1, NZ-1);
        dx = x_tmp[ix+1]-x_tmp[ix];
        dy = y_tmp[iy+1]-y_tmp[iy];
        dz = z_tmp[iz+1]-z_tmp[iz];
        m_x = (x2-x_tmp[ix])/dx;
        m_y = (y2-y_tmp[iy])/dy;
        m_z = (z2-z_tmp[iz])/dz;
        find_in_block(x2, y2, z2, &ix_bx, &iy_by, &iz_bz,
                      x_bx_tmp, y_by_tmp, z_bz_tmp,
                      NX-1, NY-1, NZ-1);
        dx_bx = x_bx_tmp[ix+1]-x_bx_tmp[ix];
        dy_by = y_by_tmp[iy+1]-y_by_tmp[iy];
        dz_bz = z_bz_tmp[iz+1]-z_bz_tmp[iz];
        m_x_bx = (x2-x_bx_tmp[ix_bx])/dx_bx;
        m_y_by = (y2-y_by_tmp[iy_by])/dy_by;
        m_z_bz = (z2-z_bz_tmp[iz_bz])/dz_bz;
	ivv0 = nf*(ix  +iy*NX+iz*NV);
	ivv1 = nf*(ix+1+iy*NX+iz*NV);
	ivv2 = nf*(ix  +(iy+1)*NX+iz*NV);
	ivv3 = nf*(ix+1+(iy+1)*NX+iz*NV);
	ivv4 = nf*(ix  +iy*NX+(iz+1)*NV);
	ivv5 = nf*(ix+1+iy*NX+(iz+1)*NV);
	ivv6 = nf*(ix  +(iy+1)*NX+(iz+1)*NV);
	ivv7 = nf*(ix+1+(iy+1)*NX+(iz+1)*NV);
        bx =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_x_tmp[ivv0]
                    +  m_x *b_x_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_x_tmp[ivv2]
                    +    m_x *b_x_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_x_tmp[ivv4]
                    +   m_x *b_x_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_x_tmp[ivv6]
                    +   m_x *b_x_tmp[ivv7]
                    )
                )+bx_dip;   
        by =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_y_tmp[ivv0]
                    +  m_x *b_y_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_y_tmp[ivv2]
                    +    m_x *b_y_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_y_tmp[ivv4]
                    +   m_x *b_y_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_y_tmp[ivv6]
                    +   m_x *b_y_tmp[ivv7]
                    )
                )+by_dip;   
        bz =
            (1-m_z)*(
                (1-m_y)*(
                    (1-m_x)*b_z_tmp[ivv0]
                    +  m_x *b_z_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_z_tmp[ivv2]
                    +    m_x *b_z_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x)*b_z_tmp[ivv4]
                    +   m_x *b_z_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_z_tmp[ivv6]
                    +   m_x *b_z_tmp[ivv7]
                    )
                )+bz_dip;   
        b = sqrt(bx*bx+by*by+bz*bz);
        if (b < 1e-5) b = 1e-5;
/* now do full step with interpolated B at position after half step */
        x2 = xx+dt*bx/b;
        y2 = yy+dt*by/b;
        z2 = zz+dt*bz/b;
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            printf("Step: %ld X: %f Y %f Z %f, X2: %f Y2: %f Z2: %f\n",
                   step, x, y, z, x2, y2, z2);
        step++;
/* save new position and update start position for new interation step */
        flx[step] = x2;
        fly[step] = y2;
        flz[step] = z2;
        if (v_mag != NULL) {v_mag[step] = b;}
        xx = x2;
        yy = y2;
        zz = z2;
        i_block = find_block(xx, yy, zz, xr, yr, zr, N_blks, i_block);
        b_x_tmp = b_x+i_block*NVV*nf;
        b_y_tmp = b_y+i_block*NVV*nf;
        b_z_tmp = b_z+i_block*NVV*nf;
        x_tmp = x+NX*i_block;
        y_tmp = y+NY*i_block;
        z_tmp = z+NZ*i_block;
        x_bx_tmp = x_bx+NX*i_block;
        y_by_tmp = y_by+NY*i_block;
        z_bz_tmp = z_bz+NZ*i_block;
    }
    status = 0; /* fieldline has returned to sphere with radius r_end */
/* reached boundary of box / failed to find block for current position */
    if (xx*xx+yy*yy+zz*zz >= r_end*r_end) status = 1;
/* ran out of allowed iterations */
    if (step == step_max[0]) status = 2;
    printf("Position at field line end: %f %f %f Status: %ld\n", xx, yy, zz, status);
    
    
    step_max[0] = step;

    if (debug_trace)
        printf("Fieldline staggered:  Start: %f %f %f  End: %f %f %f\n  I_block: %ld Status: %ld Step: %ld, step_max: %ld r at end %f\n",
               flx[0], fly[0], flz[0], xx, yy, zz, i_block, status, step, step_max[0], sqrt(xx*xx+yy*yy+zz*zz));
    
    return(status);
}

/**********************************************************************/
/* trace fieldline through a staggered 3D  SPHERICAL grid (r,p,t) of  */
/* dimensions NX,NY,NZ and number of blocks N_blks                    */
/* integration method: Runge-Kutta-type of order dn^2                 */
/* Parameters:                                                        */
/*   r_end: radius where should end (x>r_end)                         */
/*   NX,NY,NZ:                                                        */
/*   x_blk,y_blk,z_blk, cell center locations in x,y,z for all blocks */
/*   xr_blk,y_blk,zr_blk, spatial coverage of blocks                  */
/*   x_start,y_start,z_start: start point coordinates                 */
/*   b_x,b_y,b_z: field used to trace flow lines of (may be magnetic  */
/*     field, velocity or current density as defined by the driver    */
/*     program                                                        */
/*   flx,fly,flz: array to return results in (field line vertices)    */
/*   dn iteration step as fraction of cell size in block              */
/*   deg2rad conversion for angles to radians                         */
/* staggered grid (MAS coronal MHD model)                             */
/*   in addition to trace_fieldline.c each field  component bx,by,bx  */
/*   have their own x_bx,y_by,z_bz grid position arrays, respectively */
/*   The other two grid coordinates are identical to the underlying   */
/*   grid x,y,z:  bx: x_bx,y,z,   by: x,y_by,z   bz: x,y,z_bz         */
/**********************************************************************/

IDL_LONG trace_fieldline_spherical_staggered(
                    float x_start, float y_start, float z_start,
                    float r_end,
                    IDL_LONG NX, IDL_LONG NY, IDL_LONG NZ, IDL_LONG N_blks,
                    float* x, float *y, float *z,
                    float* x_bx, float *y_by, float *z_bz,
                    float *xr, float *yr, float *zr,
		    float *fields, IDL_LONG nf, IDL_LONG *v_indices,
/*                    float *b_x, float *b_y, float *b_z, */
                    float *flx, float *fly, float *flz, IDL_LONG *step_max,
                    float dn, float deg2rad, IDL_LONG usePol)
{
  long int ivv, NV, NVV, i_block, step, ib, i;
  int ix, iy, iz, status, ix_bx, iy_by, iz_bz;
  float xx, yy, zz, x2, y2, z2, dx, dy, dz, m_x, m_y, m_z, bx, by, bz, b, dt,
    lat_csh, lat_csh2, br_up, br_down, polarity;
  float *x_tmp, *y_tmp, *z_tmp, *b_x_tmp, *b_y_tmp, *b_z_tmp;
  float *x_bx_tmp, *y_by_tmp, *z_bz_tmp, m_x_bx, m_y_by, m_z_bz, *b_pol_tmp;
  float r, rr, r1, r2, rr1, rr2, rr1_bx, rr2_bx; /* radial weight factors */
  
  float *b_x, *b_y, *b_z, *b_pol;
  
    NV  = NX*NY;
    NVV = NV*NZ;
    xx = x_start;
    yy = y_start;
    zz = z_start;
    i_block = 0;
    b_x = fields+v_indices[0];
    b_y = fields+v_indices[1];
    b_z = fields+v_indices[2];
    b_pol = fields+usePol;
    b_x_tmp = b_x+i_block*NVV*nf;
    b_y_tmp = b_y+i_block*NVV*nf;
    b_z_tmp = b_z+i_block*NVV*nf;
    b_pol_tmp = b_pol+i_block*NVV*nf;
    x_tmp = x+NX*i_block;
    y_tmp = y+NY*i_block;
    z_tmp = z+NZ*i_block;
    x_bx_tmp = x_bx+NX*i_block;
    y_by_tmp = y_by+NY*i_block;
    z_bz_tmp = z_bz+NZ*i_block;
/*        periodic boundary conditions */
/* theta ("z") around poles */
    if (zz < -90.){
        zz = -zz;
        yy += (y_tmp[NY-1]-y_tmp[0])/2;
    }
    if (zz > 90.){
        zz = 90.-zz;
        yy += (y_tmp[NY-1]-y_tmp[0])/2;
    }
/* phi ("y") */
    if (yy < 0  ){ yy += 360; }        
    if (yy > 360){ yy -= 360; }

    i_block=-1*( (xx > x_tmp[NX-1]) || (xx < x_tmp[0]) ||
		 (zz > z_tmp[NZ-1]) || (zz < z_tmp[0]) );
/*    i_block = find_block(xx,yy,zz,xr,yr,zr,N_blks,0);  */
        
    ix = -1;
    iy = -1;
    iz = -1;
    ix_bx = -1;
    iy_by = -1;
    iz_bz = -1;
    ib = 0;
    step = 0;
    flx[0] = x_start;
    fly[0] = y_start;
    flz[0] = z_start;
    if (debug_trace)
      printf("Fieldline start: %f %f %f I_block: %i N_blks: %i R_end: %f Delta: %f\n",
           x_start, y_start, z_start, i_block, N_blks, r_end, dn);

    while ((i_block >=0) && (i_block < N_blks)
           && (xx >= r_end)
           && (step < step_max[0]) ){
      long ivv0, ivv1, ivv2, ivv3, ivv4, ivv5, ivv6, ivv7;
      float rsint, B_pol;
/* iterate */

/*        periodic boundary conditions */
/* theta ("z") around poles */
        if (zz < -90.){
	  fprintf(stderr,"Crossing South pole at step: %ld  Z: %f\n",step,zz);
            zz = -zz;
            yy += 180;
        }
        if (zz > 90.){
	  fprintf(stderr,"Crossing North pole at step: %ld  Z: %f\n",step,zz);
            zz = z_tmp[NZ-1]-zz;
            yy += 180;
        }
/* phi ("y") */
	if (yy < 0  ){ yy += 360; }        
	if (yy > 360){ yy -= 360; }

        hunt_in_block(xx, yy, zz, &ix, &iy, &iz,
                      x_tmp, y_tmp, z_tmp,
                      NX-1,NY-1,NZ-1);
        hunt_in_block(xx, yy, zz, &ix_bx, &iy_by, &iz_bz,
                      x_bx_tmp, y_by_tmp, z_bz_tmp,
                      NX-1, NY-1, NZ-1);
        
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            printf("I_blk: %ld IX: %ld IY: %ld IZ: %ld NX: %ld x_blk: %f %f N_blks: %ld \n",
                   i_block, ix, iy, iz, NX, x[ix+i_block*NX], x[ix+i_block*NX+1], N_blks);
/* iteration step */
/* in terms of cell size */
        dx = x_tmp[ix+1]-x_tmp[ix];
        dy = y_tmp[iy+1]-y_tmp[iy];
        dz = z_tmp[iz+1]-z_tmp[iz];
        dt = dx;
/* compare with       cos(theta)*r*delta(phi) */
	rsint=fabs(xx*cos(deg2rad*zz));
	if (rsint > 0) {
	   if ( (rsint*deg2rad*dy) < dt){
	     dt = rsint*deg2rad*dy;
	   }
        }
/* compare with       r*delta(phi) */
        //if ( (xx*deg2rad*dz) < dt){ dt=xx*deg2rad*dz; }
        dt = dt*dn;
/* calculate interpolation weights */
	rr = xx*xx;
	r1 = x_tmp[ix];
	r2 = x_tmp[ix+1];
	rr1 = x_bx_tmp[ix_bx];
	rr2 = x_bx_tmp[ix_bx+1];
        m_x = (rr - r1*r1 )/( r2*r2 - r1*r1 );
        m_x_bx = (rr -rr1*rr1)/(rr2*rr2-rr1*rr1);
        m_y = (yy-y_tmp[iy])/dy;
	m_y_by = (yy-y_by_tmp[iy_by])/(y_by_tmp[iy_by+1]-y_by_tmp[iy_by]);
        m_z = (zz-z_tmp[iz])/dz;
        m_z_bz = (zz-z_bz_tmp[iz_bz])/(z_bz_tmp[iz_bz+1]-z_bz_tmp[iz_bz]);
        if ( debug_trace && ( (fabs(m_x) > 2. ) || (fabs(m_y) > 2. ) || (fabs(m_z) > 2. )  ) ) {
        printf("m_x: %f %f %f %f %f %f\n", m_x, m_y, m_z, dx, dy, dz);
        printf("Step: %ld xyz: %f %f %f %ld %ld %ld\nxyz_blk %f %f %f  %f %f %f\n",
               step,xx, yy, zz, ix, iy, iz,
               x_tmp[ix], y_tmp[iy], z_tmp[iz],
               x_tmp[ix+1], y_tmp[iy+1], z_tmp[iz+1]);
            return(-1);
        }
	ivv0 = nf*(ix  +iy*NX+iz*NV);
	ivv1 = nf*(ix+1+iy*NX+iz*NV);
	ivv2 = nf*(ix  +(iy+1)*NX+iz*NV);
	ivv3 = nf*(ix+1+(iy+1)*NX+iz*NV);
	ivv4 = nf*(ix  +iy*NX+(iz+1)*NV);
	ivv5 = nf*(ix+1+iy*NX+(iz+1)*NV);
	ivv6 = nf*(ix  +(iy+1)*NX+(iz+1)*NV);
	ivv7 = nf*(ix+1+(iy+1)*NX+(iz+1)*NV);
	/* prevent crossing the current sheet */
	if (usePol > -1) {
	  B_pol=(1-m_z)*(
			 (1-m_y)*(
				  (1-m_x_bx)*b_pol_tmp[ivv0]
				  +  m_x_bx *b_pol_tmp[ivv1]
				  )
			 + m_y*(
				+ (1-m_x_bx)*b_pol_tmp[ivv2]
				+    m_x_bx *b_pol_tmp[ivv3]
				)
			 )
	    +m_z*(
		  +(1-m_y)*(
			    +(1-m_x)*b_pol_tmp[ivv4]
			    +   m_x *b_pol_tmp[ivv5]
			    )
		  + m_y*(
			 +(1-m_x)*b_pol_tmp[ivv6]
			 +   m_x *b_pol_tmp[ivv7]
			 )
		  );
	/* initialize polarity of field for field line */
	  if (step == 0) {polarity=(B_pol > 0);}
	}
	if ((usePol > -1) && ((B_pol > 0) != polarity)){
	  float dr_br, dp_br, dt_br, dr, grad_br2, dphi, dtheta, step_r, step_p, step_t;
	  int ix_, iy_, iz_, ivv_, found;
	  float xx_, yy_, zz_, rr_, last_rr;
	  xx_ = xx;
	  yy_ = yy;
	  zz_ = zz;
	  found = 0;
	  last_rr = 1e9;
	  // pick closest grid cell corner with correct polarity
	  //	  for (iz_=max(0,iz-1);iz_<=min(NZ-1,iz+2);iz_++) {
	  //for (iy_=max(0,iy-1);iy_<=min(NY-1,iy+2);iy_++) {
	  //  for (ix_=max(0,ix-1);ix_<=min(NX-1,ix+2);ix_++) {
	  for (iz_ = iz; iz_<=iz+1; iz_++) {
	    for (iy_ = iy; iy_<=iy+1; iy_++) {
	      for (ix_ = ix; ix_<=ix+1; ix_++) {
		int polarity_test;
		ivv_ = nf*(ix_+iy_*NX+iz_*NV);
		polarity_test = (b_pol_tmp[ivv_] > 0) == polarity;
		last_rr = 1e9; // a large number
		if (debug_trace)
		  fprintf(stderr,"Polarity check: %i %f %i\n",ivv_,b_pol_tmp[ivv_],polarity_test);
		if ( (b_pol_tmp[ivv_] > 0) == polarity){
		  rr_ = (xx-x_tmp[ix_])*(xx-x_tmp[ix_])
		    + xx*cos(deg2rad*zz)*deg2rad*(yy-y_tmp[iy_])
		     *xx*cos(deg2rad*zz)*deg2rad*(yy-y_tmp[iy_])
		    +xx*xx*deg2rad*deg2rad*(zz-z_tmp[iz_])*(zz-z_tmp[iz_]);
		  if (debug_trace)
		    fprintf(stderr,"ix_: %i iy_: %i  iz_: %i \nDX: %f %f %f DY: %f DZ: %f\n",
			  ix_, iy_, iz_,
			  xx,x_tmp[ix_],
			  xx-x_tmp[ix_],
			  xx*cos(deg2rad*zz)*deg2rad*(yy-y_tmp[iy_]),
			  xx*deg2rad*(zz-z_tmp[iz_])
			  );
		  //		  if (found == 0){
		  if (last_rr > rr_){
		    bx = b_x_tmp[ivv_];
		    by = b_y_tmp[ivv_];
		    bz = b_z_tmp[ivv_];
		    B_pol = b_pol_tmp[ivv_];
		    xx_ = x[ix_];
		    yy_ = y[iy_];
		    zz_ = z[iz_];
		    if (debug_trace)
		      fprintf(stderr,"Last RR: %f new RR: %f B_pol: %f\n", last_rr, rr_, B_pol);
		    last_rr = rr_;
		    found=1;
		  }
		}
	      }
	    }
	  }
	  if (found == 0) {
	    // error - terminate field line
	    status = 3;
	    step_max[0] = step;
	    return(-1);
	  }
	  xx = xx_;
	  yy = yy_;
	  zz = zz_;

	  fprintf(stderr,"R: %f Lon: %f Lat: %f Dist: %f B_pol: %f\n",xx,yy,zz,last_rr,B_pol);
	  /* this assumes that the CS is nearly parallel to the ecliptic (theta=0) - this is not the case so we need the otehr componen of grad(Br), too.
	  lat_csh= (z_tmp[iz]*br_up-z_tmp[iz+1]*br_down)/(br_up-br_down);
	  zz=flz[step-1]=2*lat_csh-zz; 
	  */

/* theta ("z") around poles */
	  if (zz < z_tmp[0]){
            zz = z_tmp[0]-zz;
            yy += 180.;
	  }
	  if (zz > z_tmp[NZ-1]){
            zz = z_tmp[NZ-1]-zz;
            yy += 180.;
	  }
	  if (yy > 360. ) yy -= 360;
	  if (yy <   0. ) yy += 360;

	  hunt_in_block(xx, yy, zz, &ix, &iy, &iz,
			x_tmp, y_tmp, z_tmp,
			NX-1, NY-1, NZ-1);
	  hunt_in_block(xx, yy, zz, &ix_bx, &iy_by, &iz_bz,
			x_bx_tmp, y_by_tmp, z_bz_tmp,
			NX-1, NY-1, NZ-1);
	  // need to update all 'm's
	  dx = z_tmp[iz+1]-z_tmp[iz];
	  m_x = (xx-x_tmp[iz])/dx;
	  m_x_bx = (xx-x_bx_tmp[ix_bx])/(x_bx_tmp[ix_bx+1]-x_bx_tmp[ix_bx]);
	  dy = y_tmp[iy+1]-y_tmp[iy];
	  m_y = (yy-y_tmp[iz])/dy;
	  m_y_by = (yy-y_by_tmp[iy_by])/(y_by_tmp[iy_by+1]-y_by_tmp[iy_by]);
	  dz = z_tmp[iz+1]-z_tmp[iz];
	  m_z = (zz-z_tmp[iz])/dz;
	  m_z_bz = (zz-z_bz_tmp[iz_bz])/(z_bz_tmp[iz_bz+1]-z_bz_tmp[iz_bz]);
#ifdef report_csh
	  fprintf(stderr,"Step: %i (0 of 2) Lat: ZZ: %f Z2: %f New: %f Lat_CSH: %f Br_down: %f Br_up: %f Bx: %f\n",
		  step, zz, z2, flz[step-1], lat_csh, br_down, br_up, bx);
#endif
	}
	ivv0 = nf*(ix  +iy*NX+iz*NV);
	ivv1 = nf*(ix+1+iy*NX+iz*NV);
	ivv2 = nf*(ix  +(iy+1)*NX+iz*NV);
	ivv3 = nf*(ix+1+(iy+1)*NX+iz*NV);
	ivv4 = nf*(ix  +iy*NX+(iz+1)*NV);
	ivv5 = nf*(ix+1+iy*NX+(iz+1)*NV);
	ivv6 = nf*(ix  +(iy+1)*NX+(iz+1)*NV);
	ivv7 = nf*(ix+1+(iy+1)*NX+(iz+1)*NV);
        bx =
            (1-m_z)*(
                (1-m_y)*(
			 (1-m_x_bx)*b_x_tmp[ivv0]
                    +  m_x_bx *b_x_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x_bx)*b_x_tmp[ivv2]
                    +    m_x_bx *b_x_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y)*(
                    +(1-m_x_bx)*b_x_tmp[ivv4]
                    +   m_x_bx *b_x_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x_bx)*b_x_tmp[ivv6]
                    +   m_x_bx *b_x_tmp[ivv7]
                    )
                );   
        by =
            (1-m_z)*(
                (1-m_y_by)*(
                    (1-m_x)*b_y_tmp[ivv0]
                    +  m_x *b_y_tmp[ivv1]
                    )
                + m_y_by*(
                    + (1-m_x)*b_y_tmp[ivv2]
                    +    m_x *b_y_tmp[ivv3]
                    )
                )
            + m_z*(
                (1-m_y_by)*(
                    +(1-m_x)*b_y_tmp[ivv4]
                    +   m_x *b_y_tmp[ivv5]
                    )
                + m_y_by*(
                    +(1-m_x)*b_y_tmp[ivv6]
                    +   m_x *b_y_tmp[ivv7]
                    )
                );   
        bz =
            (1-m_z_bz)*(
                (1-m_y)*(
                    (1-m_x)*b_z_tmp[ivv0]
                    +  m_x *b_z_tmp[ivv1]
                    )
                + m_y*(
                    + (1-m_x)*b_z_tmp[ivv2]
                    +    m_x *b_z_tmp[ivv3]
                    )
                )
            + m_z_bz*(
                (1-m_y)*(
                    +(1-m_x)*b_z_tmp[ivv4]
                    +   m_x *b_z_tmp[ivv5]
                    )
                + m_y*(
                    +(1-m_x)*b_z_tmp[ivv6]
                    +   m_x *b_z_tmp[ivv7]
                    )
                );   
        b = sqrt(bx*bx+by*by+bz*bz);
        if (b < 1e-5) b = 1e-5;
/* iteration in space: use scaling factors 1., 1/(r*cos(theta) ), 1/r */
        x2 = xx+dt*bx/b;
        y2 = yy+dt*by/(b*deg2rad*max(xx*cos(deg2rad*zz),1e-5) );
        z2 = zz+dt*bz/(b*xx*deg2rad);
/* theta ("z") around poles */
        if (z2 < z_tmp[0]){
            z2 = z_tmp[0]-z2;
            y2 += 180.;
        }
        if (z2 > z_tmp[NZ-1]){
            z2 = z_tmp[NZ-1]-z2;
            y2 += 180.;
        }
/* phi ("y") */
	if (y2 < 0  ){ y2 += 360; }        
	if (y2 > 360){ y2 -= 360; }

        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            printf("Step: %ld X: %f Y %f Z %f, X2: %f Y2: %f Z2: %f B: %g BX: %f %f %f D2R: %f\n",
                   step, xx, yy, zz, x2, y2, z2, b, bx, by, bz, deg2rad);
        step++;
        flx[step] = x2;
        fly[step] = y2;
        flz[step] = z2;
        xx = x2;
        yy = y2;
        zz = z2;
/*        periodic boundary conditions */
/* theta ("z") around poles */
        if (zz < z_tmp[0]){
            zz = z_tmp[0]-zz;
            yy += 180.;
        }
        if (zz > z_tmp[NZ-1]){
            zz = z_tmp[NZ-1]-zz;
            yy += 180.;
        }
/* phi ("y") */
	if (yy < 0  ){ yy += 360; }        
	if (yy > 360){ yy -= 360; }
            
        i_block=-1*( (xx > x_tmp[NX-1]) || (xx < x_tmp[0]) ||
		     (zz > z_tmp[NZ-1]) || (zz < z_tmp[0]) );
/*        i_block = find_block(xx,yy,zz,xr,yr,zr,N_blks,0);  */
        
        if (i_block >= 0){
            b_x_tmp = b_x+i_block*NVV*nf;
            b_y_tmp = b_y+i_block*NVV*nf;
            b_z_tmp = b_z+i_block*NVV*nf;
            b_pol_tmp = b_pol+i_block*NVV*nf;
            x_tmp = x+NX*i_block;
            y_tmp = y+NY*i_block;
            z_tmp = z+NZ*i_block;
            x_bx_tmp = x_bx+NX*i_block;
            y_by_tmp = y_by+NY*i_block;
            z_bz_tmp = z_bz+NZ*i_block;
        }
    }
    status=0; /* fieldline has returned to sphere with radius r_end */
/* reached boundary of box / failed to find block for current position */
    if (xx*xx+yy*yy+zz*zz >= r_end*r_end) status = 1;
/* ran out of allowed iterations */
    if (step == step_max[0]) status=2;
    
    step_max[0] = step; 
    
    if (debug_trace)
      printf("Fieldline end: %f %f %f I_block: %ld Status: %ld Step: %ld, step_max: %ld r at end %f\n",
           xx, yy, zz, i_block, status, step, step_max[0], xx);

    return(status);
}

/**********************************************************************/
/* trace_fieldline_octree: fieldline tracer using octree structure of */
/* blocks and interpolate_amrdata to get data values */
/**********************************************************************/
IDL_LONG trace_fieldline_octree(float x_start,float y_start,float z_start,
                                float r_end,
				float *bxfield, float *byfield, float *bzfield,
                                float *flx, float *fly, float *flz, IDL_LONG *step_max,
                                float dn, float bdp, float *tilt, float spherical_deg2rad)
{
    long int ivv, NV, NVV, i_block, step, ib, NULL_L=0, ONE_L=1;
    int ix, iy, iz, status;
    float x, y, z, x2, y2, z2, r, dx_blk, dy_blk, dz_blk, m_x, m_y, m_z;
    float bx_dip, by_dip, bz_dip, xx1, zz1, rr, rr5, sintilt, costilt, sintilt2, costilt2;
    float n1, n2, n3;
    float tiny=1e-5;
    int is_spherical=0;
    float y_z_scale=1;
    // setup dipole axis to compute dipole magnetic field 
    dipole_axis(&n1, &n2, &n3, tilt);

    if (spherical_deg2rad > 0.99){ /* nonzero values -> spherical grid */
      y_z_scale = spherical_deg2rad;
      return(status);
    }

    // start up - grid size in block
    NV = NX*NY;
    NVV = NV*NZ;
    // start position
    x = x_start;
    y = y_start;
    z = z_start;
    // find block
    i_block = find_octree_block(x, y, z,- 2L, -1);
    if (debug_trace)
      fprintf(stderr,"Fieldline start: %f %f %f I_block: %ld N_blks: %ld R_end: %f\n",
           x_start, y_start, z_start, i_block, N_blks, r_end);
    // populate tracing variables
    ib = 0;
    step = 0;
    flx[0] = x_start;
    fly[0] = y_start;
    flz[0] = z_start;
    // tracer loop
    while ((i_block >=0) && (i_block < N_blks)
           && (x*x+y*y+z*z >= r_end*r_end)
           && (step < step_max[0]) ){
      long ivv0,ivv1,ivv2,ivv3,ivv4,ivv5,ivv6,ivv7;
      float bx,by,bz,bx2,by2,bz2,bdp_rr5,dt,b;
/* iterate - grid cell size */
        dx_blk = (octree_blocklist[i_block].XMAX - octree_blocklist[i_block].XMIN)/NX;
        dy_blk = (octree_blocklist[i_block].YMAX - octree_blocklist[i_block].YMIN)/NX;
        dz_blk = (octree_blocklist[i_block].ZMAX - octree_blocklist[i_block].ZMIN)/NX;
/* iteration step */
        dt = dx_blk;
	if (dy_blk < dt){dt=dy_blk;}
	if (dz_blk < dt){dt=dz_blk;}	
	dt = dt*dn;
	if (debug_trace == 1 && ( (step % debug_iout) == 0))
	  fprintf(stderr,"dx: %f %f %f DT: %f\n", dx_blk, dy_blk, dz_blk, dt);
	/* new between-block interpolator */
	bx = interpolate_amrdata(x,y,z,bxfield,1);
	by = interpolate_amrdata(x,y,z,byfield,0); /* new_position=0 - use cached block indices */
	bz = interpolate_amrdata(x,y,z,bzfield,0);  
	//	bx=interpolate_amrdata(x,y,z,"bx",v_indices[0],nf,fields,1);
	//by=interpolate_amrdata(x,y,z,"by",v_indices[1],nf,fields,0); /* new_position=0 - use cached block indices */
	//bz=interpolate_amrdata(x,y,z,"bz",v_indices[2],nf,fields,0);  
	if (debug_trace == 1 && ( (step % debug_iout) == 0))
	  fprintf(stderr,"X: %f Y: %f Z; %f  Bx: %f By %f Bz %f\n",
		  x, y, z, bx, by, bz);
	// add magnetic dipole field if needed
	if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x, y, z, n1, n2, n3, bdp);
	  bx += bx_dip;
	  by += by_dip;
	  bz += bz_dip;
	  }        
	b = sqrt(bx*bx+by*by+bz*bz);
	if (b < tiny) b = tiny;
	x2 = x+dt/2*bx/b;
	y2 = y+dt/2*by/b;
	z2 = z+dt/2*bz/b;
	/* new between-block interpolater */
	bx2 = interpolate_amrdata(x2,y2,z2,bxfield,1);
	by2 = interpolate_amrdata(x2,y2,z2,byfield,0);
	bz2 = interpolate_amrdata(x2,y2,z2,bzfield,0);  
	//bx2=interpolate_amrdata(x2,y2,z2,"bx",v_indices[0],nf,fields,1);
	//by2=interpolate_amrdata(x2,y2,z2,"by",v_indices[1],nf,fields,0);
	//bz2=interpolate_amrdata(x2,y2,z2,"bz",v_indices[2],nf,fields,0);  
	if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x2, y2, z2, n1, n2, n3, bdp);
	  bx2 += bx_dip;
	  by2 += by_dip;
	  bz2 += bz_dip;        
	}        
        b = sqrt(bx2*bx2+by2*by2+bz2*bz2);
        if (b < tiny) b = tiny;
        x2 = x+dt*bx2/b;
        y2 = y+dt*by2/b;
        z2 = z+dt*bz2/b;
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            fprintf(stderr,"Step: %ld X: %f Y %f Z %f, X2: %f Y2: %f Z2: %f\n  B: %f %f %f B2: %f %f %f\n",
                   step,x,y,z,x2,y2,z2,bx,by,bz,bx2,by2,bz2);
	// iteration stalled?
	if ( (fabs(x2 - x) < tiny) && (fabs(y2-y) < tiny)  && (fabs(z2-z) < tiny)   // completely stalled
	     ){
	  if (debug_trace){
	    fprintf(stderr,"DT: %f   X      =%f   Y     =%f  Z      =%f\n", dt, x, y, z);
	    fprintf(stderr,"DT: %f   X2     =%f   Y2    =%f  Z2     =%f\nTiny=%f\n", dt, x2, y2, z2, tiny);
	  }
	  i_block = -1; // terminate iteration
	} else {
	  i_block = find_octree_block(x,y,z,-1L,-1);
	}
	step++;
	flx[step] = x2;
	fly[step] = y2;
	flz[step] = z2;
	x = x2;
	y = y2;
	z = z2;
	if ( (step > 1) &&  ( sqrt(+(flx[step-2]-x)*(flx[step-2]-x)               // oscillating
				   +(fly[step-2]-y)*(fly[step-2]-y)
				   +(flz[step-2]-z)*(flz[step-2]-z) ) < (fabs(dt)/10) ) ){
	  fprintf(stderr,"Trace_fieldline_octree: found backtracking...\n");
	  fprintf(stderr,"I: %ld FLX[I-2]=%f FLY[I-2]=%f FLZ[I-2]=%f\n", step, flx[step-2], fly[step-2], flz[step-2]);
	  fprintf(stderr,"DT: %f   X2     =%f   Y2    =%f  Z2     =%f\nTiny=%f\n", dt, x, y, z, tiny);
	  i_block = -1; // terminate iteration
	}
    }
    status = 0; /* fieldline has returned to sphere with radius r_end */
/* reached boundary of box / failed to find block for current position */
    if ( (x*x+y*y+z*z) >= (1.1*r_end*r_end) ) status = 1;
/* ran out of allowed iterations */
    if (step == step_max[0]) status=2;
    
    step_max[0] = step; 
    
    if (debug_trace)
       fprintf(stderr,"Fieldline end: %f %f %f I_block: %ld Status: %ld Step: %ld, step_max: %ld r at end %f\n",
	       x, y, z, i_block, status, step, step_max[0], sqrt(x*x+y*y+z*z));

    return(status);
}


/********************************************************************************/
/* trace_fieldline_octree_spherical: fieldline tracer using octree structure of */
/* blocks and interpolate_amrdata to get data values SPHERICAL grid version!!!  */
/* we assume that longitudes run from 0 to 360 and latitudes from -90 rto 90    */
/*                            or from 0 to 2Pi and               -Pi/2 to +pi/2 */
/********************************************************************************/
IDL_LONG trace_fieldline_octree_spherical(float x_start,float y_start,float z_start,
                                float r_end,
                                float *bxfield, float *byfield, float *bzfield,
                                float *flx, float *fly, float *flz, IDL_LONG *step_max,
                                float dn, float bdp, float *tilt, float deg2rad)
{
    long int ivv, NV, NVV, i_block, step, ib, NULL_L=0, ONE_L=1;
    int ix, iy, iz, status;
    float x, y, z, x2, y2, z2, r, dx_blk, dy_blk, dz_blk, m_x, m_y, m_z;
    float bx_dip, by_dip, bz_dip, xx1, zz1, rr, rr5, sintilt, costilt, sintilt2, costilt2;
    float n1, n2, n3;
    float tiny=1e-5;
    float YMIN, YMAX, ZMIN, ZMAX, YRANGE, YRANGE_HALF, ZMIN_DOUBLE, ZMAX_DOUBLE;
    /* some useful constants */
    YMIN = 0;
    YMAX = 360.*deg2rad;
    ZMIN = -90*deg2rad;
    ZMAX = +90.*deg2rad;
    ZMIN_DOUBLE = 2*ZMIN;
    ZMAX_DOUBLE = 2*ZMAX;
    YRANGE = YMAX-YMIN;
    YRANGE_HALF = YRANGE/2;
    // setup dipole axis to compute dipole magnetic field 
    dipole_axis(&n1, &n2, &n3, tilt);

    // start up - grid size in block
    NV = NX*NY;
    NVV = NV*NZ;
    // start position
    x = x_start;
    y = y_start;
    z = z_start;
    // find block
    i_block = -2;
    i_block = find_octree_block(x,y,z,-2L,-1);
    if (debug_trace)
      fprintf(stderr,"Fieldline start: %f %f %f I_block: %ld N_blks: %ld R_end: %f\n",
           x_start, y_start, z_start, i_block, N_blks, r_end);
    // populate tracing variables
    ib = 0;
    step = 0;
    flx[0] = x_start;
    fly[0] = y_start;
    flz[0] = z_start;
    // tracer loop
    while ((i_block >=0) && (i_block < N_blks)
           && (x*x+y*y+z*z >= r_end*r_end)
           && (step < step_max[0]) ){
      long ivv0,ivv1,ivv2,ivv3,ivv4,ivv5,ivv6,ivv7;
      float bx,by,bz,bx2,by2,bz2,bdp_rr5,dt,b;
/* iterate - grid cell size */
        dx_blk = (octree_blocklist[i_block].XMAX - octree_blocklist[i_block].XMIN)/NX;
        dy_blk = (octree_blocklist[i_block].YMAX - octree_blocklist[i_block].YMIN)/NX;
        dz_blk = (octree_blocklist[i_block].ZMAX - octree_blocklist[i_block].ZMIN)/NX;
/* iteration step */
        dt = dx_blk;
	/*	if (dy_blk < dt){dt=dy_blk;}
	  if (dz_blk < dt){dt=dz_blk;}	 */
	if ( (x*sin(deg2rad*z)*deg2rad*dy_blk) < dt){
	  dt = x*sin(deg2rad*z)*deg2rad*dy_blk;
	}	  
	dt=dt*dn;
	if (debug_trace == 1 && ( (step % debug_iout) == 0))
	  fprintf(stderr,"dx: %f %f %f DT: %f\n", dx_blk, dy_blk, dz_blk, dt);
	/* new between-block interpolator */
	bx = interpolate_amrdata(x,y,z,bxfield,1);
	by = interpolate_amrdata(x,y,z,byfield,0); /* new_position=0 - use cached block indices */
	bz = interpolate_amrdata(x,y,z,bzfield,0);  
	//bx=interpolate_amrdata(x,y,z,"bx",v_indices[0],nf,fields,1);
	//by=interpolate_amrdata(x,y,z,"by",v_indices[1],nf,fields,0); /* new_position=0 - use cached block indices */
	//bz=interpolate_amrdata(x,y,z,"bz",v_indices[2],nf,fields,0);  
	if (debug_trace == 1 && ( (step % debug_iout) == 0))
	  fprintf(stderr,"X: %f Y: %f Z; %f  Bx: %f By %f Bz %f\n",
		  x, y, z, bx, by, bz);
	// add magnetic dipole field if needed
	if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x, y, z, n1, n2, n3, bdp);
	  bx += bx_dip;
	  by += by_dip;
	  bz += bz_dip;
	}        
	b = sqrt(bx*bx+by*by+bz*bz);
	if (b < tiny) b = tiny;
        x2 = x+dt/2*bx/b; 
        y2 = y+dt/2*by/(b*x*max(sin(deg2rad*z),1e-5) );
        z2 = z+dt/2*bz/(b*x);
	/* new between-block interpolater */
	bx2 = interpolate_amrdata(x2,y2,z2,bxfield,1);
	by2 = interpolate_amrdata(x2,y2,z2,byfield,0);
	bz2 = interpolate_amrdata(x2,y2,z2,bzfield,0);  
	if (bdp != 0.){
	  b_dipole(&bx_dip, &by_dip, &bz_dip, x2, y2, z2, n1, n2, n3, bdp);
	  bx2 += bx_dip;
	  by2 += by_dip;
	  bz2 += bz_dip;        
	}        
        b = sqrt(bx2*bx2+by2*by2+bz2*bz2);
        if (b < tiny) b = tiny;
        x2 = x+dt*bx/b; 
        y2 = y+dt*by/(b*x*max(sin(deg2rad*z),1e-5) );
        z2 = z+dt*bz/(b*x);
        if (debug_trace == 1 && ( (step % debug_iout) == 0))
            fprintf(stderr,"Step: %ld X: %f Y %f Z %f, X2: %f Y2: %f Z2: %f\n  B: %f %f %f B2: %f %f %f\n",
                   step, x, y, z, x2, y2, z2, bx, by, bz, bx2, by2, bz2);
	// iteration stalled?
	if ( (fabs(x2 - x) < tiny) && (fabs(y2-y) < tiny)  && (fabs(z2-z) < tiny)   // completely stalled
	     ){
	  if (debug_trace){
	    fprintf(stderr,"DT: %f   X      =%f   Y     =%f  Z      =%f\n", dt, x, y, z);
	    fprintf(stderr,"DT: %f   X2     =%f   Y2    =%f  Z2     =%f\nTiny=%f\n", dt, x2, y2, z2, tiny);
	  }
	  i_block=-1; // terminate iteration
	} else {
	  i_block = find_octree_block(x, y, z, -1L, -1);
	}
	step++;
	flx[step] = x2;
	fly[step] = y2;
	flz[step] = z2;
	x = x2;
	y = y2;
	z = z2;
        if (z < ZMIN ){
            z = ZMIN_DOUBLE-z;
            y += YRANGE_HALF*deg2rad;
        }
        if (z > (+90.*deg2rad)){
            z = ZMAX_DOUBLE-z;
            y += YRANGE_HALF*deg2rad;
        }
/* phi ("y") */
        if (y < YMIN ){ y += YRANGE; }        
        if (y > YMAX ){ y -= YRANGE; }

	if ( (step > 1) &&  ( sqrt(+(flx[step-2]-x)*(flx[step-2]-x)               // oscillating
				   +(fly[step-2]-y)*(fly[step-2]-y)
				   +(flz[step-2]-z)*(flz[step-2]-z) ) < (fabs(dt)/10) ) ){
	  fprintf(stderr,"Trace_fieldline_octree: found backtracking...\n");
	  fprintf(stderr,"I: %ld FLX[I-2]=%f FLY[I-2]=%f FLZ[I-2]=%f\n",step, flx[step-2], fly[step-2], flz[step-2]);
	  fprintf(stderr,"DT: %f   X2     =%f   Y2    =%f  Z2     =%f\nTiny=%f\n", dt, x, y, z, tiny);
	  i_block = -1; // terminate iteration
	}
    }
    status = 0; /* fieldline has returned to sphere with radius r_end */
/* reached boundary of box / failed to find block for current position */
    if ( x >= (1.05*r_end) ) status = 1;
/* ran out of allowed iterations */
    if (step == step_max[0]) status = 2;
    
    step_max[0] = step; 
    
    if (debug_trace)
       fprintf(stderr,"Fieldline end: %f %f %f I_block: %ld Status: %ld Step: %ld, step_max: %ld r at end %f\n",
	       x, y, z, i_block, status, step, step_max[0], sqrt(x*x+y*y+z*z));

    return(status);
}
