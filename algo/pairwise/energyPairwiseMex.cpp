#include <mex.h>
#include <vector>
#include <algorithm> // for set_intersection
#include <cmath>     // for abs(double)

using namespace std;

// extracts boundary pixels to vector<int> (0-based)
inline vector<int> boundaryPixels (const mxArray *SP_bnd, int32_t idx) {
  mxArray *SP_bnd_idx = mxGetCell(SP_bnd,idx);
  int32_t n = mxGetM(SP_bnd_idx);
  double* vals = (double*)mxGetPr(SP_bnd_idx);
  vector<int> bp; bp.resize(n);
  for (int i=0; i<n; i++)
    bp[i] = vals[i]-1;
  return bp;
}

inline double disparity (double u, double v, double a, double b, double c) {
  double d = a*u + b*v + c;
  return d;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  
  if (nrhs!=6)
    mexErrMsgTxt("6 inputs required: SP_adj, SP_bnd, Planes, nStates, clip_val, img_height]");
  if (nlhs!=1)
    mexErrMsgTxt("1 output required (E_pair)");
  if (!mxIsDouble(prhs[0]))
    mexErrMsgTxt("SP_adj must be a double matrix");
    if (!mxIsCell(prhs[1]))
    mexErrMsgTxt("SP_bnd must be a cell array");
  if (!mxIsDouble(prhs[2]))
    mexErrMsgTxt("Planes must be a double matrix");
  
  int ns = mxGetM(prhs[0]); // number of superpixels
  
  // get pointers to input
  double *SP_adj = (double*)mxGetPr(prhs[0]);
  const mxArray *SP_bnd = prhs[1];
  double *Planes = (double*)mxGetPr(prhs[2]);
  double nStates_ = *((double*) mxGetPr(prhs[3]));
  double clip_val = *((double*)mxGetPr(prhs[4]));
  double img_height_ = *((double*) mxGetPr(prhs[5]));
  
  int nStates = (int) nStates_;
  int img_height = (int) img_height_;
  
  // create output cell array (pairwise energies)
  int E_pair_dims[2] = {ns,ns};
  plhs[0] = mxCreateCellArray(2,E_pair_dims);

  // for all superpixels do
  for (int i=0; i<ns; i++) {
    
    // get boundary pixel indices for superpixel i
    vector<int> bp_i = boundaryPixels(SP_bnd,i);

    // for all other superpixels do
    for (int j=i+1; j<ns; j++) {
      
      // if superpixels are adjacent
      if (SP_adj[j*ns+i]>0.5) {
        // get boundary pixel indices for superpixel j
        vector<int> bp_j = boundaryPixels(SP_bnd,j);
        
        // compute indices of intersecting boundary pixels
        // important note: this assumes that the inputs bp_i and bp_j are sorted!!
        vector<int> bp_int(bp_i.size()+bp_j.size());
        vector<int>::iterator it;
        it = set_intersection (bp_i.begin(),bp_i.end(),bp_j.begin(),bp_j.end(),bp_int.begin());
        bp_int.resize(it-bp_int.begin());
        int bp_num = bp_int.size();
       
        // create output
        int E_dims[2] = {nStates,nStates};
        mxArray* E = mxCreateNumericArray(2,E_dims,mxDOUBLE_CLASS,mxREAL);
        double* E_vals = (double*)mxGetPr(E);

        // for all planes in first superpixel do
        for (int pi=0; pi<nStates; pi++) {
          
          // extract plane parameters for first superpixel
          int pi_ = i*nStates+pi;
          pi_ = 3 * pi_;
          double a1 = Planes[pi_];
          double b1 = Planes[pi_+1];
          double c1 = Planes[pi_+2];
          
          // for all planes in second superpixel do
          for (int pj=0; pj<nStates; pj++) {
          
            // extract plane parameters for second superpixel
            int pj_ = j*nStates+pj;
            pj_ = 3 * pj_; 
            double a2 = Planes[pj_];
            double b2 = Planes[pj_+1];
            double c2 = Planes[pj_+2];

            // compute accumulated truncated l1 disparity error
            double e = 0;
            
            // for all boundary pixels (pixels of intersection) do
            for (int k=0; k<bp_num; k++) {
              
              // extract u and v image coordinates
              // note: 1-based index is calculated as planes are represented
              //       using MATLAB 1-based indices
              double u = bp_int[k]/img_height+1;
              double v = bp_int[k]%img_height+1;
              
              // compute disparity values given both plane hypotheses
              double d1 = disparity(u,v,a1,b1,c1);
              double d2 = disparity(u,v,a2,b2,c2);
              
              // accumulate truncated l1 disparity error
              //e += min(abs(d1-d2),clip_val);
              double d = abs(d1-d2);
              e += d < clip_val ? d : (d-clip_val)*0.2 + clip_val;
            }

            E_vals[pj*nStates+pi] = e;
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
