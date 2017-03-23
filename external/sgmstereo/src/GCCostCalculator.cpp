#include "GCCostCalculator.h"
#include <emmintrin.h>
#include <nmmintrin.h>
#include "CappedSobelFilter.h"
#include "CensusTransform.h"

const int GCCOSTCALCULATOR_DEFAULT_DISPARITY_TOTAL = 256;
const int GCCOSTCALCULATOR_DEFAULT_SOBEL_CAP_VALUE = 15;
const int GCCOSTCALCULATOR_DEFAULT_CENSUS_WINDOW_RADIUS = 2;
const double GCCOSTCALCULATOR_DEFAULT_CENSUS_WEIGHT_FACTOR = 1.0/6.0;
const int GCCOSTCALCULATOR_DEFAULT_AGGREGATION_WINDOW_RADIUS = 2;

GCCostCalculator::GCCostCalculator() : disparityTotal_(GCCOSTCALCULATOR_DEFAULT_DISPARITY_TOTAL),
                                       sobelCapValue_(GCCOSTCALCULATOR_DEFAULT_SOBEL_CAP_VALUE),
                                       censusWindowRadius_(GCCOSTCALCULATOR_DEFAULT_CENSUS_WINDOW_RADIUS),
                                       censusWeightFactor_(GCCOSTCALCULATOR_DEFAULT_CENSUS_WEIGHT_FACTOR),
                                       aggregationWindowRadius_(GCCOSTCALCULATOR_DEFAULT_AGGREGATION_WINDOW_RADIUS),
                                       width_(0), height_(0) {}

GCCostCalculator::GCCostCalculator(const int disparityTotal,
                                   const int sobelCapValue,
                                   const int censusWindowRadius,
                                   const double censusWeightFactor,
                                   const int aggregationWindowRadius)
{
    setParameter(disparityTotal, sobelCapValue, censusWindowRadius, censusWeightFactor, aggregationWindowRadius);
}

void GCCostCalculator::setParameter(const int disparityTotal,
                                    const int sobelCapValue,
                                    const int censusWindowRadius,
                                    const double censusWeightFactor,
                                    const int aggregationWindowRadius)
{
    // The number of disparities
    if (disparityTotal <= 0 || disparityTotal%16 != 0) {
        throw rev::Exception("GCCostCalculator::setParameters", "the number of disparities must be a multiple of 16");
    }
    disparityTotal_ = disparityTotal;
    
    // Cap value of sobel filter
    sobelCapValue_ = std::max(sobelCapValue, 15);
    sobelCapValue_ = std::min(sobelCapValue_, 127) | 1;
    
    // Window size of Census transform
    censusWindowRadius_ = censusWindowRadius;
    // Division factor of hamming distance between Census codes
    if (censusWeightFactor < 0) {
        throw rev::Exception("GCCostCalculator::setParameters", "weight of census transform must be positive");
    }
    censusWeightFactor_ = censusWeightFactor;
    
    // Window radius for aggregating pixelwise costs
    if (aggregationWindowRadius < 0) {
        throw rev::Exception("GCCostCalculator::setParameters", "window size of cost aggregation is less than zero");
    }
    aggregationWindowRadius_ = aggregationWindowRadius;
}

void GCCostCalculator::computeCostImage(const rev::Image<unsigned char>& leftImage,
                                        const rev::Image<unsigned char>& rightImage,
                                        rev::Image<unsigned short>& leftCostImage,
                                        rev::Image<unsigned short>& rightCostImage)
{
    checkInputImages(leftImage, rightImage);
    
    computeLeftCostImage(leftImage, rightImage, leftCostImage);
    makeRightCostImage(leftCostImage, rightCostImage);
}


void GCCostCalculator::checkInputImages(const rev::Image<unsigned char>& leftImage,
                                        const rev::Image<unsigned char>& rightImage) const
{
    if (leftImage.width() != rightImage.width() || leftImage.height() != rightImage.height()) {
        throw rev::Exception("GCCostCalculator::checkInputImages", "sizes of input stereo images are not the same");
    }
}

