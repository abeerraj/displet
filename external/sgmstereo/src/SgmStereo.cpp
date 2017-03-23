#include "SgmStereo.h"
#include <vector>
#include <stack>
#include <limits.h>
#include "GCCostCalculator.h"

// Default parameters
const int SGMSTEREO_DEFAULT_DISPARITY_TOTAL = 256;
const double SGMSTEREO_DEFAULT_OUTPUT_DISPARITY_FACTOR = 256;
const int SGMSTEREO_DEFAULT_SOBEL_CAP_VALUE = 15;
const int SGMSTEREO_DEFAULT_CENSUS_WINDOW_RADIUS = 2;
const double SGMSTEREO_DEFAULT_CENSUS_WEIGHT_FACTOR = 1.0/6.0;
const int SGMSTEREO_DEFAULT_AGGREGATION_WINDOW_RADIUS = 2;
const int SGMSTEREO_DEFAULT_SMOOTHNESS_PENALTY_SMALL = 4*(SGMSTEREO_DEFAULT_AGGREGATION_WINDOW_RADIUS*2 + 1)*(SGMSTEREO_DEFAULT_AGGREGATION_WINDOW_RADIUS*2 + 1);
const int SGMSTEREO_DEFAULT_SMOOTHNESS_PENALTY_LARGE = 64*(SGMSTEREO_DEFAULT_AGGREGATION_WINDOW_RADIUS*2 + 1)*(SGMSTEREO_DEFAULT_AGGREGATION_WINDOW_RADIUS*2 + 1);
const bool SGMSTEREO_DEFAULT_PATH_EIGHT_DIRECTIONS = false;
const int SGMSTEREO_DEFAULT_CONSISTENCY_THRESHOLD = 1;

SgmStereo::SgmStereo() : disparityTotal_(SGMSTEREO_DEFAULT_DISPARITY_TOTAL),
                         outputDisparityFactor_(SGMSTEREO_DEFAULT_OUTPUT_DISPARITY_FACTOR),
                         sobelCapValue_(SGMSTEREO_DEFAULT_SOBEL_CAP_VALUE),
                         censusWindowRadius_(SGMSTEREO_DEFAULT_CENSUS_WINDOW_RADIUS),
                         censusWeightFactor_(SGMSTEREO_DEFAULT_CENSUS_WEIGHT_FACTOR),
                         aggregationWindowRadius_(SGMSTEREO_DEFAULT_AGGREGATION_WINDOW_RADIUS),
                         smoothnessPenaltySmall_(SGMSTEREO_DEFAULT_SMOOTHNESS_PENALTY_SMALL),
                         smoothnessPenaltyLarge_(SGMSTEREO_DEFAULT_SMOOTHNESS_PENALTY_LARGE),
                         pathEightDirections_(SGMSTEREO_DEFAULT_PATH_EIGHT_DIRECTIONS),
                         consistencyThreshold_(SGMSTEREO_DEFAULT_CONSISTENCY_THRESHOLD) {}

void SgmStereo::setDisparityTotal(const int disparityTotal) {
    if (disparityTotal <= 0 || disparityTotal%16 != 0) {
        throw rev::Exception("SgmStereo::setDisparityTotal", "the number of disparities must be a multiple of 16");
    }
    
    disparityTotal_ = disparityTotal;
}

void SgmStereo::setOutputDisparityFactor(const double outputDisparityFactor) {
    if (outputDisparityFactor <= 0) {
        throw rev::Exception("SgmStereo::setOutputDisparityFactor", "disparity factor is less than zero");
    }
    
    outputDisparityFactor_ = outputDisparityFactor;
}

void SgmStereo::setDataCostParameters(const int sobelCapValue,
                                      const int censusWindowRadius,
                                      const int censusWeightFactor,
                                      const int aggregationWindowRadius)
{
    sobelCapValue_ = sobelCapValue;
    censusWindowRadius_ = censusWindowRadius;
    censusWeightFactor_ = censusWeightFactor;
    aggregationWindowRadius_ = aggregationWindowRadius;
}

