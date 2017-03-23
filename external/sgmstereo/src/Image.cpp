#include "Image.h"
#include <fstream>
#include <sstream>
#include <png.h>
#include "Exception.h"

namespace rev{

Image<unsigned char> convertToGrayscale(const Image<unsigned char>& colorImage) {
    if (colorImage.channelTotal() == 1) return colorImage;
        
    int width = colorImage.width();
    int height = colorImage.height();
    Image<unsigned char> grayscaleImage(width, height, 1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color<unsigned char> pixelColor = colorImage.getColor(x, y);
            unsigned char pixelGrayscaleValue = static_cast<unsigned char>(0.299*pixelColor[0] + 0.587*pixelColor[1] + 0.114*pixelColor[2]);
            grayscaleImage(x, y) = pixelGrayscaleValue;
        }
    }
        
    return grayscaleImage;
}
    
Image<unsigned char> convertToColor(const Image<unsigned char>& grayscaleImage) {
    if (grayscaleImage.channelTotal() == 3) return grayscaleImage;
        
    int width = grayscaleImage.width();
    int height = grayscaleImage.height();
    Image<unsigned char> colorImage(width, height, 3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char pixelValue = grayscaleImage(x, y);
            colorImage.setColor(x, y, Color<unsigned char>(pixelValue, pixelValue, pixelValue));
        }
    }
        
    return colorImage;
}
    
    
// Image file I/O
// Prototype declaration
std::string getFilenameExtension(const std::string filename);
Image<unsigned char> readPGM(const std::string imageFilename);
Image<unsigned char> readPPM(const std::string imageFilename);
Image<unsigned char> readPNG(const std::string imageFilename);
Image<unsigned short> read16bitPNG(const std::string imageFilename);
void writePGM(const std::string imageFilename, const Image<unsigned char>& image);
void writePPM(const std::string imageFilename, const Image<unsigned char>& image);
void writePNG(const std::string imageFilename, const Image<unsigned char>& image);
void write16bitPNG(const std::string imageFilename, const Image<unsigned short>& image);
    
Image<unsigned char> readImageFile(const std::string imageFilename) {
    Image<unsigned char> image;
        
    std::string filenameExtension = getFilenameExtension(imageFilename);
    if (filenameExtension == "pgm" || filenameExtension == "PGM") {
        image = readPGM(imageFilename);
    } else if (filenameExtension == "ppm" || filenameExtension == "PPM") {
        image = readPPM(imageFilename);
    } else if (filenameExtension == "png" || filenameExtension == "PNG") {
        image = readPNG(imageFilename);
    } else {
        throw Exception("rev::readImageFile", "unsupport type of image file");
    }
        
    return image;
}
    
Image<unsigned char> readImageFileGrayscale(const std::string imageFilename) {
    Image<unsigned char> image = readImageFile(imageFilename);
    if (image.channelTotal() != 1) {
        image = convertToGrayscale(image);
    }
        
    return image;
}
    
Image<unsigned char> readImageFileColor(const std::string imageFilename) {
    Image<unsigned char> image = readImageFile(imageFilename);
    if (image.channelTotal() != 3) {
        image = convertToColor(image);
    }
        
    return image;
}
    
Image<unsigned short> read16bitImageFile(const std::string imageFilename) {
    Image<unsigned short> image;
        
    std::string filenameExtension = getFilenameExtension(imageFilename);
    if (filenameExtension == "png" || filenameExtension == "PNG") {
        image = read16bitPNG(imageFilename);
    } else {
        throw Exception("rev::read16bitImageFile", "unsupport type of image file");
    }
        
    return image;
}
    
void writeImageFile(const std::string imageFilename, const Image<unsigned char>& image) {
    std::string filenameExtension = getFilenameExtension(imageFilename);
    if (filenameExtension == "pgm" || filenameExtension == "PGM") {
        writePGM(imageFilename, image);
    } else if (filenameExtension == "ppm" || filenameExtension == "PPM") {
        writePPM(imageFilename, image);
    } else if (filenameExtension == "png" || filenameExtension == "PNG") {
        writePNG(imageFilename, image);
    } else {
        throw Exception("rev::writeImageFile", "unsupport type of image file");
    }
}
    
void write16bitImageFile(const std::string imageFilename, const Image<unsigned short>& image) {
    std::string filenameExtension = getFilenameExtension(imageFilename);
    if (filenameExtension == "png" || filenameExtension == "PNG") {
        write16bitPNG(imageFilename, image);
    } else {
        throw Exception("rev::writeImageFile", "unsupport type of image file");
    }
}
    
    
std::string getFilenameExtension(const std::string filename) {
    std::string filenameExtension = filename;
    size_t dotPosition = filenameExtension.rfind('.');
    if (dotPosition != std::string::npos) {
        filenameExtension.erase(0, dotPosition+1);
    } else {
        filenameExtension = "";
    }
        
    return filenameExtension;
}
    
