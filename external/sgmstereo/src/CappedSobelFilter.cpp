#include "CappedSobelFilter.h"
#include <vector>
#include <emmintrin.h>

const int CAPPEDSOBELFILTER_DEFAULT_CAP_VALUE = 63;

CappedSobelFilter::CappedSobelFilter() : capValue_(CAPPEDSOBELFILTER_DEFAULT_CAP_VALUE) {}

void CappedSobelFilter::setCapValue(const int capValue) {
    capValue_ = std::max(capValue, 15);
    capValue_ = std::min(capValue_, 127) | 1;
}

void CappedSobelFilter::filter(const rev::Image<unsigned char>& grayscaleImage,
                               rev::Image<unsigned char>& cappedSobelImage,
                               const bool horizontalFlip) const
{
    if (grayscaleImage.channelTotal() != 1) {
        throw rev::Exception("CappedSobelFilter::filter", "input image must be grayscale");
    }
    
    rev::Image<short> sobelImage;
    filterSSE(grayscaleImage, sobelImage);
    capSobelImage(sobelImage, horizontalFlip, cappedSobelImage);
}


void CappedSobelFilter::filterSSE(const rev::Image<unsigned char>& sourceImage,
                                  rev::Image<short>& sobelImage) const
{
    int width = sourceImage.width();
    int height = sourceImage.height();
    int widthStep = sourceImage.widthStep();
    sobelImage.resize(width, height, 1);
    
    // Sobel filter
    rev::AlignedMemory<short> tempBuffer(widthStep*height);
    convolveVerticalKernel(sourceImage.dataPointer(), widthStep, height, tempBuffer.pointer());
    convolveHorizontalKernel(tempBuffer.pointer(), widthStep, height, sobelImage.dataPointer());
}

void CappedSobelFilter::convolveVerticalKernel(const unsigned char* imageData,
                                               const int width, const int height,
                                               short* output) const
{
    unsigned char* input = const_cast<unsigned char*>(imageData);
    __m128i zeroRegister = _mm_setzero_si128();
    for (int i = 0; i < width*(height - 2); i += 16) {
        __m128i topRow = _mm_load_si128(reinterpret_cast<__m128i*>(input + i));
        __m128i centerRow = _mm_load_si128(reinterpret_cast<__m128i*>(input + width + i));
        __m128i bottomRow = _mm_load_si128(reinterpret_cast<__m128i*>(input + width*2 + i));
        
        __m128i resultLowBit = _mm_setzero_si128();
        __m128i resultHighBit = _mm_setzero_si128();
        
        // Top row
        __m128i lowBitRegister = _mm_unpacklo_epi8(topRow, zeroRegister);
        __m128i highBitRegister = _mm_unpackhi_epi8(topRow, zeroRegister);
        resultLowBit = _mm_add_epi16(resultLowBit, lowBitRegister);
        resultHighBit = _mm_add_epi16(resultHighBit, highBitRegister);
        // Center row*2
        lowBitRegister = _mm_unpacklo_epi8(centerRow, zeroRegister);
        highBitRegister = _mm_unpackhi_epi8(centerRow, zeroRegister);
        resultLowBit = _mm_add_epi16(resultLowBit, lowBitRegister);
        resultLowBit = _mm_add_epi16(resultLowBit, lowBitRegister);
        resultHighBit = _mm_add_epi16(resultHighBit, highBitRegister);
        resultHighBit = _mm_add_epi16(resultHighBit, highBitRegister);
        // Bottom row
        lowBitRegister = _mm_unpacklo_epi8(bottomRow, zeroRegister);
        highBitRegister = _mm_unpackhi_epi8(bottomRow, zeroRegister);
        resultLowBit = _mm_add_epi16(resultLowBit, lowBitRegister);
        resultHighBit = _mm_add_epi16(resultHighBit, highBitRegister);
        
        _mm_store_si128(reinterpret_cast<__m128i*>(output + width + i), resultLowBit);
        _mm_store_si128(reinterpret_cast<__m128i*>(output + width + i + 8), resultHighBit);
    }
}

void CappedSobelFilter::convolveHorizontalKernel(short* input, const int width, const int height, short* output) const {
    for (int i = 0; i < width*height - 2; i += 8) {
        __m128i leftColumn = _mm_load_si128(reinterpret_cast<__m128i*>(input + i));
        __m128i rightColumn = _mm_loadu_si128(reinterpret_cast<__m128i*>(input + i + 2));
        __m128i resultRegister = _mm_sub_epi16(rightColumn, leftColumn);
        
        _mm_storeu_si128(reinterpret_cast<__m128i*>(output + i + 1), resultRegister);
    }
}

void CappedSobelFilter::capSobelImage(const rev::Image<short>& sobelImage,
                                      const bool horizontalFlip,
                                      rev::Image<unsigned char>& cappedSobelImage) const
{
    int width = sobelImage.width();
    int height = sobelImage.height();
    
    // Look up table for converting sobel value
    const int tableOffset = 256*4;
    const int tableSize = tableOffset*2 + 256;
    std::vector<unsigned char> convertTable(tableSize);
    for (int i = 0; i < tableSize; ++i) {
        convertTable[i] = static_cast<unsigned char>(std::min(std::max(i - tableOffset, -capValue_), capValue_) + capValue_);
    }
    
    cappedSobelImage.resize(width, height, 1);
    cappedSobelImage.setTo(convertTable[tableOffset]);
    if (!horizontalFlip) {
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                cappedSobelImage(x, y) = convertTable[tableOffset + sobelImage(x, y)];
            }
        }
    } else {
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                cappedSobelImage(width - x - 1, y) = convertTable[tableOffset + sobelImage(x, y)];
            }
        }
    }
}
