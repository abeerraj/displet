#ifndef CENSUS_TRANSFORM_H
#define CENSUS_TRANSFORM_H

#include "Image.h"

class CensusTransform {
public:
    CensusTransform();
    
    void setWindowRadius(const int windowRadius);
    
    void transform(const rev::Image<unsigned char>& grayscaleImage, rev::Image<int>& censusImage) const;

private:
    int windowRadius_;
};

#endif
