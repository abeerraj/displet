####################################################################################
# This is free software; you can redistribute it and/or modify it under the		     #
# terms of the GNU General Public License as published by the Free Software        #
# Foundation; either version 2 of the License, or any later version.               #
#                                                                                  #
# This code is distributed in the hope that it will be useful, but WITHOUT ANY     #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A  #
# PARTICULAR PURPOSE. See the GNU General Public License for more details.         #
#                                                                                  #
# You should have received a copy of the GNU General Public License along with     #
# this code; if not, write to the Free Software Foundation, Inc., 51 Franklin      #
# Street, Fifth Floor, Boston, MA 02110-1301, USA                                  #
####################################################################################

This is the reference implementation of 'Displets: Resolving Stereo Ambiguities
using Object Knowledge' as described in 

@INPROCEEDINGS{Guney2015CVPR,
  author = {Fatma Güney and Andreas Geiger},
  title = {Displets: Resolving Stereo Ambiguities using Object Knowledge},
  booktitle = {Conference on Computer Vision and Pattern Recognition (CVPR)},
  year = {2015}
}

Building the mex Files
======================

We use mex files in

- algo/boundaries/
- algo/pairwise/
- external/librender/
- external/libtrws/

and provide the binary-mex files compiled on a 64-bit Linux machine. To
recompile these for other architectures, please go to these folders and
type make in Matlab.

The code was developed and tested under Matlab 2014b and Ubuntu 14.04.
We do not provide any support for compilation issues or other OS.

Running the Demo
================

1) Go to http://www.cvlibs.net/projects/displets/ and download the
   pre-processed data files for the KITTI dataset. Extract the files to
   the 'data' directory such that the 'data' directory will contain the
   subdirectories 'Kitti' and 'models'.

2) run: callMain('data', 'release', 'training', 'cnn', 2);
   This call will process the training image number 2 using CNN features.
   The experiment name is 'release', the results will be stored in
   'data/Kitti/training/release'. All the parameters are explained inside
   the file. To reproduce the experiments in the paper, increase the
   iteration number from 10 to 40.

Optional: The sampled displets are provided in the pre-processed data
          package. If you want to sample the displets yourself, the same
          script can be used by commenting the call to main and
          uncommenting the call to run_sampling. Sampled displets are
          saved in the folder 'data/Kitti/training/displets'. Note that to
          achieve the runtimes reported in the paper, a parallelized
          version of this code (with 12 threads) has been used.

Input Data
==========

The input data folder consists of two subfolders: 

- models: The car models we used to sample displets together with their
          simplified versions via the proposed semi-convex hull algorithm.
          To simplify your own CAD models, you can use our algorithm which
          we provide as a separate library at:
          http://www.cvlibs.net/software/semi_convex_hull/
- dataset folder (Kitti): We provide the input data we used in
  our algorithm for both training and test set.
    - image_0, image_1, calib, colored_0: Following the Kitti [1] directory
      structure, they correspond to the images from left camera, right camera,
      calibration files, and colored left images, respectively.
    - dispmaps: We provide our initial (reference frame) disparity images
      for both SGM [2] and CNN [3]. For processing a new dataset you have
      to run the 3rd party sgmstereo and stereoslic code.
    - stereoslic: This folder contains superpixel images and resulting
      disparity images that we use to initialize plane parameters at the
      begining of our algorithm. Note that executables of sgmstereo and
      stereoslic produce files with suffixes, we rename them to the original
      file names following the Kitti convention and place them in the
      respective folders.
    - ale_sem_seg: We modify ALE library (external folder) to segment car
      objects from the background on Kitti and provide the results in this
      folder. It's possible to run the algorithm on new images by placing
      them in external/ALE/Kitti/Images/Test/ folder and running the
      executable (./ale). We provide the training files by following the
      directory structure required by the library. The algorithm utilizes
      all available cores on the machine to parallelize over the images.
      Then, the results are saved in external/ALE/Kitti/Result/Crf/Test/. 
      To train the algorithm on new data, training images and corresponding
      groundtruth should be supplied in external/ALE/Kitti/Images/Train/
      and external/ALE/Kitti/GroundTruth/Train/ folders respectively and
      external/ALE/main.cpp should be modified (by uncommenting the lines
      from 27 to 30) and recompiled by typing make.
      Note that ALE uses Developer's Image library (DevIL). 
      Please check ALE's README file for details.
    - displets: We provide the pre-computed displets in this folder. 

