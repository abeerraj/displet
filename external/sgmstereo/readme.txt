// Build
1) cd png
2) Type 'cmake .'
3) Type 'make'
4) cd ..
5) Type 'cmake .'
6) Type 'make'

// Run
Usage: ./sgmstereo imageL.png imageR.png

Inputs:
   imageL.png: left image
   imageR.png: right image

Output:
   imageL_left_disparity.png: left disparity image (KITTI format: 16bit grayscale multiplied by 256)
   imageR_right_disparity.png: right disparity image (KITTI format: 16bit grayscale multiplied by 256)
