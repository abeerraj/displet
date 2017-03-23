#include "CensusTransform.h"

const int CENSUSTRANSFORM_DEFAUT_WINDOW_RADIUS = 2;

CensusTransform::CensusTransform() : windowRadius_(CENSUSTRANSFORM_DEFAUT_WINDOW_RADIUS) {}

void CensusTransform::setWindowRadius(const int windowRadius) {
    if (windowRadius < 1 || windowRadius > 2) {
        throw rev::Exception("CensusTransform::setWindowRadius", "window radius must be 1 or 2");
    }
    
    windowRadius_ = windowRadius;
}

void CensusTransform::transform(const rev::Image<unsigned char>& grayscaleImage,
                                rev::Image<int>& censusImage) const
{
    if (grayscaleImage.channelTotal() != 1) {
        throw rev::Exception("CensusTransform::transform", "input image must be a grayscale image");
    }
    
    int width = grayscaleImage.width();
    int height = grayscaleImage.height();
    censusImage.resize(width, height, 1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char centerValue = grayscaleImage(x, y);
            int censusCode = 0;
            for (int offsetY = -windowRadius_; offsetY <= windowRadius_; ++offsetY) {
                for (int offsetX = -windowRadius_; offsetX <= windowRadius_; ++offsetX) {
                    if (y + offsetY >= 0 && y + offsetY < height
                        && x + offsetX >= 0 && x + offsetX < width
                        && grayscaleImage(x + offsetX, y + offsetY) >= centerValue) censusCode += 1;
                    censusCode = censusCode << 1;
                }
            }
            censusImage(x, y) = censusCode;
        }
    }
}