Image<unsigned char> readPGM(const std::string imageFilename) {
    // Open file
    std::ifstream imageFileStream(imageFilename.c_str(), std::ios_base::in | std::ios_base::binary);
    if (imageFileStream.fail()) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("readPGM", errorMessage.str());
    }
    // Check header
    std::string header;
    imageFileStream >> header;
    if (header != "P2" && header != "P5") {
        throw Exception("readPGM", "invalid header");
    }
    imageFileStream.get();
    // Skip comment
    while (imageFileStream.peek() == '#') {
        while (imageFileStream.peek() != '\n') {
            imageFileStream.get();
        }
        imageFileStream.get();
    }
    // Image size
    int width;
    imageFileStream >> width;
    int height;
    imageFileStream >> height;
    int maxValue;
    imageFileStream >> maxValue;
    imageFileStream.get();
        
    // Initialize image
    Image<unsigned char> image(width, height, 1);
    // Read image data
    if (header == "P2") {
        // ASCII
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int pixelValue;
                imageFileStream >> pixelValue;
                image(x, y) = static_cast<unsigned char>(pixelValue);
                imageFileStream.get();
                if ((x < width-1 || y < height-1) && imageFileStream.eof()) {
                    throw Exception("readPGM", "read error");
                }
            }
        }
    } else {
        // Binary
        unsigned char* imageDataBuffer = new unsigned char [width*height];
        imageFileStream.read(reinterpret_cast<char*>(imageDataBuffer), sizeof(unsigned char)*width*height);
        if (imageFileStream.bad()) {
            throw Exception("readPGM", "read error");
        }
        int bufferIndex = 0;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                image(x, y) = imageDataBuffer[bufferIndex];
                ++bufferIndex;
            }
        }
        delete [] imageDataBuffer;
    }
    imageFileStream.close();

    return image;
}
    
Image<unsigned char> readPPM(const std::string imageFilename) {
    // Open file
    std::ifstream imageFileStream(imageFilename.c_str(), std::ios_base::in | std::ios_base::binary);
    if (imageFileStream.fail()) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("readPPM", errorMessage.str());
    }
    // Check header
    std::string header;
    imageFileStream >> header;
    if (header != "P3" && header != "P6") {
        throw Exception("readPPM", "invalid header");
    }
    // Skip comment
    while (imageFileStream.peek() == '#') {
        while (imageFileStream.peek() != '\n') {
            imageFileStream.get();
        }
        imageFileStream.get();
    }
    // Image size
    int width;
    imageFileStream >> width;
    int height;
    imageFileStream >> height;
    int maxValue;
    imageFileStream >> maxValue;
    imageFileStream.get();
    
    // Initiazlie image
    Image<unsigned char> image(width, height, 3);
    // Read image data
    if (header == "P3") {
        // ASCII
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                for (int channelIndex = 0; channelIndex < 3; ++channelIndex) {
                    int pixelValue;
                    imageFileStream >> pixelValue;
                    image(x, y, channelIndex) = static_cast<unsigned char>(pixelValue);
                    imageFileStream.get();
                    if ((x < width-1 || y < height-1 || channelIndex < 2) && imageFileStream.eof()) {
                        throw Exception("readPPM", "read error");
                    }
                }
            }
        }
    } else {
        // Binary
        unsigned char* imageDataBuffer = new unsigned char [width*height*3];
        imageFileStream.read(reinterpret_cast<char*>(imageDataBuffer), sizeof(unsigned char)*width*height*3);
        if (imageFileStream.bad()) {
            throw Exception("readPPM", "read error");
        }
        int bufferIndex = 0;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                for (int channelIndex = 0; channelIndex < 3; ++channelIndex) {
                    image(x, y, channelIndex) = imageDataBuffer[bufferIndex];
                    ++bufferIndex;
                }
            }
        }
        delete [] imageDataBuffer;
    }
    imageFileStream.close();
    
    return image;
}

