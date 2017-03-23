#ifndef SGM_STEREO_H
#define SGM_STEREO_H

#include "Image.h"

class SgmStereo {
public:
    SgmStereo();
    
    void setDisparityTotal(const int disparityTotal);
    void setOutputDisparityFactor(const double outputDisparityFactor);
    void setDataCostParameters(const int sobelCapValue,
                               const int censusWindowRadius,
                               const int censusWeightFactor,
                               const int aggregationWindowRadius);
    void setSmoothnessCostParameters(const int smoothnessPenaltySmall, const int smoothnessPenaltyLarge);
    void setSgmParameters(const bool pathEightDirections, const int consistencyThreshold);
    
    void computeLeftRight(const rev::Image<unsigned char>& leftImage,
                          const rev::Image<unsigned char>& rightImage,
                          rev::Image<unsigned char>& leftDisparityImage,
                          rev::Image<unsigned char>& rightDisparityImage);
    void computeLeftRight(const rev::Image<unsigned char>& leftImage,
                          const rev::Image<unsigned char>& rightImage,
                          rev::Image<unsigned short>& leftDisparityImage,
                          rev::Image<unsigned short>& rightDisparityImage);
    
private:
    void allocateBuffer(const int width, const int height);
    rev::Image<unsigned short> performSGM(const rev::Image<unsigned short>& costImage);
    void enforceLeftRightConsistency(rev::Image<unsigned short>& leftDisparityImage,
                                     rev::Image<unsigned short>& rightDisparityImage) const;
    
    void setBufferSize(const int width, const int height);
    void speckleFilter(const int maxSpeckleSize, const int maxDifference,
                       rev::Image<unsigned short>& image) const;
    
    // Parameter
    int disparityTotal_;
    double outputDisparityFactor_;
    int sobelCapValue_;
    int censusWindowRadius_;
    double censusWeightFactor_;
    int aggregationWindowRadius_;
    int smoothnessPenaltySmall_;
    int smoothnessPenaltyLarge_;
    bool pathEightDirections_;
    int consistencyThreshold_;
    
    // Data
    int pathRowBufferTotal_;
    int disparitySize_;
    int pathTotal_;
    int pathDisparitySize_;
    int costSumBufferRowSize_;
    int costSumBufferSize_;
    int pathMinCostBufferSize_;
    int pathCostBufferSize_;
    int totalBufferSize_;
    rev::AlignedMemory<short> buffer_;
};

#endif