void SgmStereo::setSmoothnessCostParameters(const int smoothnessPenaltySmall, const int smoothnessPenaltyLarge)
{
    if (smoothnessPenaltySmall < 0 || smoothnessPenaltyLarge < 0) {
        throw rev::Exception("SgmStereo::setSmoothnessCostParameters", "smoothness penalty value is less than zero");
    }
    if (smoothnessPenaltySmall >= smoothnessPenaltyLarge) {
        throw rev::Exception("SgmStereo::setSmoothnessCostParameters",
                             "small value of smoothness penalty must be smaller than large penalty value");
    }
    
    smoothnessPenaltySmall_ = smoothnessPenaltySmall;
    smoothnessPenaltyLarge_ = smoothnessPenaltyLarge;
}

void SgmStereo::setSgmParameters(const bool pathEightDirections, const int consistencyThreshold) {
    pathEightDirections_ = pathEightDirections;
    
    if (consistencyThreshold < 0) {
        throw rev::Exception("SgmStereo::setSgmParameters", "Threshold for LR consistency must be positive");
    }
    consistencyThreshold_ = consistencyThreshold;
}

void SgmStereo::computeLeftRight(const rev::Image<unsigned char>& leftImage,
                                 const rev::Image<unsigned char>& rightImage,
                                 rev::Image<unsigned char>& leftDisparityImage,
                                 rev::Image<unsigned char>& rightDisparityImage)
{
    rev::Image<unsigned short> leftDisparityShortImage;
    rev::Image<unsigned short> rightDisparityShortImage;
    computeLeftRight(leftImage, rightImage, leftDisparityShortImage, rightDisparityShortImage);
    
    int width = leftDisparityShortImage.width();
    int height = leftDisparityShortImage.height();
    leftDisparityImage.resize(width, height, 1);
    rightDisparityImage.resize(width, height, 1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            leftDisparityImage(x, y) = static_cast<unsigned char>(leftDisparityShortImage(x, y));
            rightDisparityImage(x, y) = static_cast<unsigned char>(rightDisparityShortImage(x, y));
        }
    }
}

void SgmStereo::computeLeftRight(const rev::Image<unsigned char>& leftImage,
                                 const rev::Image<unsigned char>& rightImage,
                                 rev::Image<unsigned short>& leftDisparityImage,
                                 rev::Image<unsigned short>& rightDisparityImage)
{
    // Compute cost images
    rev::Image<unsigned short> leftCostImage, rightCostImage;
    GCCostCalculator costCalculator(disparityTotal_,
                                    sobelCapValue_,
                                    censusWindowRadius_,
                                    censusWeightFactor_,
                                    aggregationWindowRadius_);
    costCalculator.computeCostImage(leftImage, rightImage, leftCostImage, rightCostImage);
    
    // Semi-global matching
    allocateBuffer(leftCostImage.width(), leftCostImage.height());
    leftDisparityImage = performSGM(leftCostImage);
    rightDisparityImage = performSGM(rightCostImage);
    
    // Consistency check
    enforceLeftRightConsistency(leftDisparityImage, rightDisparityImage);
}

void SgmStereo::allocateBuffer(const int width, const int height) {
    setBufferSize(width, height);
    
    buffer_.resize(totalBufferSize_);
}