Image<unsigned char> readPNG(const std::string imageFilename) {
    // Open file
    FILE* inputImageFilePointer = fopen(imageFilename.c_str(), "rb");
    if (inputImageFilePointer == 0) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw  Exception("rev::readPNG", errorMessage.str());
    }
    // Initialize PNG struct
    png_structp pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pngInfo = png_create_info_struct(pngStruct);
    png_init_io(pngStruct, inputImageFilePointer);
    // Read header
    unsigned int width, height;
    int bitDepth, colorType, interlaceType;
    png_read_info(pngStruct, pngInfo);
    png_get_IHDR(pngStruct, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, NULL, NULL);
    // Read image data
    png_bytepp imageDataArray = reinterpret_cast<png_bytepp>(malloc(sizeof(png_bytepp)*height));
    for (int i = 0; i < height; ++i) {
        imageDataArray[i] = reinterpret_cast<png_bytep>(malloc(png_get_rowbytes(pngStruct, pngInfo)));
    }
    png_read_image(pngStruct, imageDataArray);
        
    // The number of channels
    int channelTotal;
    if ((colorType == 0 || colorType == 2) && bitDepth == 8) channelTotal = colorType + 1;
    else if (colorType == 3 && bitDepth == 1) channelTotal = 1;
    else throw Exception("rev::readPNG", "unsupport image type");
        
    // Initialize image
    Image<unsigned char> image(width, height, channelTotal);
    // Copy image data
    if (bitDepth == 8) {
        unsigned char* imageData = image.dataPointer();
        int widthStep = image.widthStep();
        for (int y = 0; y < height; ++y) {
            memcpy(imageData + widthStep*y, imageDataArray[y], width*channelTotal);
        }
    } else {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int arrayIndex = x/8;
                int bitIndex = 7 - x%8;
                int bitValue = (imageDataArray[y][arrayIndex] >> bitIndex)&0x1;
                if (bitValue == 0) image(x, y) = 0;
                else image(x, y) = 255;
            }
        }
    }
        
    for (int i = 0; i < height; ++i) free(imageDataArray[i]);
    free(imageDataArray);
    png_destroy_read_struct(&pngStruct, &pngInfo, NULL);
    fclose(inputImageFilePointer);

    return image;
}
    
Image<unsigned short> read16bitPNG(const std::string imageFilename) {
    // Open file
    FILE* inputImageFilePointer = fopen(imageFilename.c_str(), "rb");
    if (inputImageFilePointer == 0) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("rev::read16bitPNG", errorMessage.str());
    }
    // Initialize PNG struct
    png_structp pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pngInfo = png_create_info_struct(pngStruct);
    png_init_io(pngStruct, inputImageFilePointer);
    // Read header
    unsigned int width, height;
    int bitDepth, colorType, interlaceType;
    png_read_info(pngStruct, pngInfo);
    png_get_IHDR(pngStruct, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, NULL, NULL);
    // Read image data
    png_bytepp imageDataArray = reinterpret_cast<png_bytepp>(malloc(sizeof(png_bytepp)*height));
    for (int i = 0; i < height; ++i) {
        imageDataArray[i] = reinterpret_cast<png_bytep>(malloc(png_get_rowbytes(pngStruct, pngInfo)));
    }
    png_read_image(pngStruct, imageDataArray);
        
    // The number of channels
    int channelTotal;
    if ((colorType == 0 || colorType == 2) && bitDepth == 16) channelTotal = colorType + 1;
    else throw Exception("read16bitPNG", "unsupport image type");
        
    // Initialize image
    Image<unsigned short> image(width, height, channelTotal);
    // Copy image data
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int channelIndex = 0; channelIndex < channelTotal; ++channelIndex) {
                unsigned short originalValue = reinterpret_cast<unsigned short*>(imageDataArray[y])[channelTotal*x+channelIndex];
                int highBitValue = originalValue/256;
                int lowBitValue = originalValue - 256*highBitValue;
                image(x, y, channelIndex) = lowBitValue*256 + highBitValue;
            }
        }
    }
        
    for (int i = 0; i < height; ++i) free(imageDataArray[i]);
    free(imageDataArray);
    png_destroy_read_struct(&pngStruct, &pngInfo, NULL);
    fclose(inputImageFilePointer);
        
    return image;
}
    
void writePGM(const std::string imageFilename, const Image<unsigned char>& image) {
    Image<unsigned char> grayscaleImage = convertToGrayscale(image);
    
    // Copy image data to array
    int width = grayscaleImage.width();
    int height = grayscaleImage.height();
    unsigned char* imageDataBuffer = new unsigned char [width*height];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            imageDataBuffer[width*y + x] = grayscaleImage(x, y);
        }
    }
    
    // Open output file
    std::ofstream outputFileStream(imageFilename.c_str(), std::ios_base::out | std::ios_base::binary);
    if (outputFileStream.fail()) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("writePGM", errorMessage.str());
    }
    outputFileStream << "P5" << std::endl;
    outputFileStream << width << " " << height << std::endl;
    outputFileStream << "255" << std::endl;
    outputFileStream.write(reinterpret_cast<char*>(imageDataBuffer), sizeof(unsigned char)*width*height);
    if (outputFileStream.bad()) {
        throw Exception("writePGM", "write error");
    }
    outputFileStream.close();
    delete [] imageDataBuffer;
}
    