void GCCostCalculator::computeLeftCostImage(const rev::Image<unsigned char>& leftImage,
                                            const rev::Image<unsigned char>& rightImage,
                                            rev::Image<unsigned short>& leftCostImage)
{
    // Image size
    width_ = leftImage.width();
    height_ = leftImage.height();
    // Convert to grayscale
    rev::Image<unsigned char> leftGrayscaleImage = rev::convertToGrayscale(leftImage);
    rev::Image<unsigned char> rightGrayscaleImage = rev::convertToGrayscale(rightImage);
    
    // Sobel filter
    CappedSobelFilter sobel;
    sobel.setCapValue(sobelCapValue_);
    rev::Image<unsigned char> leftSobelImage;
    sobel.filter(leftGrayscaleImage, leftSobelImage);
    rev::Image<unsigned char> flippedRightSobelImage;
    sobel.filter(rightGrayscaleImage, flippedRightSobelImage, true);
    
    // Census transform
    CensusTransform census;
    census.setWindowRadius(censusWindowRadius_);
    rev::Image<int> leftCensusImage;
    rev::Image<int> rightCensusImage;
    census.transform(leftGrayscaleImage, leftCensusImage);
    census.transform(rightGrayscaleImage, rightCensusImage);
    
    // Compute cost image
    allocateDataBuffer();
    leftCostImage.resize(width_, height_, disparityTotal_);
    leftCostImage.setTo(0);
    
    unsigned char* leftSobelRow = leftSobelImage.dataPointer();
    unsigned char* rightSobelRow = flippedRightSobelImage.dataPointer();
    int widthStepSobel = leftSobelImage.widthStep();
    int* leftCensusRow = leftCensusImage.dataPointer();
    int* rightCensusRow = rightCensusImage.dataPointer();
    int widthStepCensus = leftCensusImage.widthStep();
    unsigned short* costImageRow = leftCostImage.dataPointer();
    int widthStepCost = leftCostImage.widthStep();
    
    calcTopRowCost(leftSobelRow, rightSobelRow, widthStepSobel,
                   leftCensusRow, rightCensusRow, widthStepCensus,
                   costImageRow);
    costImageRow += widthStepCost;
    calcRowCosts(leftSobelRow, rightSobelRow, widthStepSobel,
                 leftCensusRow, rightCensusRow, widthStepCensus,
                 costImageRow, widthStepCost);
}

void GCCostCalculator::makeRightCostImage(const rev::Image<unsigned short>& leftCostImage,
                                          rev::Image<unsigned short>& rightCostImage) const
{
    rightCostImage.resize(width_, height_, disparityTotal_);
    
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            int maxDisparityIndex = std::min(disparityTotal_, width_ - x);
            for (int d = 0; d < maxDisparityIndex; ++d) {
                rightCostImage(x, y, d) = leftCostImage(x + d, y, d);
            }
            unsigned short lastValue = rightCostImage(x, y, maxDisparityIndex - 1);
            for (int d = maxDisparityIndex; d < disparityTotal_; ++d) {
                rightCostImage(x, y, d) = lastValue;
            }
        }
    }
}


void GCCostCalculator::allocateDataBuffer() {
    int pixelwiseCostRowBufferSize = width_*disparityTotal_;
    int rowAggregatedCostBufferSize = width_*disparityTotal_*(aggregationWindowRadius_*2 + 2);
    int halfPixelRightBufferSize = width_;
    
    pixelwiseCostRow_.resize(pixelwiseCostRowBufferSize);
    rowAggregatedCost_.resize(rowAggregatedCostBufferSize);
    halfPixelRightMin_.resize(halfPixelRightBufferSize);
    halfPixelRightMax_.resize(halfPixelRightBufferSize);
}


