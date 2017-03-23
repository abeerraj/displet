#ifndef REVLIB_IMAGE_H
#define REVLIB_IMAGE_H

#include <string>
#include <string.h>
#include <emmintrin.h>
#include "AlignedMemory.h"
#include "Exception.h"

namespace rev {
    
template <typename T> class Color {
public:
    Color() {}
    Color(const T firstChannelValue, const T secondChannelValue, const T thirdChannelValue) {
        channelValues_[0] = firstChannelValue;
        channelValues_[1] = secondChannelValue;
        channelValues_[2] = thirdChannelValue;
    }
    Color(const Color<T>& sourceColor) {
        channelValues_[0] = sourceColor.channelValues_[0];
        channelValues_[1] = sourceColor.channelValues_[1];
        channelValues_[2] = sourceColor.channelValues_[2];
    }
        
    void set(const T firstChannelValue, const T secondChannelValue, const T thirdChannelValue) {
        channelValues_[0] = firstChannelValue;
        channelValues_[1] = secondChannelValue;
        channelValues_[2] = thirdChannelValue;
    }
    Color<T>& operator=(const Color<T>& sourceColor) {
        if (&sourceColor != this) {
            channelValues_[0] = sourceColor.channelValues_[0];
            channelValues_[1] = sourceColor.channelValues_[1];
            channelValues_[2] = sourceColor.channelValues_[2];
        }
        return *this;
    }
        
    bool operator==(const Color<T>& comparisonColor) const {
        if (comparisonColor.channelValues_[0] == channelValues_[0]
            && comparisonColor.channelValues_[1] == channelValues_[1]
            && comparisonColor.channelValues_[2] == channelValues_[2]) return true;
        else return false;
    }
        
    T& operator[](const int channelIndex) {
#ifdef DEBUG
        if (channelIndex < 0 || channelIndex > 2) {
            throw Exception("rev::Color<T>::operator[]", "channel index is out of range");
        }
#endif
        return channelValues_[channelIndex];
    }
    const T& operator[](const int channelIndex) const {
#ifdef DEBUG
        if (channelIndex < 0 || channelIndex > 2) {
            throw Exception("rev::Color<T>::operator[]", "channel index is out of range");
        }
#endif
        return channelValues_[channelIndex];
    }
        
private:
    T channelValues_[3];
};
    
template <typename T> class Image {
public:
    Image() : width_(0), height_(0), channelTotal_(0), widthStep_(0) {}
    Image(const int width, const int height, const int channelTotal);
    Image(const Image<T>& sourceImage);
    
    void resize(const int width, const int height, const int channelTotal);
    void setTo(const T pixelValue);
    void setTo(const Color<T>& colorValue);
    
    Image<T>& operator=(const Image<T>& sourceImage);
    
    T& operator()(const int x, const int y, const int channelIndex);
    const T& operator()(const int x, const int y, const int channelIndex) const;
    T& operator()(const int x, const int y);
    const T& operator()(const int x, const int y) const;
    T& at(const int x, const int y, const int channelIndex);
    const T& at(const int x, const int y, const int channelIndex) const;
    T& at(const int x, const int y);
    const T& at(const int x, const int y) const;
    void setColor(const int x, const int y, const Color<T>& colorValue);
    Color<T> getColor(const int x, const int y) const;

    int width() const { return width_; }
    int height() const { return height_; }
    int channelTotal() const { return channelTotal_; }
    int widthStep() const { return widthStep_; }
    T* dataPointer() const { return imageData_.pointer(); }
    
private:
    int width_;
    int height_;
    int channelTotal_;
    int widthStep_;
    AlignedMemory<T> imageData_;
};


Image<unsigned char> convertToGrayscale(const Image<unsigned char>& colorImage);
Image<unsigned char> convertToColor(const Image<unsigned char>& grayscaleImage);
    
// Image file I/O
Image<unsigned char> readImageFile(const std::string imageFilename);
Image<unsigned char> readImageFileGrayscale(const std::string imageFilename);
Image<unsigned char> readImageFileColor(const std::string imageFilename);
Image<unsigned short> read16bitImageFile(const std::string imageFilename);
void writeImageFile(const std::string imageFilename, const Image<unsigned char>& image);
void write16bitImageFile(const std::string imageFilename, const Image<unsigned short>& image);
    
    
template <typename T> Image<T>::Image(const int width, const int height, const int channelTotal) {
    if (width < 1 || height < 1 || channelTotal < 1) {
        throw Exception("rev::Image<T>::Image", "image size is less than zero");
    }
    
    width_ = width;
    height_ = height;
    channelTotal_ = channelTotal;
    widthStep_ = width_*channelTotal_ + REVLIB_ALIGNMENT_SIZE - 1 - (width_*channelTotal_ - 1)%REVLIB_ALIGNMENT_SIZE;
    imageData_.resize(widthStep_*height_);
}
    
template <typename T> Image<T>::Image(const Image<T>& sourceImage) {
    width_ = sourceImage.width_;
    height_ = sourceImage.height_;
    channelTotal_ = sourceImage.channelTotal_;
    widthStep_ = sourceImage.widthStep_;
    imageData_.resize(widthStep_*height_);
    memcpy(imageData_.pointer(), sourceImage.imageData_.pointer(), sizeof(T)*widthStep_*height_);
}
    
template <typename T> void Image<T>::resize(const int width, const int height, const int channelTotal) {
    if (width == width_ && height == height_ && channelTotal == channelTotal_) return;
    
    if (width < 1 || height < 1 || channelTotal < 1) {
        throw Exception("rev::Image<T>::resize", "image size is less than zero");
    }
    
    width_ = width;
    height_ = height;
    channelTotal_ = channelTotal;
    widthStep_ = width_*channelTotal_ + REVLIB_ALIGNMENT_SIZE - 1 - (width_*channelTotal_ - 1)%REVLIB_ALIGNMENT_SIZE;
    imageData_.resize(widthStep_*height_);
}
    
template <typename T> void Image<T>::setTo(const T pixelValue) {
    int dataTotalPerRegister = 16/sizeof(T);
    AlignedMemory<T> pixelValueBaseArray(dataTotalPerRegister);
    for (int i = 0; i < dataTotalPerRegister; ++i) pixelValueBaseArray.pointer()[i] = pixelValue;
    __m128i pixelValueRegister = _mm_load_si128(reinterpret_cast<__m128i*>(pixelValueBaseArray.pointer()));
    
    for (int y = 0; y < height_; ++y) {
        T* dataLineHead = imageData_.pointer() + widthStep_*y;
        for (int x = 0; x < widthStep_; x += dataTotalPerRegister) {
            _mm_store_si128(reinterpret_cast<__m128i*>(dataLineHead + x), pixelValueRegister);
        }
    }
}
   
template <typename T> void Image<T>::setTo(const Color<T>& colorValue) {
    if (channelTotal_ != 3) {
        throw Exception("rev::Image<T>::setTo(const Color<T>& colorValue)", "this image is not a color image");
    }
        
    int dataTotalPerRegister = 16/sizeof(T);
    AlignedMemory<T> firstColorValueBaseArray(dataTotalPerRegister);
    AlignedMemory<T> secondColorValueBaseArray(dataTotalPerRegister);
    AlignedMemory<T> thirdColorValueBaseArray(dataTotalPerRegister);
    for (int i = 0; i < dataTotalPerRegister; ++i) {
        firstColorValueBaseArray.pointer()[i] = colorValue[i%3];
        secondColorValueBaseArray.pointer()[i] = colorValue[(i+1)%3];
        thirdColorValueBaseArray.pointer()[i] = colorValue[(i+2)%3];
    }
    __m128i firstColorValueRegister = _mm_load_si128(reinterpret_cast<__m128i*>(firstColorValueBaseArray.pointer()));
    __m128i secondColorValueRegister = _mm_load_si128(reinterpret_cast<__m128i*>(secondColorValueBaseArray.pointer()));
    __m128i thirdColorValueRegister = _mm_load_si128(reinterpret_cast<__m128i*>(thirdColorValueBaseArray.pointer()));
        
    for (int y = 0; y < height_; ++y) {
        T* dataLineHead = imageData_.pointer() + widthStep_*y;
        for (int linePointer = 0; linePointer < widthStep_; linePointer += dataTotalPerRegister*3) {
            _mm_store_si128(reinterpret_cast<__m128i*>(dataLineHead + linePointer), firstColorValueRegister);
            _mm_store_si128(reinterpret_cast<__m128i*>(dataLineHead + linePointer + dataTotalPerRegister), secondColorValueRegister);
            _mm_store_si128(reinterpret_cast<__m128i*>(dataLineHead + linePointer + dataTotalPerRegister*2), thirdColorValueRegister);
        }
    }
}
    
template <typename T> Image<T>& Image<T>::operator=(const Image<T>& sourceImage) {
    if (&sourceImage != this) {
        width_ = sourceImage.width_;
        height_ = sourceImage.height_;
        channelTotal_ = sourceImage.channelTotal_;
        widthStep_ = sourceImage.widthStep_;
        imageData_.resize(widthStep_*height_);
        memcpy(imageData_.pointer(), sourceImage.imageData_.pointer(), sizeof(T)*widthStep_*height_);
    }
    
    return *this;
}
    
template <typename T> inline T& Image<T>::operator()(const int x, const int y, const int channelIndex) {
#ifdef DEBUG
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || channelIndex < 0 || channelIndex >= channelTotal_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y, const int channelIndex)", "access to out of range");
    }