rev::Image<unsigned short> SgmStereo::performSGM(const rev::Image<unsigned short>& costImage) {
    const short costMax = SHRT_MAX;
    
    int width = costImage.width();
    int height = costImage.height();
    
    unsigned short* costImageData = costImage.dataPointer();
    int widthStepCostImage = costImage.widthStep();
    
    rev::Image<unsigned short> disparityImage(width, height, 1);
    unsigned short* disparityImageData = disparityImage.dataPointer();
    int widthStepDisparityImage = disparityImage.widthStep();
    
    short* costSums = buffer_.pointer();
    memset(costSums, 0, costSumBufferSize_*sizeof(short));
    
    short** pathCosts = new short* [pathRowBufferTotal_];
    short** pathMinCosts = new short* [pathRowBufferTotal_];
    
    const int processPassTotal = 2;
    for (int processPassCount = 0; processPassCount < processPassTotal; ++processPassCount) {
        int startX, endX, stepX;
        int startY, endY, stepY;
        if (processPassCount == 0) {
            startX = 0; endX = width; stepX = 1;
            startY = 0; endY = height; stepY = 1;
        } else {
            startX = width - 1; endX = -1; stepX = -1;
            startY = height - 1; endY = -1; stepY = -1;
        }
        
        for (int i = 0; i < pathRowBufferTotal_; ++i) {
            pathCosts[i] = costSums + costSumBufferSize_ + pathCostBufferSize_*i + pathDisparitySize_ + 8;
            memset(pathCosts[i] - pathDisparitySize_ - 8, 0, pathCostBufferSize_*sizeof(short));
            pathMinCosts[i] = costSums + costSumBufferSize_ + pathCostBufferSize_*pathRowBufferTotal_
            + pathMinCostBufferSize_*i + pathTotal_*2;
            memset(pathMinCosts[i] - pathTotal_, 0, pathMinCostBufferSize_*sizeof(short));
        }
        
        for (int y = startY; y != endY; y += stepY) {
            unsigned short* disparityRow = disparityImageData + widthStepDisparityImage*y;
            unsigned short* pixelCostRow = costImageData + widthStepCostImage*y;
            short* costSumRow = costSums + costSumBufferRowSize_*y;
            
            memset(pathCosts[0] - pathDisparitySize_ - 8, 0, pathDisparitySize_*sizeof(short));
            memset(pathCosts[0] + width*pathDisparitySize_ - 8, 0, pathDisparitySize_*sizeof(short));
            memset(pathMinCosts[0] - pathTotal_, 0, pathTotal_*sizeof(short));
            memset(pathMinCosts[0] + width*pathTotal_, 0, pathTotal_*sizeof(short));
            
            for (int x = startX; x != endX; x += stepX) {
                int pathMinX = x*pathTotal_;
                int pathX = pathMinX*disparitySize_;
                
                int previousPathMin0, previousPathMin1, previousPathMin2, previousPathMin3;
                previousPathMin0 = pathMinCosts[0][pathMinX - stepX*pathTotal_] + smoothnessPenaltyLarge_;
                previousPathMin2 = pathMinCosts[1][pathMinX + 2] + smoothnessPenaltyLarge_;
                
                short *previousPathCosts0, *previousPathCosts1, *previousPathCosts2, *previousPathCosts3;
                previousPathCosts0 = pathCosts[0] + pathX - stepX*pathDisparitySize_;
                previousPathCosts2 = pathCosts[1] + pathX + disparitySize_*2;
                
                previousPathCosts0[-1] = previousPathCosts0[disparityTotal_] = costMax;
                previousPathCosts2[-1] = previousPathCosts2[disparityTotal_] = costMax;
                
                if (pathEightDirections_) {
                    previousPathMin1 = pathMinCosts[1][pathMinX - pathTotal_ + 1] + smoothnessPenaltyLarge_;
                    previousPathMin3 = pathMinCosts[1][pathMinX + pathTotal_ + 3] + smoothnessPenaltyLarge_;
                    
                    previousPathCosts1 = pathCosts[1] + pathX - pathDisparitySize_ + disparitySize_;
                    previousPathCosts3 = pathCosts[1] + pathX + pathDisparitySize_ + disparitySize_*3;
                    
                    previousPathCosts1[-1] = previousPathCosts1[disparityTotal_] = costMax;
                    previousPathCosts3[-1] = previousPathCosts3[disparityTotal_] = costMax;
                }
                
                short* pathCostCurrent = pathCosts[0] + pathX;
                const unsigned short* pixelCostCurrent = pixelCostRow + disparityTotal_*x;
                short* costSumCurrent = costSumRow + disparityTotal_*x;
                
                __m128i regPenaltySmall = _mm_set1_epi16(static_cast<short>(smoothnessPenaltySmall_));
                
                __m128i regPathMin0, regPathMin1, regPathMin2, regPathMin3;
                regPathMin0 = _mm_set1_epi16(static_cast<short>(previousPathMin0));
                regPathMin2 = _mm_set1_epi16(static_cast<short>(previousPathMin2));
                if (pathEightDirections_) {
                    regPathMin1 = _mm_set1_epi16(static_cast<short>(previousPathMin1));
                    regPathMin3 = _mm_set1_epi16(static_cast<short>(previousPathMin3));
                }
                __m128i regNewPathMin = _mm_set1_epi16(costMax);
                
                for (int d = 0; d < disparityTotal_; d += 8) {
                    __m128i regPixelCost = _mm_load_si128(reinterpret_cast<const __m128i*>(pixelCostCurrent + d));
                    
                    __m128i regPathCost0, regPathCost1, regPathCost2, regPathCost3;
                    regPathCost0 = _mm_load_si128(reinterpret_cast<const __m128i*>(previousPathCosts0 + d));
                    regPathCost2 = _mm_load_si128(reinterpret_cast<const __m128i*>(previousPathCosts2 + d));
                    
                    regPathCost0 = _mm_min_epi16(regPathCost0,
                                                 _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts0 + d - 1)),
                                                                regPenaltySmall));
                    regPathCost0 = _mm_min_epi16(regPathCost0,
                                                 _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts0 + d + 1)),
                                                                regPenaltySmall));
                    regPathCost2 = _mm_min_epi16(regPathCost2,
                                                 _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts2 + d - 1)),
                                                                regPenaltySmall));
                    regPathCost2 = _mm_min_epi16(regPathCost2,
                                                 _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts2 + d + 1)),
                                                                regPenaltySmall));
                    
                    regPathCost0 = _mm_min_epi16(regPathCost0, regPathMin0);
                    regPathCost0 = _mm_adds_epi16(_mm_subs_epi16(regPathCost0, regPathMin0), regPixelCost);
                    regPathCost2 = _mm_min_epi16(regPathCost2, regPathMin2);
                    regPathCost2 = _mm_adds_epi16(_mm_subs_epi16(regPathCost2, regPathMin2), regPixelCost);
                    
                    _mm_store_si128(reinterpret_cast<__m128i*>(pathCostCurrent + d), regPathCost0);
                    _mm_store_si128(reinterpret_cast<__m128i*>(pathCostCurrent + d + disparitySize_*2), regPathCost2);
                    
                    if (pathEightDirections_) {
                        regPathCost1 = _mm_load_si128(reinterpret_cast<const __m128i*>(previousPathCosts1 + d));
                        regPathCost3 = _mm_load_si128(reinterpret_cast<const __m128i*>(previousPathCosts3 + d));
                        
                        regPathCost1 = _mm_min_epi16(regPathCost1,
                                                     _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts1 + d - 1)),
                                                                    
                                                                    regPenaltySmall));
                        regPathCost1 = _mm_min_epi16(regPathCost1,
                                                     _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts1 + d + 1)),
                                                                    regPenaltySmall));
                        regPathCost3 = _mm_min_epi16(regPathCost3,
                                                     _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts3 + d - 1)),
                                                                    regPenaltySmall));
                        regPathCost3 = _mm_min_epi16(regPathCost3,
                                                     _mm_adds_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(previousPathCosts3 + d + 1)),
                                                                    regPenaltySmall));
                        
                        regPathCost1 = _mm_min_epi16(regPathCost1, regPathMin1);
                        regPathCost1 = _mm_adds_epi16(_mm_subs_epi16(regPathCost1, regPathMin1), regPixelCost);
                        regPathCost3 = _mm_min_epi16(regPathCost3, regPathMin3);
                        regPathCost3 = _mm_adds_epi16(_mm_subs_epi16(regPathCost3, regPathMin3), regPixelCost);
                        
                        _mm_store_si128(reinterpret_cast<__m128i*>(pathCostCurrent + d + disparitySize_), regPathCost1);
                        _mm_store_si128(reinterpret_cast<__m128i*>(pathCostCurrent + d + disparitySize_*3), regPathCost3);
                    }
                    
                    __m128i regMin02 = _mm_min_epi16(_mm_unpacklo_epi16(regPathCost0, regPathCost2),
                                                     _mm_unpackhi_epi16(regPathCost0, regPathCost2));
                    if (pathEightDirections_) {
                        __m128i regMin13 = _mm_min_epi16(_mm_unpacklo_epi16(regPathCost1, regPathCost3),
                                                         _mm_unpackhi_epi16(regPathCost1, regPathCost3));
                        regMin02 = _mm_min_epi16(_mm_unpacklo_epi16(regMin02, regMin13),
                                                 _mm_unpackhi_epi16(regMin02, regMin13));
                    } else {
                        regMin02 = _mm_min_epi16(_mm_unpacklo_epi16(regMin02, regMin02),
                                                 _mm_unpackhi_epi16(regMin02, regMin02));
                    }
                    regNewPathMin = _mm_min_epi16(regNewPathMin, regMin02);
                    
                    __m128i regCostSum = _mm_load_si128(reinterpret_cast<const __m128i*>(costSumCurrent + d));
                    
                    if (pathEightDirections_) {
                        regPathCost0 = _mm_adds_epi16(regPathCost0, regPathCost1);
                        regPathCost2 = _mm_adds_epi16(regPathCost2, regPathCost3);
                    }
                    regCostSum = _mm_adds_epi16(regCostSum, regPathCost0);
                    regCostSum = _mm_adds_epi16(regCostSum, regPathCost2);
                    
                    _mm_store_si128(reinterpret_cast<__m128i*>(costSumCurrent + d), regCostSum);
                }
                
                regNewPathMin = _mm_min_epi16(regNewPathMin, _mm_srli_si128(regNewPathMin, 8));
                _mm_storel_epi64(reinterpret_cast<__m128i*>(&pathMinCosts[0][pathMinX]), regNewPathMin);
            }
            
            if (processPassCount == processPassTotal - 1) {
                for (int x = 0; x < width; ++x) {
                    short* costSumCurrent = costSumRow + disparityTotal_*x;
                    int bestSumCost = costSumCurrent[0];
                    int bestDisparity = 0;
                    for (int d = 1; d < disparityTotal_; ++d) {
                        if (costSumCurrent[d] < bestSumCost) {
                            bestSumCost = costSumCurrent[d];
                            bestDisparity = d;
                        }
                    }
                    
                    if (bestDisparity > 0 && bestDisparity < disparityTotal_ - 1) {
                        int centerCostValue = costSumCurrent[bestDisparity];
                        int leftCostValue = costSumCurrent[bestDisparity - 1];
                        int rightCostValue = costSumCurrent[bestDisparity + 1];
                        if (rightCostValue < leftCostValue) {
                            bestDisparity = static_cast<int>(bestDisparity*outputDisparityFactor_
                                                             + static_cast<double>(rightCostValue - leftCostValue)/(centerCostValue - leftCostValue)/2.0*outputDisparityFactor_ + 0.5);
                        } else {
                            bestDisparity = static_cast<int>(bestDisparity*outputDisparityFactor_
                                                             + static_cast<double>(rightCostValue - leftCostValue)/(centerCostValue - rightCostValue)/2.0*outputDisparityFactor_ + 0.5);
                        }
                    } else {
                        bestDisparity *= outputDisparityFactor_;
                    }
                    
                    disparityRow[x] = static_cast<unsigned short>(bestDisparity);
                }
            }
            
            std::swap(pathCosts[0], pathCosts[1]);
            std::swap(pathMinCosts[0], pathMinCosts[1]);
        }
    }
    delete [] pathCosts;
    delete [] pathMinCosts;
    
    speckleFilter(100, 2*outputDisparityFactor_, disparityImage);
    
    return disparityImage;
}

