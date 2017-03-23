#include <mex.h>
#include <vector>
#include <algorithm> // for set_intersection
#include <math.h>    // for M_PI
#include <cmath>     // for abs(double)

using namespace std;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  
  if (nrhs!=4)
    mexErrMsgTxt("4 inputs required: SP_adj, Normals, nStates, clip_val]");
  if (nlhs!=1)
    mexErrMsgTxt("1 output required (E_pair)");
  if (!mxIsDouble(prhs[0]))
    mexErrMsgTxt("SP_adj must be a double matrix");
  if (!mxIsDouble(prhs[1]))
    mexErrMsgTxt("Normals must be a double matrix");
  
  int ns = mxGetN(prhs[0]); // number of superpixels
  
  // get pointers to input
  double *SP_adj = (double*)mxGetPr(prhs[0]);
  double *Normals = (double*)mxGetPr(prhs[1]);
  double nStates_ = *((double*) mxGetPr(prhs[2]));
  double clip_val = *((double*)mxGetPr(prhs[3]));
  
  int nStates = (int) nStates_;
  
  // create output cell array (pairwise energies)
  int E_pair_dims[2] = {ns,ns};
  plhs[0] = mxCreateCellArray(2,E_pair_dims);

  // for all superpixels do
  for (int i=0; i<ns; i++) {

    // for all other superpixels do
    for (int j=i+1; j<ns; j++) {
      
      // if superpixels are adjacent
      if (SP_adj[j*ns+i]>0.5) {
        // create output
        int E_dims[2] = {nStates,nStates};
        mxArray* E = mxCreateNumericArray(2,E_dims,mxDOUBLE_CLASS,mxREAL);
        double* E_vals = (double*)mxGetPr(E);

        // for all planes in first superpixel do
        for (int pi=0; pi<nStates; pi++) {
          
          // extract plane parameters for first superpixel
          int pi_ = i*nStates+pi;
          pi_ = 3 * pi_;
          double n1_x = Normals[pi_];
          double n1_y = Normals[pi_+1];
          double n1_z = Normals[pi_+2];
          
          // for all planes in second superpixel do
          for (int pj=0; pj<nStates; pj++) {
          
            // extract plane parameters for second superpixel
            int pj_ = j*nStates+pj;
            pj_ = 3 * pj_; 
            double n2_x = Normals[pj_];
            double n2_y = Normals[pj_+1];
            double n2_z = Normals[pj_+2];
            
            double cos_sim = (n1_x*n2_x + n1_y*n2_y + n1_z*n2_z) / (sqrt(n1_x*n1_x + n1_y*n1_y + n1_z*n1_z) * sqrt(n2_x*n2_x + n2_y*n2_y + n2_z*n2_z));
            double d = 0.5*(1-cos_sim);
            E_vals[pj*nStates+pi] = (d < clip_val ? d : (d-clip_val)*0.2 + clip_val);
          }
        }

        // set energy values in cell corresponding to superpixel (i,j)
        int subs[2] = {i,j};
        int idx = mxCalcSingleSubscript(plhs[0],2,subs);
        mxSetCell(plhs[0],idx,E);
      }
    }
  }
}