void GCCostCalculator::calcTopRowCost(unsigned char*& leftSobelRow, unsigned char*& rightSobelRow, const int widthStepSobel,
                                      int*& leftCensusRow, int*& rightCensusRow, const int widthStepCensus,
                                      unsigned short* costImageRow)
{
    for (int rowIndex = 0; rowIndex <= aggregationWindowRadius_; ++rowIndex) {
        int rowAggregatedCostIndex = std::min(rowIndex, height_ - 1)%(aggregationWindowRadius_*2 + 2);
        unsigned short* rowAggregatedCostCurrent = rowAggregatedCost_.pointer() + rowAggregatedCostIndex*width_*disparityTotal_;
        
        calcPixelwiseSAD(leftSobelRow, rightSobelRow);
        addPixelwiseHamming(leftCensusRow, rightCensusRow);

        memset(rowAggregatedCostCurrent, 0, disparityTotal_*sizeof(unsigned short));
        // x = 0
        for (int x = 0; x <= aggregationWindowRadius_; ++x) {
            int scale = x == 0 ? aggregationWindowRadius_ + 1 : 1;
            for (int d = 0; d < disparityTotal_; ++d) {
                rowAggregatedCostCurrent[d] += static_cast<unsigned short>(pixelwiseCostRow_[disparityTotal_*x + d]*scale);
            }
        }
        // x = 1...width-1
        for (int x = 1; x < width_; ++x) {
            const unsigned char* addPixelwiseCost = pixelwiseCostRow_.pointer()
            + std::min((x + aggregationWindowRadius_)*disparityTotal_, (width_ - 1)*disparityTotal_);
            const unsigned char* subPixelwiseCost = pixelwiseCostRow_.pointer()
            + std::max((x - aggregationWindowRadius_ - 1)*disparityTotal_, 0);
            
            for (int d = 0; d < disparityTotal_; ++d) {
                rowAggregatedCostCurrent[disparityTotal_*x + d]
                = static_cast<unsigned short>(rowAggregatedCostCurrent[disparityTotal_*(x - 1) + d]
                                              + addPixelwiseCost[d] - subPixelwiseCost[d]);
            }
        }
        
        // Add to cost
        int scale = rowIndex == 0 ? aggregationWindowRadius_ + 1 : 1;
        for (int i = 0; i < width_*disparityTotal_; ++i) {
            costImageRow[i] += rowAggregatedCostCurrent[i]*scale;
        }
        
        leftSobelRow += widthStepSobel;
        rightSobelRow += widthStepSobel;
        leftCensusRow += widthStepCensus;
        rightCensusRow += widthStepCensus;
    }
}

