
void find_in_block(float x, float y, float z,
                   int *ix, int *iy, int *iz,
                   const float *x_blk, const float *y_blk, const float *z_blk,
                   int NX, int NY, int NZ)
{
    int ixt,iyt,izt;
    int NX1,NY1,NZ1;

    NX1 = NX-1;
    NY1 = NY-1;
    NZ1 = NZ-1;
    ixt = 1;
    iyt = 1;
    izt = 1;
    while((x_blk[ixt] < x) && (ixt < NX1)) {ixt++;}
    while((y_blk[iyt] < y) && (iyt < NY1)) {iyt++;}
    while((z_blk[izt] < z) && (izt < NZ1)) {izt++;}
    ix[0] = ixt-1;
    iy[0] = iyt-1;
    iz[0] = izt-1;
}
