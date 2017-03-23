#include <iostream>
#include <string>
#include "cmdline.h"
#include "Image.h"
#include "SgmStereo.h"

struct ParameterSgmStereo {
    bool verbose;
    int disparityTotal;
    double outputDisparityFactor;
    std::string leftImageFilename;
    std::string rightImageFilename;
    std::string outputLeftDisparityImageFilename;
    std::string outputRightDisparityImageFilename;
};

// Prototype declaration
cmdline::parser makeCommandParser();
ParameterSgmStereo parseCommandline(int argc, char* argv[]);

cmdline::parser makeCommandParser() {
    cmdline::parser commandParser;
    commandParser.add<std::string>("output", 'o', "output directory", false, "");
    commandParser.add<int>("disparity", 'd', "number of disparities", false, 256);
    commandParser.add<double>("factor", 'f', "disparity factor of output disparity image", false, 256);
    commandParser.add("verbose", 'v', "verbose");
    commandParser.add("help", 'h', "display this message");
    commandParser.footer("left_image right_image");
    commandParser.set_program_name("sgmstereo");
    
    return commandParser;
}

ParameterSgmStereo parseCommandline(int argc, char* argv[]) {
    // Make command parser
    cmdline::parser commandParser = makeCommandParser();
    // Parse command line
    bool isCorrectCommand = commandParser.parse(argc, argv);
    if (!isCorrectCommand) {
        std::cerr << commandParser.error() << std::endl;
    }
    if (!isCorrectCommand || commandParser.exist("help") || commandParser.rest().size() < 2) {
        std::cerr << commandParser.usage() << std::endl;
        exit(1);
    }
    
    // Set program parameters
    ParameterSgmStereo parameters;
    // Verbose flag
    parameters.verbose = commandParser.exist("verbose");
    // The number of disparities
    parameters.disparityTotal = commandParser.get<int>("disparity");
    // Disparity factor
    parameters.outputDisparityFactor = commandParser.get<double>("factor");
    // Input stereo images
    parameters.leftImageFilename = commandParser.rest()[0];
    parameters.rightImageFilename = commandParser.rest()[1];
    // Output directory
    std::string outputDirectoryname = commandParser.get<std::string>("output");
    if (outputDirectoryname == "") {
        // Same directory as input
        // Output left disparity image
        std::string outputLeftDisparityImageFilename = parameters.leftImageFilename;
        size_t dotPosition = outputLeftDisparityImageFilename.rfind('.');
        if (dotPosition != std::string::npos) outputLeftDisparityImageFilename.erase(dotPosition);
        outputLeftDisparityImageFilename = outputLeftDisparityImageFilename + "_left_disparity.png";
        parameters.outputLeftDisparityImageFilename = outputLeftDisparityImageFilename;
        // Output right disparity image
        std::string outputRightDisparityImageFilename = parameters.rightImageFilename;
        dotPosition = outputRightDisparityImageFilename.rfind('.');
        if (dotPosition != std::string::npos) outputRightDisparityImageFilename.erase(dotPosition);
        outputRightDisparityImageFilename = outputRightDisparityImageFilename + "_right_disparity.png";
        parameters.outputRightDisparityImageFilename = outputRightDisparityImageFilename;
    } else {
        // Output directory is specified
        // Output left disparity image
        std::string outputLeftDisparityImageFilename = parameters.leftImageFilename;
        size_t slashPosition = outputLeftDisparityImageFilename.rfind('/');
        if (slashPosition != std::string::npos) outputLeftDisparityImageFilename.erase(0, slashPosition+1);
        size_t dotPosition = outputLeftDisparityImageFilename.rfind('.');
        if (dotPosition != std::string::npos) outputLeftDisparityImageFilename.erase(dotPosition);
        outputLeftDisparityImageFilename = outputDirectoryname + "/" + outputLeftDisparityImageFilename + "_left_disparity.png";
        parameters.outputLeftDisparityImageFilename = outputLeftDisparityImageFilename;
        // Output right disparity image
        std::string outputRightDisparityImageFilename = parameters.rightImageFilename;
        slashPosition = outputRightDisparityImageFilename.rfind('/');
        if (slashPosition != std::string::npos) outputRightDisparityImageFilename.erase(0, slashPosition+1);
        dotPosition = outputRightDisparityImageFilename.rfind('.');
        if (dotPosition != std::string::npos) outputRightDisparityImageFilename.erase(dotPosition);
        outputRightDisparityImageFilename = outputDirectoryname + "/" + outputRightDisparityImageFilename + "_right_disparity.png";
        parameters.outputRightDisparityImageFilename = outputRightDisparityImageFilename;
    }
    
    return parameters;
}

int main(int argc, char* argv[]) {
    // Parse command line
    ParameterSgmStereo parameters = parseCommandline(argc, argv);
    
    // Output parameters
    if (parameters.verbose) {
        std::cerr << std::endl;
        std::cerr << "Left image:             " << parameters.leftImageFilename << std::endl;
        std::cerr << "Right image:            " << parameters.rightImageFilename << std::endl;
        std::cerr << "Output left disparity:  " << parameters.outputLeftDisparityImageFilename << std::endl;
        std::cerr << "Output right disparity: " << parameters.outputRightDisparityImageFilename << std::endl;
        std::cerr << "   #disparities:     " << parameters.disparityTotal << std::endl;
        std::cerr << "   output format:    ";
        std::cerr << "   disparity factor: " << parameters.outputDisparityFactor << std::endl;
        std::cerr << std::endl;
    }
    
    try {
        // Open images
        rev::Image<unsigned char> leftImage = rev::readImageFile(parameters.leftImageFilename);
        rev::Image<unsigned char> rightImage = rev::readImageFile(parameters.rightImageFilename);
        
        // SGM Stereo
        SgmStereo sgmStereo;
        sgmStereo.setDisparityTotal(parameters.disparityTotal);
        sgmStereo.setOutputDisparityFactor(parameters.outputDisparityFactor);
        
        // Compute disparity images
        rev::Image<unsigned short> leftDisparityImage;
        rev::Image<unsigned short> rightDisparityImage;
        sgmStereo.computeLeftRight(leftImage, rightImage, leftDisparityImage, rightDisparityImage);
            
        // Write disparity images
        rev::write16bitImageFile(parameters.outputLeftDisparityImageFilename, leftDisparityImage);
        rev::write16bitImageFile(parameters.outputRightDisparityImageFilename, rightDisparityImage);
        
    } catch (const rev::Exception& revException) {
        std::cerr << "Error [" << revException.functionName() << "]: " << revException.message() << std::endl;
        exit(1);
    }
}