void GCCostCalculator::calcRowCosts(unsigned char*& leftSobelRow, unsigned char*& rightSobelRow, const int widthStepSobel,
                                    int*& leftCensusRow, int*& rightCensusRow, const int widthStepCensus,
                                    unsigned short* costImageRow, const int widthStepCost)
{
    const __m128i registerZero = _mm_setzero_si128();
    
    for (int y = 1; y < height_; ++y) {
        int addRowIndex = y + aggregationWindowRadius_;
        int addRowAggregatedCostIndex = std::min(addRowIndex, height_ - 1)%(aggregationWindowRadius_*2 + 2);
        unsigned short* addRowAggregatedCost = rowAggregatedCost_.pointer() + width_*disparityTotal_*addRowAggregatedCostIndex;
        
        if (addRowIndex < height_) {
            calcPixelwiseSAD(leftSobelRow, rightSobelRow);
            addPixelwiseHamming(leftCensusRow, rightCensusRow);
            
            memset(addRowAggregatedCost, 0, disparityTotal_*sizeof(unsigned short));
            // x = 0
            for (int x = 0; x <= aggregationWindowRadius_; ++x) {
                int scale = x == 0 ? aggregationWindowRadius_ + 1 : 1;
                for (int d = 0; d < disparityTotal_; ++d) {
                    addRowAggregatedCost[d] += static_cast<unsigned short>(pixelwiseCostRow_[disparityTotal_*x + d]*scale);
                }
            }
            // x = 1...width-1
            int subRowAggregatedCostIndex = std::max(y - aggregationWindowRadius_ - 1, 0)%(aggregationWindowRadius_*2 + 2);
            const unsigned short* subRowAggregatedCost = rowAggregatedCost_.pointer() + width_*disparityTotal_*subRowAggregatedCostIndex;
            const unsigned short* previousCostRow = costImageRow - widthStepCost;
            for (int x = 1; x < width_; ++x) {
                const unsigned char* addPixelwiseCost = pixelwiseCostRow_.pointer()
                + std::min((x + aggregationWindowRadius_)*disparityTotal_, (width_ - 1)*disparityTotal_);
                const unsigned char* subPixelwiseCost = pixelwiseCostRow_.pointer()
                + std::max((x - aggregationWindowRadius_ - 1)*disparityTotal_, 0);
                
                for (int d = 0; d < disparityTotal_; d += 16) {
                    __m128i registerAddPixelwiseLow = _mm_load_si128(reinterpret_cast<const __m128i*>(addPixelwiseCost + d));
                    __m128i registerAddPixelwiseHigh = _mm_unpackhi_epi8(registerAddPixelwiseLow, registerZero);
                    registerAddPixelwiseLow = _mm_unpacklo_epi8(registerAddPixelwiseLow, registerZero);
                    __m128i registerSubPixelwiseLow = _mm_load_si128(reinterpret_cast<const __m128i*>(subPixelwiseCost + d));
                    __m128i registerSubPixelwiseHigh = _mm_unpackhi_epi8(registerSubPixelwiseLow, registerZero);
                    registerSubPixelwiseLow = _mm_unpacklo_epi8(registerSubPixelwiseLow, registerZero);
                    
                    // Low
                    __m128i registerAddAggregated = _mm_load_si128(reinterpret_cast<const __m128i*>(addRowAggregatedCost
                                                                                                    + disparityTotal_*(x - 1) + d));
                    registerAddAggregated = _mm_adds_epi16(_mm_subs_epi16(registerAddAggregated, registerSubPixelwiseLow),
                                                           registerAddPixelwiseLow);
                    __m128i registerCost = _mm_load_si128(reinterpret_cast<const __m128i*>(previousCostRow + disparityTotal_*x + d));
                    registerCost = _mm_adds_epi16(_mm_subs_epi16(registerCost,
                                                                 _mm_load_si128(reinterpret_cast<const __m128i*>(subRowAggregatedCost + disparityTotal_*x + d))),
                                                  registerAddAggregated);
                    _mm_store_si128(reinterpret_cast<__m128i*>(addRowAggregatedCost + disparityTotal_*x + d), registerAddAggregated);
                    _mm_store_si128(reinterpret_cast<__m128i*>(costImageRow + disparityTotal_*x + d), registerCost);
                    
                    // High
                    registerAddAggregated = _mm_load_si128(reinterpret_cast<const __m128i*>(addRowAggregatedCost + disparityTotal_*(x-1) + d + 8));
                    registerAddAggregated = _mm_adds_epi16(_mm_subs_epi16(registerAddAggregated, registerSubPixelwiseHigh),
                                                           registerAddPixelwiseHigh);
                    registerCost = _mm_load_si128(reinterpret_cast<const __m128i*>(previousCostRow + disparityTotal_*x + d + 8));
                    registerCost = _mm_adds_epi16(_mm_subs_epi16(registerCost,
                                                                 _mm_load_si128(reinterpret_cast<const __m128i*>(subRowAggregatedCost + disparityTotal_*x + d + 8))),
                                                  registerAddAggregated);
                    _mm_store_si128(reinterpret_cast<__m128i*>(addRowAggregatedCost + disparityTotal_*x + d + 8), registerAddAggregated);
                    _mm_store_si128(reinterpret_cast<__m128i*>(costImageRow + disparityTotal_*x + d + 8), registerCost);
                }
            }
        }
        
        leftSobelRow += widthStepSobel;
        rightSobelRow += widthStepSobel;
        leftCensusRow += widthStepCensus;
        rightCensusRow += widthStepCensus;
        costImageRow += widthStepCost;
    }
}

