#ifndef GC_COST_CALCULATOR_H
#define GC_COST_CALCULATOR_H

#include "Image.h"

// Gradient-Census cost calculator
class GCCostCalculator {
public:
    GCCostCalculator();
    GCCostCalculator(const int disparityTotal,
                     const int sobelCapValue,
                     const int censusWindowRadius,
                     const double censusWeightFactor,
                     const int aggregationWindowRadius);
    
    void setParameter(const int disparityTotal,
                      const int sobelCapValue,
                      const int censusWindowRadius,
                      const double censusWeightFactor,
                      const int aggregationWindowRadius);
    
    void computeCostImage(const rev::Image<unsigned char>& leftImage,
                          const rev::Image<unsigned char>& rightImage,
                          rev::Image<unsigned short>& leftCostImage,
                          rev::Image<unsigned short>& rightCostImage);
    
    
private:
    void checkInputImages(const rev::Image<unsigned char>& leftImage,
                          const rev::Image<unsigned char>& rightImage) const;
    void computeLeftCostImage(const rev::Image<unsigned char>& leftImage,
                              const rev::Image<unsigned char>& rightImage,
                              rev::Image<unsigned short>& leftCostImage);
    void makeRightCostImage(const rev::Image<unsigned short>& leftCostImage,
                            rev::Image<unsigned short>& rightCostImage) const;
    
    void allocateDataBuffer();
    void calcTopRowCost(unsigned char*& leftSobelRow, unsigned char*& rightSobelRow, const int widthStepSobel,
                        int*& leftCensusRow, int*& rightCensusRow, const int widthStepCensus,
                        unsigned short* costImageRow);
    void calcRowCosts(unsigned char*& leftSobelRow, unsigned char*& rightSobelRow, const int widthStepSobel,
                      int*& leftCensusRow, int*& rightCensusRow, const int widthStepCensus,
                      unsigned short* costImageRow, const int widthStepCost);
    void calcPixelwiseSAD(const unsigned char* leftSobelRow, const unsigned char* rightSobelRow);
    void addPixelwiseHamming(const int* leftCensusRow, const int* rightCensusRow);
    void calcHalfPixelRight(const unsigned char* rightSobelRow);
    
    // Parameters
    int disparityTotal_;
    int sobelCapValue_;
    int censusWindowRadius_;
    double censusWeightFactor_;
    int aggregationWindowRadius_;
    
    // Image size
    int width_;
    int height_;
    // Data buffer
    rev::AlignedMemory<unsigned char> pixelwiseCostRow_;
    rev::AlignedMemory<unsigned short> rowAggregatedCost_;
    rev::AlignedMemory<unsigned char> halfPixelRightMin_;
    rev::AlignedMemory<unsigned char> halfPixelRightMax_;
};

#endif
