#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fl_extern.h"

#define no_idl

// #define DEBUG
// #define DEBUG2
// #define DEBUG3 
// #define debug_main
//#define test_with_missing  

/* interpolate_amrdata_IDL: IDL interface
   arguments:
   float *x_pos, float *y_pos, float *z_pos,float *output_value,
   char *variable_name, float *variable_data_array,
   octree_block *octree_blocklist,
   long *block_NX, long *block_NY, long *block_NZ,
   float *x_blk, float *y_blk, float *z_blk
*/

#define xyz_blk

#ifndef no_idl
IDL_LONG  interpolate_amrdata_IDL(int argc, char *argv[]) {
  float *xx,*yy,*zz;
  IDL_STRING *varname,*CDFname;
  float *field,*value_arr;
  IDL_LONG NV,iv;
  /* #ifdef xyz_blk */
  if ( (argc != 19)
#ifndef xyz_blk
/* allow 15 arguments (without x_blk,y_blk,z_blk,N_blks) */
       && (argc != 15) 
#endif
       ){
      fprintf(stderr,"Interpolate_amrdata: Number of arguments: %ld incorrect!\n",argc);
      return(1);
  }
/*#else
  if (argc != 15){
      fprintf(stderr,"Interpolate_amrdata: Number of arguments: %ld incorrect!\n",argc);
      return(1);
  }
  #endif */ 
/* array of X,Y,Z positions */  
  xx=(float*)argv[0];
  yy=(float*)argv[1];
  zz=(float*)argv[2];
/* array of returned values */  
  value_arr=(float*)argv[3];
/* length of above arrays */
  NV=((IDL_LONG*)argv[4])[0];
  varname=(IDL_STRING*)argv[5];
  CDFname=(IDL_STRING*)argv[6];
/* block and grid information needed */
/* if anyone is missing this routine should revert to reading the CDF file */
  field=(float*)argv[7];
  octree_blocklist=(octree_block*)argv[8]; /* array of 8*N_blks/7 structures */
  numparents_at_AMRlevel=(IDL_LONG*)argv[9];
  block_at_AMRlevel=(IDL_LONG*)argv[10];
/* block size NX,NY,NZ needed to determine valid index values */
  NX=((IDL_LONG*)argv[11])[0];
  NY=((IDL_LONG*)argv[12])[0];
  NZ=((IDL_LONG*)argv[13])[0];
  N_blks=((IDL_LONG*)argv[14])[0]; // now required

#ifdef debug_main
  fprintf(stderr,"Inputs 1-8: %f %f %f %f %ld %s %s %f\n",
         xx[0],yy[0],zz[0],value_arr[0],NV,
         (varname[0]).s,(CDFname[0]).s,(octree_blocklist[0]).XCenter);
#endif

#ifdef xyz_blk
/* plan to replace those with references top XMIN,XMAX, ...
   for each block in octree_blocklist */
  x_blk=(float*)argv[15];
  y_blk=(float*)argv[16];
  z_blk=(float*)argv[17];
#endif
/* N_blks needed to determine array layout of block_at_AMRlevel */
  if (argc == 20){
      N_blks=((IDL_LONG*)argv[18])[0];
      MISSING=((float*)argv[19])[0];
  }
  if (argc == 16){
      MISSING=((float*)argv[15])[0];
  }
  
#ifdef debug_main
  fprintf(stderr,"Inputs 9-13: %ld %ld %ld %ld\n",
         numparents_at_AMRlevel[0],block_at_AMRlevel[0],
         NX,NY,NZ);
  if (argc == 19){
      fprintf(stderr,"Last element of X_blk: %f Y_blk: %f Z_blk: %f\n",
             x_blk[NX*N_blks-1],y_blk[NY*N_blks-1],z_blk[NZ*N_blks-1]);
      fprintf(stderr,"N_blks: %ld\n",N_blks);
  }
  fprintf(stderr,"MISSING: %f\n",MISSING);
#endif  
/* CDF init code here if *field == NULL or any grid info is missing */
 
  for (iv=0;iv<NV;iv++){
      value_arr[iv]=interpolate_amrdata(xx[iv],yy[iv],zz[iv],
                                        (varname[0]).s,
                                        0,1,
                                        field,1L);
#ifdef debug_main
      fprintf(stderr,"IV; %ld Value: %f\n",iv,value_arr[iv]);
#endif
  }
  
  return(0);
}