void GCCostCalculator::calcPixelwiseSAD(const unsigned char* leftSobelRow, const unsigned char* rightSobelRow) {
    calcHalfPixelRight(rightSobelRow);
    
    for (int x = 0; x < 16; ++x) {
        int leftCenterValue = leftSobelRow[x];
        int leftHalfLeftValue = x > 0 ? (leftCenterValue + leftSobelRow[x - 1])/2 : leftCenterValue;
        int leftHalfRightValue = x < width_ - 1 ? (leftCenterValue + leftSobelRow[x + 1])/2 : leftCenterValue;
        int leftMinValue = std::min(leftHalfLeftValue, leftHalfRightValue);
        leftMinValue = std::min(leftMinValue, leftCenterValue);
        int leftMaxValue = std::max(leftHalfLeftValue, leftHalfRightValue);
        leftMaxValue = std::max(leftMaxValue, leftCenterValue);
        
        for (int d = 0; d <= x; ++d) {
            int rightCenterValue = rightSobelRow[width_ - 1 - x + d];
            int rightMinValue = halfPixelRightMin_[width_ - 1 - x + d];
            int rightMaxValue = halfPixelRightMax_[width_ - 1 - x + d];
            
            int costLtoR = std::max(0, leftCenterValue - rightMaxValue);
            costLtoR = std::max(costLtoR, rightMinValue - leftCenterValue);
            int costRtoL = std::max(0, rightCenterValue - leftMaxValue);
            costRtoL = std::max(costRtoL, leftMinValue - rightCenterValue);
            int costValue = std::min(costLtoR, costRtoL);
            
            pixelwiseCostRow_[disparityTotal_*x + d] = costValue;
        }
        for (int d = x + 1; d < disparityTotal_; ++d) {
            pixelwiseCostRow_[disparityTotal_*x + d] = pixelwiseCostRow_[disparityTotal_*x + d - 1];
        }
    }
    for (int x = 16; x < disparityTotal_; ++x) {
        int leftCenterValue = leftSobelRow[x];
        int leftHalfLeftValue = x > 0 ? (leftCenterValue + leftSobelRow[x - 1])/2 : leftCenterValue;
        int leftHalfRightValue = x < width_ - 1 ? (leftCenterValue + leftSobelRow[x + 1])/2 : leftCenterValue;
        int leftMinValue = std::min(leftHalfLeftValue, leftHalfRightValue);
        leftMinValue = std::min(leftMinValue, leftCenterValue);
        int leftMaxValue = std::max(leftHalfLeftValue, leftHalfRightValue);
        leftMaxValue = std::max(leftMaxValue, leftCenterValue);
        
        __m128i registerLeftCenterValue = _mm_set1_epi8(static_cast<char>(leftCenterValue));
        __m128i registerLeftMinValue =_mm_set1_epi8(static_cast<char>(leftMinValue));
        __m128i registerLeftMaxValue =_mm_set1_epi8(static_cast<char>(leftMaxValue));
        
        for (int d = 0; d < x/16; d += 16) {
            __m128i registerRightCenterValue = _mm_loadu_si128(reinterpret_cast<const __m128i*>(rightSobelRow + width_ - 1 - x + d));
            __m128i registerRightMinValue = _mm_loadu_si128(reinterpret_cast<const __m128i*>(halfPixelRightMin_.pointer() + width_ - 1 - x + d));
            __m128i registerRightMaxValue = _mm_loadu_si128(reinterpret_cast<const __m128i*>(halfPixelRightMax_.pointer() + width_ - 1 - x + d));
            
            __m128i registerCostLtoR = _mm_max_epu8(_mm_subs_epu8(registerLeftCenterValue, registerRightMaxValue),
                                                    _mm_subs_epu8(registerRightMinValue, registerLeftCenterValue));
            __m128i registerCostRtoL = _mm_max_epu8(_mm_subs_epu8(registerRightCenterValue, registerLeftMaxValue),
                                                    _mm_subs_epu8(registerLeftMinValue, registerRightCenterValue));
            __m128i registerCost = _mm_min_epu8(registerCostLtoR, registerCostRtoL);
            
            _mm_store_si128(reinterpret_cast<__m128i*>(pixelwiseCostRow_.pointer() + disparityTotal_*x + d), registerCost);
        }
        for (int d = x/16; d <= x; ++d) {
            int rightCenterValue = rightSobelRow[width_ - 1 - x + d];
            int rightMinValue = halfPixelRightMin_[width_ - 1 - x + d];
            int rightMaxValue = halfPixelRightMax_[width_ - 1 - x + d];
            
            int costLtoR = std::max(0, leftCenterValue - rightMaxValue);
            costLtoR = std::max(costLtoR, rightMinValue - leftCenterValue);
            int costRtoL = std::max(0, rightCenterValue - leftMaxValue);
            costRtoL = std::max(costRtoL, leftMinValue - rightCenterValue);
            int costValue = std::min(costLtoR, costRtoL);
            
            pixelwiseCostRow_[disparityTotal_*x + d] = costValue;
        }
        for (int d = x + 1; d < disparityTotal_; ++d) {
            pixelwiseCostRow_[disparityTotal_*x + d] = pixelwiseCostRow_[disparityTotal_*x + d - 1];
        }
    }
    for (int x = disparityTotal_; x < width_; ++x) {
        int leftCenterValue = leftSobelRow[x];
        int leftHalfLeftValue = x > 0 ? (leftCenterValue + leftSobelRow[x - 1])/2 : leftCenterValue;
        int leftHalfRightValue = x < width_ - 1 ? (leftCenterValue + leftSobelRow[x + 1])/2 : leftCenterValue;
        int leftMinValue = std::min(leftHalfLeftValue, leftHalfRightValue);
        leftMinValue = std::min(leftMinValue, leftCenterValue);
        int leftMaxValue = std::max(leftHalfLeftValue, leftHalfRightValue);
        leftMaxValue = std::max(leftMaxValue, leftCenterValue);
        
        __m128i registerLeftCenterValue = _mm_set1_epi8(static_cast<char>(leftCenterValue));
        __m128i registerLeftMinValue =_mm_set1_epi8(static_cast<char>(leftMinValue));
        __m128i registerLeftMaxValue =_mm_set1_epi8(static_cast<char>(leftMaxValue));
        
        for (int d = 0; d < disparityTotal_; d += 16) {
            __m128i registerRightCenterValue = _mm_loadu_si128(reinterpret_cast<const __m128i*>(rightSobelRow + width_ - 1 - x + d));
            __m128i registerRightMinValue = _mm_loadu_si128(reinterpret_cast<const __m128i*>(halfPixelRightMin_.pointer() + width_ - 1 - x + d));
            __m128i registerRightMaxValue = _mm_loadu_si128(reinterpret_cast<const __m128i*>(halfPixelRightMax_.pointer() + width_ - 1 - x + d));
            
            __m128i registerCostLtoR = _mm_max_epu8(_mm_subs_epu8(registerLeftCenterValue, registerRightMaxValue),
                                                    _mm_subs_epu8(registerRightMinValue, registerLeftCenterValue));
            __m128i registerCostRtoL = _mm_max_epu8(_mm_subs_epu8(registerRightCenterValue, registerLeftMaxValue),
                                                    _mm_subs_epu8(registerLeftMinValue, registerRightCenterValue));
            __m128i registerCost = _mm_min_epu8(registerCostLtoR, registerCostRtoL);
            
            _mm_store_si128(reinterpret_cast<__m128i*>(pixelwiseCostRow_.pointer() + disparityTotal_*x + d), registerCost);
        }
    }
}

