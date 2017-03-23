#ifndef CAPPED_SOBEL_FILTER_H
#define CAPPED_SOBEL_FILTER_H

#include "Image.h"

class CappedSobelFilter {
public:
    CappedSobelFilter();
    
    void setCapValue(const int capValue);
    
    void filter(const rev::Image<unsigned char>& grayscaleImage,
                rev::Image<unsigned char>& sobelImage,
                const bool horizontalFlip = false) const;

private:
    void filterSSE(const rev::Image<unsigned char>& sourceImage, rev::Image<short>& filteredImage) const;
    void convolveVerticalKernel(const unsigned char* imageData, const int width, const int height, short* output) const;
    void convolveHorizontalKernel(short* input, const int width, const int height, short* output) const;
    void capSobelImage(const rev::Image<short>& sobelImage,
                       const bool horizontalFlip,
                       rev::Image<unsigned char>& cappedSobelImage) const;
    
    // Parameter
    int capValue_;
};

#endif