/*****************************************************************/
/* return multiple variables with single call                    */
/* input: fields[nvar,nx,ny,nz,n_blks], N_var, N_var_ID          */
/*        -> var_array[NV,N_var]                                 */
/*****************************************************************/

IDL_LONG  interpolate_amrdata_multivar_IDL(int argc, char *argv[]) {
/* get multiple variables from data at given location(s) */
    float *xx,*yy,*zz;
  IDL_STRING *varname,*CDFname;
  float *field,*value_arr;
  IDL_LONG NV,iv,NVAR,*VAR_IDs,N_VAR_IDs,ivar;
#ifndef xyz_blk
  if (argc != 18){
#else
  if (argc != 21){
#endif
    fprintf(stderr,"Interpolate_amrdata: Number of arguments %ld incorrect!\n",argc);
    return(1);
  }
/* array of X,Y,Z positions */  
  xx=(float*)argv[0];
  yy=(float*)argv[1];
  zz=(float*)argv[2];
/* array of returned values */  
  value_arr=(float*)argv[3];
/* length of above arrays */
  NV=((IDL_LONG*)argv[4])[0];
/*  varname=(IDL_STRING*)argv[5];  */
  VAR_IDs=(IDL_LONG*)argv[5];
/* value_arr must be at least NV*N_VAR_IDs long */
  N_VAR_IDs=((IDL_LONG*)argv[6])[0]; 
  NVAR=((IDL_LONG*)argv[7])[0];
  CDFname=(IDL_STRING*)argv[8];
/*  CDFname=(IDL_STRING*)argv[6]; */
/* block and grid information needed */
/* if anyone is missing this routine should revert to reading the CDF file */
  field=(float*)argv[9];
  octree_blocklist=(octree_block*)argv[10]; /*array of 8*N_blks/7 structures */
  numparents_at_AMRlevel=(IDL_LONG*)argv[11];
  block_at_AMRlevel=(IDL_LONG*)argv[12];
/* block size NX,NY,NZ needed to determine valid index values */
  NX=((IDL_LONG*)argv[13])[0];
  NY=((IDL_LONG*)argv[14])[0];
  NZ=((IDL_LONG*)argv[15])[0];
  N_blks=((IDL_LONG*)argv[16])[0];
#ifndef xyz_blk
  MISSING=((float*)argv[17])[0];
#else
  x_blk=(float*)argv[17];
  y_blk=(float*)argv[18];
  z_blk=(float*)argv[19];
  MISSING=((float*)argv[20])[0];
#endif
#ifdef debug_main
  fprintf(stderr,"Inputs 1-8: %f %f %f %f %i %f\n",
         xx[0],yy[0],zz[0],value_arr[0],NV,
         (octree_blocklist[0]).XCenter);
  fprintf(stderr,"MISSING:%f\n",MISSING);
  fprintf(stderr,"NUMPARENTS: %i  Parent block 0: %i\n",
	  numparents_at_AMRlevel[0],
	  block_at_AMRlevel[0]);
#endif

#ifdef debug_main
  fprintf(stderr,"NVAR %ld N_IDs %i ID_arr[0] %i ",
         NVAR,N_VAR_IDs,VAR_IDs[0]);
  for (ivar=1;ivar<N_VAR_IDs;ivar++){
    fprintf(stderr,"%i ",VAR_IDs[ivar]);
  }
  fprintf(stderr,"\n");
#endif

/* CDF init code here if *field == NULL or any grid info is missing */
/* multiple variables returned - position them into separate fields
   of length NV  */
  for (iv=0;iv<NV;iv++){
      if (VAR_IDs[0] > -1){
        value_arr[iv*NVAR]=interpolate_amrdata(xx[iv],yy[iv],zz[iv],"",
                                        VAR_IDs[0],NVAR,field,1);

      } else {
        value_arr[iv*NVAR]=MISSING;
      }
      for (ivar=1;ivar<N_VAR_IDs;ivar++) {
        if (VAR_IDs[ivar] > -1){
          value_arr[ivar+iv*NVAR]=
              interpolate_amrdata(xx[iv],yy[iv],zz[iv],"xx",
                                  VAR_IDs[ivar],NVAR,field,0);
	} else {
	  value_arr[ivar+iv*NVAR]=MISSING;
	}
      } 
      
#ifdef debug_main
      fprintf(stderr,"IV: %ld X: %f Y: %f Z: %f Value: %f\n",
	      iv,xx[iv],yy[iv],zz[iv],value_arr[iv*NVAR]);
#endif
  }
  
  return(0);
}
#endif  // end #ifndef no_idl block
/*********************************************************/
/* interpolate_amrdata                                   */
/* interpolate BATSRUS block adaptive grid data by using */
/* grid positions in neighboring blocks if necessary     */
/* finds blocks using octree data structure              */
/* global variables: NX,NY,NZ,N_blks,x_blk,y_blk,z_blk   */
/*                   MISSING,xr_blk,yr_blk,zr_blk        */
/* Author: Lutz Rastaetter, CCMC, August 3, 2004         */
/* Revision history:                                     */
/*   Aug. 17, 2004: stable version that works with       */
 /*                  field line tracer "fl_int"           */
/*                                                       */
/*********************************************************/

/* method of interpolation: pair-wise averaging in X,Y,then Z */
/* invalid cells are discarded */
#ifdef no_idl
  float interpolate_amrdata(float xx,float yy,float zz,
			   //                          char *variable_name,
			    //                          IDL_LONG VAR_ID, /* offset in fields array */
			    //                          IDL_LONG NVAR, /* stride in fields array */
                          float *field, IDL_LONG new_position)
{
  IDL_LONG VAR_ID=0,NVAR=1; // only accepting data and request for a single quantity
#else
  float interpolate_amrdata(float xx,float yy,float zz,
                          octree_block *octree_blocklist_in,
                          IDL_LONG VAR_ID, /* offset in fields array */
                          IDL_LONG NVAR, /* stride in fields array */
                          float *field, IDL_LONG new_position)
  {
    octree_blocklist=octree_blocklist_in;  
#endif
/* cache position information for repeat calls for different variables at same position */
    static IDL_LONG ib=-1,ibc,ib_c[8],ix_c[8],iy_c[8],iz_c[8];
    static float xx_c[8],yy_c[8],zz_c[8],dx,dy,dz;
    static int valid_c[8],valid,have_cell_corners=0;
/* temporary data and indices */
    IDL_LONG i,ic,new_blk[8];
    float ixx,iyy,izz,dx2,dy2,dz2,next_block_offset,inside_block_offset; 
    float data_c[8],d_m1,d_m2,yy_c2[4],zz_c2[4];
    float XMIN,XMAX,YMIN,YMAX,ZMIN,ZMAX;
#ifdef test_with_missing
    float MISSING_2;
#endif
    MISSING=NAN;

#ifdef DEBUG2
    ib=1959;
    fprintf(stderr,"Var_ID: %ld NVAR: %ld\n",VAR_ID,NVAR);
    fprintf(stderr,"octree_blocklist: %p\n",octree_blocklist);
    fprintf(stderr,"New position: %i\n",new_position);
    fprintf(stderr,"octree_blocklist[%i].XMIN: %f\n",ib,octree_blocklist[ib].XMIN);
    fprintf(stderr,"octree_blocklist[%i].XMAX: %f\n",ib,octree_blocklist[ib].XMAX);
    fprintf(stderr,"octree_blocklist[%i].YMIN: %f\n",ib,octree_blocklist[ib].YMIN);
    fprintf(stderr,"octree_blocklist[%i].YMAX: %f\n",ib,octree_blocklist[ib].YMAX);
    fprintf(stderr,"octree_blocklist[%i].ZMIN: %f\n",ib,octree_blocklist[ib].ZMIN);
    fprintf(stderr,"octree_blocklist[%i].ZMAX: %f\n",ib,octree_blocklist[ib].ZMAX);
    fprintf(stderr,"octree_blocklist[%i].child_count: %i  child_ids: %i %i %i %i %i %i %i %i\n",ib,octree_blocklist[0].child_count,octree_blocklist[0].child_IDs[0],octree_blocklist[ib].child_IDs[1],octree_blocklist[ib].child_IDs[2],octree_blocklist[ib].child_IDs[3],octree_blocklist[ib].child_IDs[4],octree_blocklist[ib].child_IDs[5],octree_blocklist[ib].child_IDs[6],octree_blocklist[ib].child_IDs[7]);

    //    return(0.);

#endif
/************************************/
/* calculate all neighboring points */
/************************************/
    if (new_position) {
#ifdef xyz_blk
      have_cell_corners=(octree_blocklist[0]).XMIN == x_blk[0];
      next_block_offset=0.5*(have_cell_corners == 0)+0.5;
      inside_block_offset=0.5*(have_cell_corners == 0);
#else
      // we cannot tell
      // have_cell_corners=0;
      // next_block_offset=1.0;
      have_cell_corners=1;
      next_block_offset=0.5;
      inside_block_offset=0.5*(have_cell_corners == 0);
#endif
#ifdef debug_main
      fprintf(stderr,"have_cell_corners: %i next_block_offset: %f inside_block_offset: %f\n",have_cell_corner,next_block_offset, inside_block_offset);
      //      fprintf(stderr,"Block 0: XMIN: %f x[0]: %f\n",(octree_blocklist[0]).XMIN,x_blk[0]);
      fprintf(stderr,"XX: %f  YY: %f  ZZ: %f\n",x,yy,zz);
#endif
      ib=find_octree_block(xx,yy,zz,-1,-1);
#ifdef debug3
      fprintf(stderr,"Block number ib: %i\n",ib);
      return(0.);
#endif
//#define debug_main
#ifdef debug_main
      fprintf(stderr,"Block ID: 0 <= %i < %i  Have Cell Corners: %i\n",ib,N_blks,have_cell_corners);
      //      fprintf(stderr,"Block 0: XMIN: %f x[0]: %f\n",(octree_blocklist[0]).XMIN,x_blk[0]);
#endif

      if (ib < 0 || ib >= N_blks) return(MISSING); /* outside of range */

      XMIN= (octree_blocklist[ib]).XMIN;
      XMAX= (octree_blocklist[ib]).XMAX;
      YMIN= (octree_blocklist[ib]).YMIN;
      YMAX= (octree_blocklist[ib]).YMAX;
      ZMIN= (octree_blocklist[ib]).ZMIN;
      ZMAX= (octree_blocklist[ib]).ZMAX;
#ifdef DEBUG2
      fprintf(stderr,"NX,NY,NZ: %ld %ld %ld  XMIN,XMAX: %f %f YMIN,YMAX: %f %f ZMIN<ZMAX: %f %f\nBlock: %ld Missing value: %f\nVariable ID: %i\n",
	      NX,NY,NZ,
	      XMIN,XMAX,YMIN,YMAX,ZMIN,ZMAX,
	      ib,MISSING,VAR_ID
	      );
#endif
    
/* get position in block assuming equidistant grid in block */
/*   find_in_block(xx,yy,zz,&ix,&iy,&iz,x_blk+ib*NX,y_blk+ib*NY,z_blk+ib*NZ,NX,NY,NZ); */
#ifdef xyz_blk
        dx=x_blk[NX*ib+1]-x_blk[NX*ib];
        dy=y_blk[NY*ib+1]-y_blk[NY*ib];
        dz=z_blk[NZ*ib+1]-z_blk[NZ*ib]; 
        
        ixx = (xx-x_blk[NX*ib])/dx;
        iyy = (yy-y_blk[NY*ib])/dy;
        izz = (zz-z_blk[NZ*ib])/dz;
#else
        dx=(XMAX - XMIN)/NX;
        dy=(YMAX - YMIN)/NY;
        dz=(ZMAX - ZMIN)/NZ;
        
        ixx = (xx-XMIN)/dx-0.5;
        iyy = (yy-YMIN)/dy-0.5;
        izz = (zz-ZMIN)/dz-0.5;
#endif
        
/* indices of grid positions around sample location */
// new: accommodate grid with cell corners
	int ixx_fl,iyy_fl,izz_fl;
	ixx_fl=floor(ixx);
	iyy_fl=floor(iyy);
	izz_fl=floor(izz);
	if (have_cell_corners){
	  ixx_fl=max(0,min(NX-2,ixx_fl));
	  iyy_fl=max(0,min(NY-2,iyy_fl));
	  izz_fl=max(0,min(NZ-2,izz_fl));
	}
        ix_c[0]=ix_c[2]=ix_c[4]=ix_c[6]=ixx_fl;;
        ix_c[1]=ix_c[3]=ix_c[5]=ix_c[7]=ixx_fl+1;
        iy_c[0]=iy_c[1]=iy_c[4]=iy_c[5]=iyy_fl;
        iy_c[2]=iy_c[3]=iy_c[6]=iy_c[7]=iyy_fl+1;
        iz_c[0]=iz_c[1]=iz_c[2]=iz_c[3]=izz_fl;
        iz_c[4]=iz_c[5]=iz_c[6]=iz_c[7]=izz_fl+1;
#ifdef DEBUG
        fprintf(stderr,"Position in block %f %f %f\n",xx,yy,zz);
        fprintf(stderr,"indices in block %f %f %f\n",ixx,iyy,izz);
        fprintf(stderr,"Modified indices %i %i %i\n",ixx_fl,iyy_fl,izz_fl);
#endif

        for (ic=0;ic<8;ic++){
            ib_c[ic] = ib;    
#ifdef xyz_blk
            xx_c[ic] = x_blk[NX*ib]+ix_c[ic]*dx;
            yy_c[ic] = y_blk[NY*ib]+iy_c[ic]*dy;
            zz_c[ic] = z_blk[NZ*ib]+iz_c[ic]*dz; 
#else
            xx_c[ic] = XMIN+(inside_block_offset+ix_c[ic])*dx;
            yy_c[ic] = YMIN+(inside_block_offset+iy_c[ic])*dy;
            zz_c[ic] = ZMIN+(inside_block_offset+iz_c[ic])*dz;
#endif
            new_blk[ic]=0;            
/* validate positions in block and find new block as necessary */
            if (ix_c[ic] < 0.){
                new_blk[ic] = 1;
#ifdef xyz_blk
                xx_c[ic] = x_blk[NX*ib]-next_block_offset*dx;
#else
                xx_c[ic] = XMIN-0.5*dx;
#endif
            } else {
                if (ix_c[ic] > (NX-0.5)) {
                    new_blk[ic] = 1;
#ifdef xyz_blk
                    xx_c[ic] = x_blk[NX-1+NX*ib]+next_block_offset*dx; 
#else
                    xx_c[ic] = XMAX+0.5*dx;
#endif
                } else {
#ifdef xyz_blk
                    xx_c[ic] = x_blk[ix_c[ic]+NX*ib];
#else
                    xx_c[ic] = XMIN+(inside_block_offset+ix_c[ic])*dx; 
#endif
                }
            }
            
            if (iy_c[ic] < 0) {
                new_blk[ic] = 1;              
#ifdef xyz_blk
                yy_c[ic] = y_blk[NY*ib]-next_block_offset*dy;
#else
                yy_c[ic] = YMIN-0.5*dy;
#endif
            } else {
                if (iy_c[ic] > (NY-1) ) {
                    new_blk[ic] = 1;
#ifdef xyz_blk
                    yy_c[ic] = y_blk[NY-1+NY*ib]+next_block_offset*dy; 
#else
                    yy_c[ic] = YMAX+0.5*dy;
#endif
                } else {
#ifdef xyz_blk
                    yy_c[ic] = y_blk[iy_c[ic]+NY*ib];
#else
                    yy_c[ic] = YMIN+(inside_block_offset+iy_c[ic])*dy;
#endif
                }
            }
            if (iz_c[ic] < 0) {
                new_blk[ic] = 1;
#ifdef xyz_blk
                zz_c[ic] = z_blk[NZ*ib]-next_block_offset*dz;
#else
                zz_c[ic] = ZMIN-0.5*dz;
#endif
            } else {  
                if (iz_c[ic] > (NZ-1) ){
                    new_blk[ic] = 1;
#ifdef xyz_blk
                    zz_c[ic] = z_blk[NZ-1+NZ*ib]+next_block_offset*dz; 
#else
                    zz_c[ic] = ZMAX+0.5*dz;
#endif
                } else {
#ifdef xyz_blk
                    zz_c[ic] = z_blk[iz_c[ic]+NZ*ib]; 
#else
                    zz_c[ic] = ZMIN+(inside_block_offset+iz_c[ic])*dz;
#endif
                }
            }
#ifdef DEBUG
            fprintf(stderr,"ic: %ld Block: %ld ixyz_c: %i %i %i XYZ_c: %f %f %f\n",
                   ic, ib_c[ic], ix_c[ic], iy_c[ic], iz_c[ic],
                   xx_c[ic], yy_c[ic], zz_c[ic]);
#endif
        } /* end for ic=0..7 */

#ifdef DEBUG3
       fprintf(stderr,"XYZ: %f %f %f\n",xx,yy,zz);
#endif
        for (ic=0; ic<8; ic++){
	    valid_c[ic] = 1;
            if (new_blk[ic]) {
                long ibc;
                ibc=find_octree_block(xx_c[ic], yy_c[ic], zz_c[ic], -1L, -1);
#ifdef DEBUG2
                fprintf(stderr,"New block: %ld XYZ: %f %f %f\n",
                        ibc, xx_c[ic], yy_c[ic], zz_c[ic]);
#endif
/* ; now snap into position at new block cell */
#ifdef DEBUG
                fprintf(stderr,"New block: %ld\n",ibc);
#endif
                if (ibc >= 0 && ibc < N_blks) { // we may allow for blocks with incomplete child count
                    ib_c[ic] = ibc; /* change block number */
#ifdef xyz_blk
                    dx2 = x_blk[NX*ibc+1]-x_blk[NX*ibc];
                    dy2 = y_blk[NY*ibc+1]-y_blk[NY*ibc];
                    dz2 = z_blk[NZ*ibc+1]-z_blk[NZ*ibc];
#else
                    XMIN = (octree_blocklist[ibc]).XMIN;
                    XMAX = (octree_blocklist[ibc]).XMAX;
                    YMIN = (octree_blocklist[ibc]).YMIN;
                    YMAX = (octree_blocklist[ibc]).YMAX;
                    ZMIN = (octree_blocklist[ibc]).ZMIN;
                    ZMAX = (octree_blocklist[ibc]).ZMAX;
                    dx2 = (XMAX - XMIN)/(NX-have_cell_corners);
                    dy2 = (YMAX - YMIN)/(NY-have_cell_corners);
		    dz2 = (ZMAX - ZMIN)/(NZ-have_cell_corners);
#endif
                    xx_c[ic] = xx+(    ic   % 2)*dx2; /* adjust stencil with */
                    yy_c[ic] = yy+((ic % 4) / 2)*dy2; /* resolution change between */
                    zz_c[ic] = zz+(    ic   / 4)*dz2; /* neighboring blocks */
#ifdef xyz_blk
                    ix_c[ic] = min(NX-1,max(0,floor((xx_c[ic]-x_blk[NX*ibc])/dx2)));
                    iy_c[ic] = min(NY-1,max(0,floor((yy_c[ic]-y_blk[NY*ibc])/dy2)));
                    iz_c[ic] = min(NZ-1,max(0,floor((zz_c[ic]-z_blk[NZ*ibc])/dz2)));
                    xx_c[ic] = x_blk[ix_c[ic]+NX*ib_c[ic]];
                    yy_c[ic] = y_blk[iy_c[ic]+NY*ib_c[ic]];
                    zz_c[ic] = z_blk[iz_c[ic]+NZ*ib_c[ic]];
#else
                    ix_c[ic] = min(NX-1,max(0,floor((xx_c[ic]-XMIN)/dx2 - 0.5)));
                    iy_c[ic] = min(NY-1,max(0,floor((yy_c[ic]-YMIN)/dy2 - 0.5)));
                    iz_c[ic] = min(NZ-1,max(0,floor((zz_c[ic]-ZMIN)/dz2 - 0.5)));
                    xx_c[ic] = XMIN+dx2*(inside_block_offset+ix_c[ic]); 
                    yy_c[ic] = YMIN+dx2*(inside_block_offset+iy_c[ic]); 
                    zz_c[ic] = ZMIN+dx2*(inside_block_offset+iz_c[ic]);
#endif
                } else {
		  /* invalidate points outside domain */
		    valid_c[ic] = 0;
/* point to some grid point near the position (xx,yy,zz) */
                    ib_c[ic] = ib;
                    ix_c[ic] = NX/2;
                    iy_c[ic] = NY/2;
                    iz_c[ic] = NZ/2;
/* throw out those points by moving them off */
                    xx_c[ic] = fabs(MISSING); /*x_blk[ib*NX+ix_c[ic]]; */
                    yy_c[ic] = fabs(MISSING); /*y_blk[ib*NY+iy_c[ic]]; */
                    zz_c[ic] = fabs(MISSING); /*z_blk[ib*NZ+iz_c[ic]]; */
                }
#ifdef DEBUG2
                fprintf(stderr,"IC: %ld XYZ_c: %f %f %f\n", ic, xx_c[ic], yy_c[ic], zz_c[ic]);
#endif
            }
        }    
    
#ifdef DEBUG2
        fprintf(stderr,"Interpolate_amrdata: %f %f %f %ld\n", xx, yy, zz, ib);
#endif
/* end if new_position */
    } else {
        if (ib < 0) {return(MISSING);} /* outside of range */
    }
#ifdef test_with_missing
    MISSING_2 = 7*MISSING/8;
#endif
/************************************/
/* retrieve data                    */
/************************************/
    if (field != NULL){
        for (ic=0;ic<8;ic++){
            data_c[ic] = field[VAR_ID
                     +NVAR*(ix_c[ic]+NX*(iy_c[ic]+NY*(iz_c[ic]+NZ*ib_c[ic])) )
            ];
#ifdef test_with_missing
	    valid_c[ic] = valid_c[ic]*(data_c[ic] > MISSING_2); 
#endif
#ifdef DEBUG2
            fprintf(stderr,"Corners: %f %f %f %f\n",
                    xx_c[ic], yy_c[ic], zz_c[ic], data_c[ic]);
#endif
        }
    }
#ifdef DEBUG3
    if (VAR_ID == 0) {
      fprintf(stderr,"Data=[%f, %f, %f, %f, %f, %f, %f, %f]\n",
	     data_c[0], data_c[1], data_c[2], data_c[3],
	     data_c[4], data_c[5], data_c[6], data_c[7]);
      fprintf(stderr,"XX=[ %f, %f, %f, %f, %f, %f, %f, %f]\n",
	     xx_c[0], xx_c[1], xx_c[2], xx_c[3],
	     xx_c[4], xx_c[5], xx_c[6], xx_c[7]);
      fprintf(stderr,"YY=[ %f, %f, %f, %f, %f, %f, %f, %f]\n",
	     yy_c[0], yy_c[1], yy_c[2], yy_c[3],
	     yy_c[4], yy_c[5], yy_c[6], yy_c[7]);
      fprintf(stderr,"ZZ=[ %f, %f, %f, %f, %f, %f, %f, %f]\n",
	     zz_c[0], zz_c[1], zz_c[2], zz_c[3],
	     zz_c[4], zz_c[5], zz_c[6], zz_c[7]);
    }
#endif
/************************************/
/* do pair-wise interpolations      */
/* in x,y and z directions          */
/************************************/
    for (ic=0; ic<4; ic++){
        int ic2, ic2_1;
        ic2 = 2*ic;
        ic2_1 = ic2+1;
        d_m1 = (xx-xx_c[ic2]);
        d_m2 = (xx_c[ic2_1]-xx);
#ifdef DEBUG2
        fprintf(stderr,"Interp_x: X: %f %f dx: %f %f data: %f %f\n",
                xx_c[ic2], xx_c[ic2_1], d_m1, d_m2, data_c[ic2], data_c[ic2_1]);
#endif

        if (
#ifdef test_with_missing
            (valid_c[ic2]) && (valid_c[ic2_1])  &&
#endif
            (fabs(d_m1+d_m2) > (dx/4.) ) 
	    ){
            data_c[ic] = (d_m2*data_c[ic2]+d_m1*data_c[ic2_1])
                /(d_m1+d_m2);
            yy_c2[ic] = (d_m2*yy_c[ic2]+d_m1*yy_c[ic2_1])
                /(d_m1+d_m2);
            zz_c2[ic] = (d_m2*zz_c[ic2]+d_m1*zz_c[ic2_1])
                /(d_m1+d_m2);
	    valid=1;
        } else {
	    valid=0;
	    if (valid_c[ic2_1]) {
	        data_c[ic] = data_c[ic2_1];
                yy_c2[ic] = yy_c[ic2_1];
                zz_c2[ic] = zz_c[ic2_1];
		valid = 1;
            } 
	    if (valid_c[ic2]){
                data_c[ic] = data_c[ic2];
                yy_c2[ic] = yy_c[ic2];
                zz_c2[ic] = zz_c[ic2];
		valid = 1;
	    } 
	}
	valid_c[ic] = valid;
    }
    
    for (ic=0; ic<2; ic++){
        int ic2, ic2_1;
        ic2 = 2*ic;
        ic2_1 = ic2+1;
        d_m1 = (yy-yy_c2[ic2]);
        d_m2 = (yy_c2[ic2_1]-yy);
#ifdef DEBUG2
        fprintf(stderr,"Interp_y: Y: %f %f dy: %f %f data: %f %f\n",
                yy_c2[ic2], yy_c2[ic2_1], d_m1, d_m2, data_c[ic2], data_c[ic2_1]);
#endif
        if (
#ifdef test_with_missing
            (valid_c[ic2]) && (valid_c[ic2_1]) &&
#endif
            (fabs(d_m1+d_m2) >= (dy/4.)) ){  
            zz_c2[ic] = (d_m2*zz_c2[ic2]+d_m1*zz_c2[ic2_1])
                /(d_m1+d_m2);
            data_c[ic] = (d_m2*data_c[ic2]+d_m1*data_c[ic2_1])
                /(d_m1+d_m2);
	    valid = 1;
        } else {
	    valid = 0;
            if (valid_c[ic2_1]) {
                data_c[ic] = data_c[ic2_1];
                zz_c2[ic] = zz_c2[ic2_1];
		valid = 1;
	    } 
	    if (valid_c[ic2]) {
                data_c[ic] = data_c[ic2];
                zz_c2[ic] = zz_c2[ic2];
		valid = 1;
            }                
        }
        valid_c[ic] = valid;
    }
    d_m1 = (zz-zz_c2[0]);
    d_m2 = (zz_c2[1]-zz);
#ifdef DEBUG2
    fprintf(stderr,"Interp_z: Z: %f %f dz: %f %f data: %f %f\n",
            zz_c2[0], zz_c2[1], d_m1, d_m2, data_c[0], data_c[1]);
#endif    

/**************************************************/
/* return result of interpolation into data_c[0]  */
/**************************************************/
    if ( 
#ifdef test_with_missing
	(valid_c[0]) && (valid_c[1]) &&
#endif
        (fabs(d_m1+d_m2) >= (dz/4.))  ){  
        data_c[0]=((d_m2*data_c[0]+d_m1*data_c[1])/(d_m1+d_m2));
	valid = 1;
    } else {
      valid=0;
        if (valid_c[1]) {
            data_c[0] = data_c[1];
	    valid = 1;
        }
	if (valid_c[0]){
	    valid = 1;
	}
    }
#ifdef DEBUG2
    fprintf(stderr,"Interp: Z: %f %f dz: %f %f data: %f\n",
            zz_c2[0], zz_c2[1], d_m1, d_m2, data_c[0]);
#endif        
#ifdef DEBUG3
    fprintf(stderr,"Interp=%f Valid=%ld\n", data_c[0], valid);
#endif        
    if (valid) {
      return(data_c[0]);
    } else {
      return(MISSING);
    }
}