#endif
        
    return imageData_.pointer()[widthStep_*y + channelTotal_*x + channelIndex];
}
    
template <typename T> inline const T& Image<T>::operator()(const int x, const int y, const int channelIndex) const {
#ifdef DEBUG
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || channelIndex < 0 || channelIndex >= channelTotal_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y, const int channelIndex) const", "access to out of range");
    }
#endif
    
    return imageData_.pointer()[widthStep_*y + channelTotal_*x + channelIndex];
}
    
template <typename T> inline T& Image<T>::operator()(const int x, const int y) {
#ifdef DEBUG
    if (channelTotal_ != 1) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y)", "this image is not single channel image");
    }
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y)", "access to out of range");
    }
#endif
        
    return imageData_.pointer()[widthStep_*y + x];
}
    
template <typename T> inline const T& Image<T>::operator()(const int x, const int y) const {
#ifdef DEBUG
    if (channelTotal_ != 1) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y) const", "this image is not single channel image");
    }
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y) const", "access to out of range");
    }
#endif
    
    return imageData_.pointer()[widthStep_*y + x];
}
    
template <typename T> T& Image<T>::at(const int x, const int y, const int channelIndex) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || channelIndex < 0 || channelIndex >= channelTotal_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y, const int channelIndex)", "access to out of range");
    }
        
    return imageData_.pointer()[widthStep_*y + channelTotal_*x + channelIndex];
}
    