void SgmStereo::enforceLeftRightConsistency(rev::Image<unsigned short>& leftDisparityImage,
                                            rev::Image<unsigned short>& rightDisparityImage) const
{
    int width = leftDisparityImage.width();
    int height = leftDisparityImage.height();
    
    // Check left disparity image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (leftDisparityImage(x, y) == 0) continue;
            
            int leftDisparityValue = static_cast<int>(static_cast<double>(leftDisparityImage(x, y))/outputDisparityFactor_ + 0.5);
            if (x - leftDisparityValue < 0) {
                leftDisparityImage(x, y) = 0;
                continue;
            }
            
            int rightDisparityValue = static_cast<int>(static_cast<double>(rightDisparityImage(x-leftDisparityValue, y))/outputDisparityFactor_ + 0.5);
            if (rightDisparityValue == 0 || abs(leftDisparityValue - rightDisparityValue) > consistencyThreshold_) {
                leftDisparityImage(x, y) = 0;
            }
        }
    }
    
    // Check right disparity image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (rightDisparityImage(x, y) == 0)  continue;
            
            int rightDisparityValue = static_cast<int>(static_cast<double>(rightDisparityImage(x, y))/outputDisparityFactor_ + 0.5);
            if (x + rightDisparityValue >= width) {
                rightDisparityImage(x, y) = 0;
                continue;
            }
            
            int leftDisparityValue = static_cast<int>(static_cast<double>(leftDisparityImage(x+rightDisparityValue, y))/outputDisparityFactor_ + 0.5);
            if (leftDisparityValue == 0 || abs(rightDisparityValue - leftDisparityValue) > consistencyThreshold_) {
                rightDisparityImage(x, y) = 0;
            }
        }
    }
}