Dependencies
============

To make the zip file self-contained, it includes copies of

a) An early version of StereoSLIC: http://ttic.uchicago.edu/~dmcallester/SPS/ 
   
   @InProceedings{Yamaguchi2013CVPR,
     Title                    = {Robust Monocular Epipolar Flow Estimation},
     Author                   = {K. Yamaguchi and D. McAllester and R. Urtasun},
     Booktitle                = {CVPR},
     Year                     = {2013}
   }

   @InProceedings{Yamaguchi2014ECCV,
			Title = {Efficient joint segmentation, occlusion labeling, stereo and flow estimation},
			Author = {Yamaguchi, Koichiro and McAllester, David and Urtasun, Raquel},
			Booktitle = {ECCV},
			Year = {2014}
	 }
   
b) ALE: http://www.inf.ethz.ch/personal/ladickyl/
   
   @Article{Ladicky2014PAMI,
     Title                    = {Associative Hierarchical Random Fields},
     Author                   = {Lubor Ladicky and Christopher Russell and Pushmeet Kohli and Philip H. S. Torr},
     Journal                  = PAMI,
     Year                     = {2014},
     Number                   = {6},
     Pages                    = {1056--1077},
     Volume                   = {36}
   }
		
c) librender: http://www.cvlibs.net/software/librender/

	 @INPROCEEDINGS{Geiger2015GCPR,
     author = {Andreas Geiger and Chaohui Wang},
     title = {Joint 3D Object and Layout Inference from a single RGB-D Image},
     booktitle = {German Conference on Pattern Recognition (GCPR)},
     year = {2015}
   } 

d) TRW-S: http://pub.ist.ac.at/~vnk/papers/TRW-S.html

   @Article{Kolmogorov2006PAMI,
     Title                    = {Convergent Tree-Reweighted Message Passing for Energy Minimization},
     Author                   = {Kolmogorov, Vladimir},
     Journal                  = PAMI,
     Year                     = {2006},
     Number                   = {10},
     Pages                    = {1568-1583},
     Volume                   = {28},
   }

e) MATLAB KDE toolbox: http://www.ics.uci.edu/~ihler/code/kde.html

Citation
========

If you find this project, data or code useful, we would be happy if you cite us:

@INPROCEEDINGS{Guney2015CVPR,
  author = {Fatma Güney and Andreas Geiger},
  title = {Displets: Resolving Stereo Ambiguities using Object Knowledge},
  booktitle = {Conference on Computer Vision and Pattern Recognition (CVPR)},
  year = {2015}
}

References
==========

[1] A. Geiger, P. Lenz, and R. Urtasun. Are we ready for autonomous driving? The KITTI vision benchmark suite. 
    Computer Vision and Pattern Recognition (CVPR), 2012.
    http://www.cvlibs.net/datasets/kitti/

[2] H. Hirschmueller. Stereo processing by semiglobal matching and mutual information. 
    Pattern Analysis and Machine Intelligence (PAMI), 30(2):328–341, 2008.

[3] J. Zbontar and Y. LeCun. Computing the stereo matching cost with a convolutional neural network.
    Computer Vision and Pattern Recognition (CVPR), 2015.

[3] K. Yamaguchi, D. McAllester, and R. Urtasun. Robust monocular epipolar flow estimation. 
    Computer Vision and Pattern Recognition (CVPR), 2013.

[4] L. Ladicky, C. Russell, P. Kohli, and P. H. S. Torr. Associative hierarchical random fields.
    Pattern Analysis and Machine Intelligence (PAMI), 36(6):1056–1077, 2014.