void writePPM(const std::string imageFilename, const Image<unsigned char>& image) {
    Image<unsigned char> colorImage = convertToColor(image);
    
    // Copy image data to array
    int width = colorImage.width();
    int height = colorImage.height();
    unsigned char* imageDataBuffer = new unsigned char [width*height*3];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int channelIndex = 0; channelIndex < 3; ++channelIndex) {
                imageDataBuffer[width*3*y + 3*x + channelIndex] = colorImage(x, y, channelIndex);
            }
        }
    }
    
    // Open output file
    std::ofstream outputFileStream(imageFilename.c_str(), std::ios_base::out | std::ios_base::binary);
    if (outputFileStream.fail()) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("writePPM", errorMessage.str());
    }
    outputFileStream << "P6" << std::endl;
    outputFileStream << width << " " << height << std::endl;
    outputFileStream << "255" << std::endl;
    outputFileStream.write(reinterpret_cast<char*>(imageDataBuffer), sizeof(unsigned char)*width*height*3);
    if (outputFileStream.bad()) {
        throw Exception("writePPM", "write error");
    }
    outputFileStream.close();
    delete [] imageDataBuffer;
}
    
void writePNG(const std::string imageFilename, const Image<unsigned char>& image) {
    // Image size
    unsigned int width = image.width();
    unsigned int height = image.height();
    int channelTotal = image.channelTotal();
    int colorType = channelTotal - 1;
        
    // Open output file
    FILE* outputImageFilePointer = fopen(imageFilename.c_str(), "wb");
    if (outputImageFilePointer == 0) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("rev::writePNG", errorMessage.str());
    }
        
    // Create PNG header
    png_structp pngStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pngInfo = png_create_info_struct(pngStruct);
    png_init_io(pngStruct, outputImageFilePointer);
    png_set_IHDR(pngStruct, pngInfo, width, height, 8, colorType, 0, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pngStruct, pngInfo);
    // Copy image data
    png_bytepp imageDataArray = reinterpret_cast<png_bytepp>(malloc(sizeof(png_bytepp)*height));
    size_t rowBytes = png_get_rowbytes(pngStruct, pngInfo);
    for (int i = 0; i < height; ++i) {
        imageDataArray[i] = reinterpret_cast<png_bytep>(malloc(rowBytes));
    }
    unsigned char* imageData = image.dataPointer();
    int widthStep = image.widthStep();
    for (int y = 0; y < height; ++y) {
        memcpy(imageDataArray[y], imageData + widthStep*y, rowBytes);
    }
    // Write image data
    png_write_image(pngStruct, imageDataArray);
    png_write_end(pngStruct, pngInfo);
        
    for (int i = 0; i < height; ++i) free(imageDataArray[i]);
    free(imageDataArray);
    png_destroy_write_struct(&pngStruct, &pngInfo);
    fclose(outputImageFilePointer);
}
    
void write16bitPNG(const std::string imageFilename, const Image<unsigned short>& image) {
    // Image size
    unsigned int width = image.width();
    unsigned int height = image.height();
    int channelTotal = image.channelTotal();
    int colorType = channelTotal - 1;
        
    // Open output file
    FILE* outputImageFilePointer = fopen(imageFilename.c_str(), "wb");
    if (outputImageFilePointer == 0) {
        std::stringstream errorMessage;
        errorMessage << "can't open image file (" << imageFilename << ")";
        throw Exception("rev::write16bitPNG", errorMessage.str());
    }
        
    // Create PNG header
    png_structp pngStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pngInfo = png_create_info_struct(pngStruct);
    png_init_io(pngStruct, outputImageFilePointer);
    png_set_IHDR(pngStruct, pngInfo, width, height, 16, colorType, 0, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pngStruct, pngInfo);
    // Copy image data
    png_bytepp imageDataArray = reinterpret_cast<png_bytepp>(malloc(sizeof(png_bytepp)*height));
    for (int i = 0; i < height; ++i) {
        imageDataArray[i] = reinterpret_cast<png_bytep>(malloc(png_get_rowbytes(pngStruct, pngInfo)));
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int channelIndex = 0; channelIndex < channelTotal; ++channelIndex) {
                int originalValue = image(x, y, channelIndex);
                int highBitValue = originalValue/256;
                int lowBitValue = originalValue - 256*highBitValue;
                reinterpret_cast<unsigned short*>(imageDataArray[y])[channelTotal*x + channelIndex] = 256*lowBitValue + highBitValue;
            }
        }
    }
    // Write image data
    png_write_image(pngStruct, imageDataArray);
    png_write_end(pngStruct, pngInfo);
        
    for (int i = 0; i < height; ++i) free(imageDataArray[i]);
    free(imageDataArray);
    png_destroy_write_struct(&pngStruct, &pngInfo);
    fclose(outputImageFilePointer);
}
    
}