void SgmStereo::setBufferSize(const int width, const int height) {
    pathRowBufferTotal_ = 2;
    disparitySize_ = disparityTotal_ + 16;
    pathTotal_ = 8;
    pathDisparitySize_ = pathTotal_*disparitySize_;
    
    costSumBufferRowSize_ = width*disparityTotal_;
    costSumBufferSize_ = costSumBufferRowSize_*height;
    pathMinCostBufferSize_ = (width + 2)*pathTotal_;
    pathCostBufferSize_ = pathMinCostBufferSize_*disparitySize_;
    totalBufferSize_ = (pathMinCostBufferSize_ + pathCostBufferSize_)*pathRowBufferTotal_ + costSumBufferSize_ + 16;
}

void SgmStereo::speckleFilter(const int maxSpeckleSize, const int maxDifference,
                              rev::Image<unsigned short>& image) const
{
    int width = image.width();
    int height = image.height();
    
    std::vector<int> labels(width*height, 0);
    std::vector<bool> regionTypes(1);
    regionTypes[0] = false;
    
    int currentLabelIndex = 0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixelIndex = width*y + x;
            if (image(x, y) != 0) {
                if (labels[pixelIndex] > 0) {
                    if (regionTypes[labels[pixelIndex]]) {
                        image(x, y) = 0;
                    }
                }else {
                    std::stack<int> wavefrontIndices;
                    wavefrontIndices.push(pixelIndex);
                    ++currentLabelIndex;
                    regionTypes.push_back(false);
                    int regionPixelTotal = 0;
                    labels[pixelIndex] = currentLabelIndex;
                    
                    while (!wavefrontIndices.empty()) {
                        int currentPixelIndex = wavefrontIndices.top();
                        wavefrontIndices.pop();
                        int currentX = currentPixelIndex%width;
                        int currentY = currentPixelIndex/width;
                        ++regionPixelTotal;
                        unsigned short pixelValue = image(currentX, currentY);
                        
                        if (currentX < width - 1 && labels[currentPixelIndex + 1] == 0
                            && image(currentX + 1, currentY) != 0
                            && abs(pixelValue - image(currentX + 1, currentY)) <= maxDifference)
                        {
                            labels[currentPixelIndex + 1] = currentLabelIndex;
                            wavefrontIndices.push(currentPixelIndex + 1);
                        }
                        
                        if (currentX > 0 && labels[currentPixelIndex - 1] == 0
                            && image(currentX - 1, currentY) != 0
                            && abs(pixelValue - image(currentX - 1, currentY)) <= maxDifference)
                        {
                            labels[currentPixelIndex - 1] = currentLabelIndex;
                            wavefrontIndices.push(currentPixelIndex - 1);
                        }
                        
                        if (currentY < height - 1 && labels[currentPixelIndex + width] == 0
                            && image(currentX, currentY + 1) != 0
                            && abs(pixelValue - image(currentX, currentY + 1)) <= maxDifference)
                        {
                            labels[currentPixelIndex + width] = currentLabelIndex;
                            wavefrontIndices.push(currentPixelIndex + width);
                        }
                        
                        if (currentY > 0 && labels[currentPixelIndex - width] == 0
                            && image(currentX, currentY - 1) != 0
                            && abs(pixelValue - image(currentX, currentY - 1)) <= maxDifference)
                        {
                            labels[currentPixelIndex - width] = currentLabelIndex;
                            wavefrontIndices.push(currentPixelIndex - width);
                        }
                    }
                    
                    if (regionPixelTotal <= maxSpeckleSize) {
                        regionTypes[currentLabelIndex] = true;
                        image(x, y) = 0;
                    }
                }
            }
        }
    }
}