void GCCostCalculator::addPixelwiseHamming(const int* leftCensusRow, const int* rightCensusRow) {
    for (int x = 0; x < disparityTotal_; ++x) {
        int leftCencusCode = leftCensusRow[x];
        int hammingDistance = 0;
        for (int d = 0; d <= x; ++d) {
            int rightCensusCode = rightCensusRow[x - d];
            hammingDistance = static_cast<int>(_mm_popcnt_u32(static_cast<unsigned int>(leftCencusCode^rightCensusCode)));
            // hammingDistance = static_cast<int>(__builtin_popcount(static_cast<unsigned int>(leftCencusCode^rightCensusCode)));
            pixelwiseCostRow_[disparityTotal_*x + d] += static_cast<unsigned char>(hammingDistance*censusWeightFactor_);
        }
        hammingDistance = static_cast<unsigned char>(hammingDistance*censusWeightFactor_);
        for (int d = x + 1; d < disparityTotal_; ++d) {
            pixelwiseCostRow_[disparityTotal_*x + d] += hammingDistance;
        }
    }
    for (int x = disparityTotal_; x < width_; ++x) {
        int leftCencusCode = leftCensusRow[x];
        for (int d = 0; d < disparityTotal_; ++d) {
            int rightCensusCode = rightCensusRow[x - d];
            // int hammingDistance = static_cast<int>(__builtin_popcount(static_cast<unsigned int>(leftCencusCode^rightCensusCode)));
            int hammingDistance = static_cast<int>(_mm_popcnt_u32(static_cast<unsigned int>(leftCencusCode^rightCensusCode)));
            pixelwiseCostRow_[disparityTotal_*x + d] += static_cast<unsigned char>(hammingDistance*censusWeightFactor_);
        }
    }
}

void GCCostCalculator::calcHalfPixelRight(const unsigned char* rightSobelRow) {
    for (int x = 0; x < width_; ++x) {
        int centerValue = rightSobelRow[x];
        int leftHalfValue = x > 0 ? (centerValue + rightSobelRow[x - 1])/2 : centerValue;
        int rightHalfValue = x < width_ - 1 ? (centerValue + rightSobelRow[x + 1])/2 : centerValue;
        int minValue = std::min(leftHalfValue, rightHalfValue);
        minValue = std::min(minValue, centerValue);
        int maxValue = std::max(leftHalfValue, rightHalfValue);
        maxValue = std::max(maxValue, centerValue);
        
        halfPixelRightMin_[x] = minValue;
        halfPixelRightMax_[x] = maxValue;
    }
}
