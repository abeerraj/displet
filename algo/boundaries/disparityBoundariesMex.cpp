#include <iostream>
#include <vector>
#include <math.h>

#include "mex.h"

using namespace std;

void disparityBoundaries (double* D, double* B, const int *dims) {

    // parameters
    int w = dims[1];
    int h = dims[0];
    int r = 3;
    double tau1 = 3;
    double tau2 = 10;
    double d1,d2;
    
    // for all pixels do
    for (int u=r; u<w-r; u++) {
        for (int v=r; v<h-r; v++) {
            
            // get left disparity value
            d1=-1;            
            for (int u2=u-1; u2>=u-r; u2--) {
                if (D[u2*h+v]>0) {
                    d1 = D[u2*h+v];
                    break;
                }
            }
            
            // get right disparity value
            d2=-1;
            for (int u2=u+1; u2<=u+r; u2++) {
                if (D[u2*h+v]>0) {
                    d2 = D[u2*h+v];
                    break;
                }
            }
            
            // update B
            if (d1>0 && d2>0)
                B[u*h+v] = max(B[u*h+v],min(fabs(d1-d2),tau2)-tau1);
            
            // get top disparity value
            d1=-1;            
            for (int v2=v-1; v2>=v-r; v2--) {
                if (D[u*h+v2]>0) {
                    d1 = D[u*h+v2];
                    break;
                }
            }
            
            // get bottom disparity value
            d2=-1;
            for (int v2=v+1; v2<=v+r; v2++) {
                if (D[u*h+v2]>0) {
                    d2 = D[u*h+v2];
                    break;
                }
            }
            
            // update B
            if (d1>0 && d2>0)
                B[u*h+v] = max(B[u*h+v],min(fabs(d1-d2),tau2)-tau1);
        }
    }
}

void mexFunction (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    // check for proper number of arguments
    if(nrhs!=1) 
        mexErrMsgTxt("1 input required (D).");
    if(nlhs!=1) 
        mexErrMsgTxt("1 output required (B).");
    
    // check for proper argument types and sizes
    if(!mxIsDouble(prhs[0]) || mxGetNumberOfDimensions(prhs[0])!=2)
      mexErrMsgTxt("Input D must be a double disparity image.");
    
    // get input
    double*   D     = (double*)mxGetPr(prhs[0]);
    const int *dims = mxGetDimensions(prhs[0]);

    // create output
    plhs[0]   = mxCreateNumericArray(2,dims,mxDOUBLE_CLASS,mxREAL);
    double* B = (double*)mxGetPr(plhs[0]);

    // do computation
    disparityBoundaries(D,B,dims);
}