template <typename T> const T& Image<T>::at(const int x, const int y, const int channelIndex) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || channelIndex < 0 || channelIndex >= channelTotal_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y, const int channelIndex) const", "access to out of range");
    }
        
    return imageData_.pointer()[widthStep_*y + channelTotal_*x + channelIndex];
}
    
template <typename T> T& Image<T>::at(const int x, const int y) {
    if (channelTotal_ != 1) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y)", "this image is not single channel image");
    }
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y)", "access to out of range");
    }
        
    return imageData_.pointer()[widthStep_*y + x];
}
    
template <typename T> const T& Image<T>::at(const int x, const int y) const {
    if (channelTotal_ != 1) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y) const", "this image is not single channel image");
    }
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw Exception("rev::Image<T>::operator()(const int x, const int y) const", "access to out of range");
    }
        
    return imageData_.pointer()[widthStep_*y + x];
}
    
template <typename T> void Image<T>::setColor(const int x, const int y, const Color<T>& colorValue) {
#ifdef DEBUG
    if (channelTotal_ != 3) {
        throw Exception("rev::Image<T>::setColor", "this image is not a color image");
    }
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw Exception("rev::Image<T>::setColor", "access to out of range");
    }
#endif
        
    imageData_.pointer()[widthStep_*y + 3*x] = colorValue[0];
    imageData_.pointer()[widthStep_*y + 3*x + 1] = colorValue[1];
    imageData_.pointer()[widthStep_*y + 3*x + 2] = colorValue[2];
}
    
template <typename T> Color<T> Image<T>::getColor(const int x, const int y) const {
#ifdef DEBUG
    if (channelTotal_ != 3) {
        throw Exception("rev::Image<T>::getColor", "this image is not a color image");
    }
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw Exception("rev::Image<T>::getColor", "access to out of range");
    }
#endif
        
    Color<T> colorValue;
    colorValue[0] = imageData_.pointer()[widthStep_*y + 3*x];
    colorValue[1] = imageData_.pointer()[widthStep_*y + 3*x + 1];
    colorValue[2] = imageData_.pointer()[widthStep_*y + 3*x + 2];
        
    return colorValue;
}

}

#endif
