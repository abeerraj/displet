#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#include <fnmatch.h>
#endif

#include "dataset.h"
#include "potential.h"
#include "crf.h"

#include <fstream>
#include <iostream>

LDataset::LDataset()
{
}

LDataset::~LDataset()
{
	int i;
	for(i = 0; i < allImageFiles.GetCount(); i++) delete[] allImageFiles[i];
}

void LDataset::RgbToLabel(unsigned char *rgb, unsigned char *label)
{
	label[0] = 0;
	for(int i = 0; i < 8; i++) label[0] = (label[0] << 3) | (((rgb[0] >> i) & 1) << 0) | (((rgb[1] >> i) & 1) << 1) | (((rgb[2] >> i) & 1) << 2);
}

void LDataset::LabelToRgb(unsigned char *label, unsigned char *rgb)
{
	unsigned char lab = label[0];
	rgb[0] = rgb[1] = rgb[2] = 0;
    for(int i = 0; lab > 0; i++, lab >>= 3)
	{
		rgb[0] |= (unsigned char) (((lab >> 0) & 1) << (7 - i));
		rgb[1] |= (unsigned char) (((lab >> 1) & 1) << (7 - i));
		rgb[2] |= (unsigned char) (((lab >> 2) & 1) << (7 - i));
	}
}

char *LDataset::GetFolderFileName(const char *imageFile, const char *folder, const char *extension)
{
	char *fileName;
	fileName = new char[strlen(imageFile) + strlen(folder) - strlen(imageFolder) + strlen(extension) - strlen(imageExtension) + 1];
	strcpy(fileName, folder);
	strncpy(fileName + strlen(folder), imageFile + strlen(imageFolder), strlen(imageFile) - strlen(imageFolder) - strlen(imageExtension));
	strcpy(fileName + strlen(imageFile) + strlen(folder) - strlen(imageFolder) - strlen(imageExtension), extension);
	return(fileName);
}

int LDataset::SortStr(char *str1, char *str2)
{
	return(strcmp(str1, str2));
};

void LDataset::LoadFolder(const char *folder, const char *extension, LList<char *> &list)
{
	char *fileName, *folderExt;

#ifdef _WIN32	
	_finddata_t info;
	int hnd;
	int done;

	folderExt = new char[strlen(folder) + strlen(extension) + 2];
	sprintf(folderExt, "%s*%s", folder, extension);
	
	hnd = (int)_findfirst(folderExt, &info);
	done = (hnd == -1);

	while(!done)
	{
		info.name[strlen(info.name) - strlen(extension)] = 0;
		fileName = new char[strlen(info.name) + 1];
		strcpy(fileName, info.name);
		list.Add(fileName);
		done = _findnext(hnd, &info);
	}
	_findclose(hnd);
#else
	struct dirent **nameList = NULL;
	int count;

	folderExt = new char[strlen(extension) + 2];
	sprintf(folderExt, "*%s", extension);
	
	count = scandir(folder, &nameList, NULL, alphasort);
	if(count >= 0)
	{
	      for(int i = 0; i < count; i++)
	      {
		      if(!fnmatch(folderExt, nameList[i]->d_name, 0))
		      {
			      nameList[i]->d_name[strlen(nameList[i]->d_name) - strlen(extension)] = 0;
			      fileName = new char[strlen(nameList[i]->d_name) + 1];
			      strcpy(fileName, nameList[i]->d_name);
			      list.Add(fileName);
		      }
		      if(nameList[i] != NULL) free(nameList[i]);
	      }
	      if(nameList != NULL) free(nameList);
	}
#endif
	delete[] folderExt;
}

void LDataset::Init()
{
	int index1, index2, i;

	LMath::SetSeed(seed);

	LoadFolder(imageFolder, imageExtension, allImageFiles);

	printf("Permuting image files..\n");
	if(allImageFiles.GetCount() > 0) for(i = 0; i < filePermutations; i++)
	{
		index1 = LMath::RandomInt(allImageFiles.GetCount());
		index2 = LMath::RandomInt(allImageFiles.GetCount());
		allImageFiles.Swap(index1, index2);
	}

	printf("Splitting image files..\n");
	for(i = 0; i < allImageFiles.GetCount(); i++)
	{
		if(i < proportionTrain * allImageFiles.GetCount()) trainImageFiles.Add(allImageFiles[i]);
		else if(i < (proportionTrain + proportionTest) * allImageFiles.GetCount()) testImageFiles.Add(allImageFiles[i]);
	}
}

void LDataset::SaveImage(LLabelImage &labelImage, LCrfDomain *domain, char *fileName)
{
	int points = labelImage.GetPoints();
	unsigned char *labelData = labelImage.GetData();
	for(int j = 0; j < points; j++, labelData++) (*labelData)++;
	labelImage.Save(fileName, domain);
}

int LDataset::Segmented(char *imageFileName)
{
	return(1);
}

void LDataset::GetLabelSet(unsigned char *labelset, char *imageFileName)
{
	memset(labelset, 0, classNo * sizeof(unsigned char));
	char *fileName;
	fileName = GetFileName(groundTruthFolder, imageFileName, groundTruthExtension);
	LLabelImage labelImage(fileName, this, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel);
	delete[] fileName;

	int points = labelImage.GetPoints(), i;
	unsigned char *labelData = labelImage.GetData();
	for(i = 0; i < points; i++, labelData++) if(*labelData) labelset[*labelData - 1] = 1; 
}


LSowerbyDataset::LSowerbyDataset() : LDataset()
{
	seed = 10000;
	classNo = 7;
	filePermutations = 10000;
	optimizeAverage = 1;
	featuresOnline = 0;
	unaryWeighted = 0;

	proportionTrain = 0.5;
	proportionTest = 0.5;

	imageFolder = "Sowerby/Images/";
	imageExtension = ".png";
	groundTruthFolder = "Sowerby/GroundTruth/";
	groundTruthExtension = ".png";
	trainFolder = "Sowerby/Result/Train/";
	testFolder = "Sowerby/Result/Crf/";

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 5;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "Sowerby/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 5;
	siftNumberOfClusters = 150;
	siftSizes[0] = 3, siftSizes[1] = 5, siftSizes[2] = 7;
	siftSizeCount = 3;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "Sowerby/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 5;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 3, colourSiftSizes[1] = 5, colourSiftSizes[2] = 7;
	colourSiftSizeCount = 3;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "Sowerby/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	locationBuckets = 12;
	locationFolder = "Sowerby/Result/Feature/Location/";
	locationExtension = ".loc";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "Sowerby/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 10;
	lbpNumberOfClusters = 150;

	meanShiftXY[0] = 3.5;
	meanShiftLuv[0] = 3.5;
	meanShiftMinRegion[0] = 10;
	meanShiftFolder[0] = "Sowerby/Result/MeanShift/35x35/";
	meanShiftXY[1] = 5.5;
	meanShiftLuv[1] = 3.5;
	meanShiftMinRegion[1] = 10;
	meanShiftFolder[1] = "Sowerby/Result/MeanShift/55x35/";
	meanShiftXY[2] = 3.5;
	meanShiftLuv[2] = 5.5;
	meanShiftMinRegion[2] = 10;
	meanShiftFolder[2] = "Sowerby/Result/MeanShift/35x55/";

	meanShiftExtension = ".msh";

	denseNumRoundsBoosting = 500;
	denseBoostingSubSample = 5;
	denseNumberOfThetas = 20;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 2;
	denseMaximumRectangleSize = 100;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboost.dat";
	denseExtension = ".dns";
	denseFolder = "Sowerby/Result/Dense/";
	denseWeight = 1.0;
	denseMaxClassRatio = 0.2;

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;
	pairwisePrior = 1.5;
	pairwiseFactor = 6.0;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 500;
	statsRandomizationFactor = 0.1;
	statsFactor = 0.5;
	statsAlpha = 0.05;
	statsPrior = 0.0;
	statsMaxClassRatio = 0.5;
	statsTrainFile = "statsboost.dat";
	statsFolder = "Sowerby/Result/Stats/";
	statsExtension = ".sts";

	Init();

	ForceDirectory(trainFolder);
	ForceDirectory(testFolder);
	ForceDirectory(textonFolder);
	ForceDirectory(siftFolder);
	ForceDirectory(colourSiftFolder);
	ForceDirectory(locationFolder);
	ForceDirectory(lbpFolder);
	for(int i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i]);
	ForceDirectory(denseFolder);
	ForceDirectory(statsFolder);
}

void LSowerbyDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[3];
	LSegmentation2D *segmentation[3];

	int i;
	for(i = 0; i < 3; i++)
	{
		segmentation[i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}
	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);
    pixelPotential->AddFeature(textonFeature);
    pixelPotential->AddFeature(siftFeature);
    pixelPotential->AddFeature(coloursiftFeature);
    pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

	crf->potentials.Add(new LEightNeighbourPottsPairwisePixelPotential(this, crf, objDomain, baseLayer, classNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight));

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 3; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;
	crf->potentials.Add(statsPotential);
}

LCorelDataset::LCorelDataset() : LDataset()
{
	seed = 10000;
	classNo = 7;
	filePermutations = 10000;
	optimizeAverage = 1;
	featuresOnline = 0;

	unaryWeighted = 0;
	unaryWeights = new double[classNo];
	for(int i = 0; i < classNo; i++) unaryWeights[i] = 1.0;

	proportionTrain = 0.5;
	proportionTest = 0.5;

	imageFolder = "Corel/Images/";
	imageExtension = ".bmp";
	groundTruthFolder = "Corel/GroundTruth/";
	groundTruthExtension = ".bmp";
	trainFolder = "Corel/Result/Train/";
	testFolder = "Corel/Result/Crf/";

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 5;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "Corel/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 5;
	siftNumberOfClusters = 150;
	siftSizes[0] = 3, siftSizes[1] = 5, siftSizes[2] = 7;
	siftSizeCount = 3;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "Corel/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 5;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 3, colourSiftSizes[1] = 5, colourSiftSizes[2] = 7;
	colourSiftSizeCount = 3;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "Corel/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	locationBuckets = 12;
	locationFolder = "Corel/Result/Feature/Location/";
	locationExtension = ".loc";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "Corel/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 10;
	lbpNumberOfClusters = 150;

	meanShiftXY[0] = 3.5;
	meanShiftLuv[0] = 3.5;
	meanShiftMinRegion[0] = 10;
	meanShiftFolder[0] = "Corel/Result/MeanShift/35x35/";
	meanShiftXY[1] = 5.5;
	meanShiftLuv[1] = 3.5;
	meanShiftMinRegion[1] = 10;
	meanShiftFolder[1] = "Corel/Result/MeanShift/55x35/";
	meanShiftXY[2] = 3.5;
	meanShiftLuv[2] = 5.5;
	meanShiftMinRegion[2] = 10;
	meanShiftFolder[2] = "Corel/Result/MeanShift/35x55/";

	meanShiftExtension = ".msh";

	denseNumRoundsBoosting = 500;
	denseBoostingSubSample = 5;
	denseNumberOfThetas = 20;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 2;
	denseMaximumRectangleSize = 100;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboost.dat";
	denseExtension = ".dns";
	denseFolder = "Corel/Result/Dense/";
	denseWeight = 1.0;
	denseMaxClassRatio = 0.2;

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;
	pairwisePrior = 1.5;
	pairwiseFactor = 6.0;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 500;
	statsRandomizationFactor = 0.1;
	statsFactor = 0.6;
	statsAlpha = 0.05;
	statsPrior = 0.0;
	statsMaxClassRatio = 0.5;
	statsTrainFile = "statsboost.dat";
	statsFolder = "Corel/Result/Stats/";
	statsExtension = ".sts";

	cooccurenceTrainFile = "cooccurence.dat";
	cooccurenceWeight = 0.05;

	pairwiseSegmentBuckets = 8;
	pairwiseSegmentPrior = 0.0;
	pairwiseSegmentFactor = 2.0;
	pairwiseSegmentBeta = 40.0;

	Init();

	ForceDirectory(trainFolder);
	ForceDirectory(testFolder);
	ForceDirectory(textonFolder);
	ForceDirectory(siftFolder);
	ForceDirectory(colourSiftFolder);
	ForceDirectory(locationFolder);
	ForceDirectory(lbpFolder);
	for(int i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i]);
	ForceDirectory(denseFolder);
	ForceDirectory(statsFolder);
}

void LCorelDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[3];
	LSegmentation2D *segmentation[3];

	int i;
	for(i = 0; i < 3; i++)
	{
		segmentation[i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}
/*
//	3 levels hierarchy (replace previous loop)
	segmentation[0] = new LMeanShiftSegmentation2D(meanShiftXY[0], meanShiftLuv[0], meanShiftMinRegion[0], meanShiftFolder[0], meanShiftExtension);
	superpixelLayer[0] = new LPnCrfLayer(crf, this, baseLayer, segmentation[0], cliqueTruncation);
	crf->segmentations.Add(segmentation[0]);
	crf->layers.Add(superpixelLayer[0]);

	for(i = 1; i < 3; i++)
	{
		segmentation[i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, this, superpixelLayer[0], segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}
	consistencyPrior = 100000;
	LConsistencyUnarySegmentPotential *consistencyPotential = new LConsistencyUnarySegmentPotential(this, crf, classNo, consistencyPrior);
	consistencyPotential->AddLayer(superpixelLayer[0]);
	crf->potentials.Add(consistencyPotential);
	crf->potentials.Add(new LHistogramPottsPairwiseSegmentPotential(this, crf, superpixelLayer[0], classNo, pairwiseSegmentPrior, pairwiseSegmentFactor, pairwiseSegmentBeta, pairwiseSegmentBuckets));
*/

	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);
    pixelPotential->AddFeature(textonFeature);
    pixelPotential->AddFeature(siftFeature);
    pixelPotential->AddFeature(coloursiftFeature);
    pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

/*
//	random forest for unary pixel potential (replace previous 3 lines)
	const char *denseForestFile = "denseforest.dat";
	int denseTrees = 20;
	int denseDepth = 10;
	double denseForestDataRatio = 1.0;

	LRandomForest<int> *pixelForest = new LRandomForest<int>(trainFolder, "denseforest.dat", classNo, denseTrees, denseDepth, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int *, int, int))&LDenseUnaryPixelPotential::GetTrainForestValues, (int (LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetEvalForestValue, denseForestDataRatio);
	crf->learnings.Add(pixelForest);
	pixelPotential->learning = pixelForest;
*/
	crf->potentials.Add(new LEightNeighbourPottsPairwisePixelPotential(this, crf, objDomain, baseLayer, classNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight));

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 3; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;
	crf->potentials.Add(statsPotential);

/*
//	linear svm for unary segment potential (replace previous boosting lines)
	const char *statsLinSVMFile = "statslinsvm.dat";
	double statsLinLambda = 1e-1;
	int statsLinEpochs = 500;
	int statsLinSkip = 400;
	double statsLinBScale = 1e-2;
	
	LLinearSVM<double> *segmentLinSVM = new LLinearSVM<double>(trainFolder, statsLinSVMFile, classNo, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainSVMValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalSVMValues, statsLinLambda, statsLinEpochs, statsLinSkip, statsLinBScale);
	crf->learnings.Add(segmentLinSVM);
	statsPotential->learning = segmentLinSVM;
*/

/*
//	approx kernel svm for unary segment potential (replace previous boosting lines)
	const char *statsApproxSVMFile = "statsapproxsvm.dat";
	double statsApproxLambda = 1e-5;
	int statsApproxEpochs = 500;
	int statsApproxSkip = 400;
	double statsApproxBScale = 1e-2;
	double statsApproxL = 0.0005;
	int statsApproxCount = 3;
	LApproxKernel<double> *kernel = new LIntersectionApproxKernel<double>();
	
	LApproxKernelSVM<double> *segmentApproxSVM = new LApproxKernelSVM<double>(trainFolder, statsApproxSVMFile, classNo, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainSVMValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalSVMValues, 1e-5, 500, 400, 1e-2, kernel, statsApproxL, statsApproxCount);
	crf->learnings.Add(segmentApproxSVM);
	statsPotential->learning = segmentApproxSVM;
*/

/*
//	random forest for unary segment potential (replace previous boosting lines)
	const char *statsForestFile = "statsforest.dat";
	int statsTrees = 50;
	int statsDepth = 12;
	double statsForestDataRatio = 1.0;

	LRandomForest<double> *segmentForest = new LRandomForest<double>(trainFolder, statsForestFile, classNo, statsTrees, statsDepth, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalSVMValues, statsForestDataRatio);
	crf->learnings.Add(segmentForest);
	statsPotential->learning = segmentForest;
*/

	LPreferenceCrfLayer *preferenceLayer = new LPreferenceCrfLayer(crf, objDomain, this, baseLayer);
	crf->layers.Add(preferenceLayer);
	crf->potentials.Add(new LCooccurencePairwiseImagePotential(this, crf, objDomain, preferenceLayer, trainFolder, cooccurenceTrainFile, classNo, cooccurenceWeight));
}

LMsrcDataset::LMsrcDataset() : LDataset()
{
	seed = 60000;
	featuresOnline = 0;
	unaryWeighted = 0;

	classNo = 21;
	filePermutations = 10000;
	optimizeAverage = 1;

	trainFileList = "Msrc/Train.txt";
	testFileList = "Msrc/Test.txt";

	proportionTrain = 0.5;
	proportionTest = 0.5;

	imageFolder = "Msrc/Images/";
	imageExtension = ".bmp";
	groundTruthFolder = "Msrc/GroundTruth/";
	groundTruthExtension = ".bmp";
	trainFolder = "Msrc/Result/Train/";
	testFolder = "Msrc/Result/Crf/";

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	locationBuckets = 12;
	locationFolder = "Msrc/Result/Feature/Location/";
	locationExtension = ".loc";

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 5;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "Msrc/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 10;
	siftNumberOfClusters = 150;
	siftSizes[0] = 3, siftSizes[1] = 5, siftSizes[2] = 7, siftSizes[3] = 9;
	siftSizeCount = 4;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "Msrc/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 10;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 3, colourSiftSizes[1] = 5, colourSiftSizes[2] = 7, colourSiftSizes[3] = 9;
	colourSiftSizeCount = 4;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "Msrc/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "Msrc/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 10;
	lbpNumberOfClusters = 150;

	denseNumRoundsBoosting = 5000;
	denseBoostingSubSample = 5;
	denseNumberOfThetas = 25;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 5;
	denseMaximumRectangleSize = 200;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboostXX.dat";
	denseExtension = ".dns";
	denseFolder = "Msrc/Result/DenseXX/";
	denseWeight = 1.0;
	denseMaxClassRatio = 0.1;

	meanShiftXY[0] = 7.0;
	meanShiftLuv[0] = 6.5;
	meanShiftMinRegion[0] = 20;
	meanShiftFolder[0] = "Msrc/Result/MeanShift/70x65/";
	meanShiftXY[1] = 7.0;
	meanShiftLuv[1] = 9.5;
	meanShiftMinRegion[1] = 20;
	meanShiftFolder[1] = "Msrc/Result/MeanShift/70x95/";
	meanShiftXY[2] = 7.0;
	meanShiftLuv[2] = 14.5;
	meanShiftMinRegion[2] = 20;
	meanShiftFolder[2] = "Msrc/Result/MeanShift/70x145/";
	meanShiftExtension = ".msh";

	kMeansDistance[0] = 30;
	kMeansXyLuvRatio[0] = 1.0;
	kMeansFolder[0] = "Msrc/Result/KMeans/30/";
	kMeansDistance[1] = 40;
	kMeansXyLuvRatio[1] = 0.75;
	kMeansFolder[1] = "Msrc/Result/KMeans/40/";
	kMeansDistance[2] = 50;
	kMeansXyLuvRatio[2] = 0.6;
	kMeansFolder[2] = "Msrc/Result/KMeans/50/";
	kMeansIterations = 10;
	kMeansMaxDiff = 3;
	kMeansExtension = ".seg";

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;
	pairwisePrior = 1.6;
	pairwiseFactor = 6.4;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 5000;
	statsRandomizationFactor = 0.1;
	statsFactor = 0.5;
	statsAlpha = 0.05;
	statsPrior = 0;
	statsMaxClassRatio = 0.5;
	statsTrainFile = "statsboost.dat";
	statsFolder = "Msrc/Result/Stats/";
	statsExtension = ".sts";

	cooccurenceTrainFile = "cooccurence.dat";
	cooccurenceWeight = 0.006;

//	Init();
	LDataset::Init();

	int i;
	ForceDirectory(trainFolder);
	ForceDirectory(testFolder);
	ForceDirectory(textonFolder);
	ForceDirectory(siftFolder);
	ForceDirectory(colourSiftFolder);
	ForceDirectory(locationFolder);
	ForceDirectory(lbpFolder);
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i]);
	for(i = 0; i < 3; i++) ForceDirectory(kMeansFolder[i]);
	ForceDirectory(denseFolder);
	ForceDirectory(statsFolder);
}

void LMsrcDataset::RgbToLabel(unsigned char *rgb, unsigned char *label)
{
	label[0] = 0;
	for(int i = 0; i < 8; i++) label[0] = (label[0] << 3) | (((rgb[0] >> i) & 1) << 0) | (((rgb[1] >> i) & 1) << 1) | (((rgb[2] >> i) & 1) << 2);

	if((label[0] == 5) || (label[0] == 8) || (label[0] == 19) || (label[0] == 20)) label[0] = 0;
	if(label[0] > 20) label[0] -= 4;
	else if(label[0] > 8) label[0] -= 2;
	else if(label[0] > 5) label[0]--;
}

void LMsrcDataset::LabelToRgb(unsigned char *label, unsigned char *rgb)
{
	unsigned char lab = label[0];
	if(lab > 16) lab += 4;
	else if(lab > 6) lab += 2;
	else if(lab > 4) lab++;

	rgb[0] = rgb[1] = rgb[2] = 0;
    for(int i = 0; lab > 0; i++, lab >>= 3)
	{
		rgb[0] |= (unsigned char) (((lab >> 0) & 1) << (7 - i));
		rgb[1] |= (unsigned char) (((lab >> 1) & 1) << (7 - i));
		rgb[2] |= (unsigned char) (((lab >> 2) & 1) << (7 - i));
	}
}

void LMsrcDataset::Init()
{
	FILE *f;
	char *fileName, name[20];

	f = fopen(trainFileList, "r");

	if(f != NULL)
	{
		fgets(name, 15, f);
		name[strlen(name) - 1] = 0;
		size_t size = strlen(name);
		while(size > 0)
		{
			fileName = new char[strlen(name) + 1];
			strncpy(fileName, name, strlen(name) - 4);
			fileName[strlen(name) - 4] = 0;

			trainImageFiles.Add(fileName);
			allImageFiles.Add(fileName);

			if(fgets(name, 15, f) == NULL) name[0] = 0;
			if(strlen(name) > 0) name[strlen(name) - 1] = 0;
			size = strlen(name);
		}
		fclose(f);
	}
	f = fopen(testFileList, "r");

	if(f != NULL)
	{
		fgets(name, 15, f);
		name[strlen(name) - 1] = 0;
		size_t size = strlen(name);
		while(size > 0)
		{
			fileName = new char[strlen(name) + 1];
			strncpy(fileName, name, strlen(name) - 4);
			fileName[strlen(name) - 4] = 0;
			testImageFiles.Add(fileName);
			allImageFiles.Add(fileName);

			if(fgets(name, 15, f) == NULL) name[0] = 0;
			if(strlen(name) > 0) name[strlen(name) - 1] = 0;
			size = strlen(name);
		}
		fclose(f);
	}
}

void LMsrcDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[6];
	LSegmentation2D *segmentation[6];

	int i;

	for(i = 0; i < 3; i++)
	{
		segmentation[i] = new LKMeansSegmentation2D(kMeansXyLuvRatio[i], kMeansDistance[i], kMeansIterations, kMeansMaxDiff, kMeansFolder[i], kMeansExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}
	for(i = 0; i < 3; i++)
	{
		segmentation[3 + i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[3 + i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[3 + i], cliqueTruncation);
		crf->segmentations.Add(segmentation[3 + i]);
		crf->layers.Add(superpixelLayer[3 + i]);
	}
	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);
    pixelPotential->AddFeature(textonFeature);
    pixelPotential->AddFeature(siftFeature);
    pixelPotential->AddFeature(coloursiftFeature);
    pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

	crf->potentials.Add(new LEightNeighbourPottsPairwisePixelPotential(this, crf, objDomain, baseLayer, classNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight));

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 6; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;
	crf->potentials.Add(statsPotential);

	LPreferenceCrfLayer *preferenceLayer = new LPreferenceCrfLayer(crf, objDomain, this, baseLayer);
	crf->layers.Add(preferenceLayer);
	crf->potentials.Add(new LCooccurencePairwiseImagePotential(this, crf, objDomain, preferenceLayer, trainFolder, cooccurenceTrainFile, classNo, cooccurenceWeight));
}

LVOCDataset::LVOCDataset() : LDataset()
{
	seed = 10000;
	featuresOnline = 0;
	unaryWeighted = 0;

	classNo = 21;
	optimizeAverage = 1;

	imageFolder = "VOC2010/Images/";
	imageExtension = ".jpg";
	groundTruthFolder = "VOC2010/GroundTruth/";
	groundTruthExtension = ".png";

	trainFileList = "VOC2010/trainval.txt";
	testFileList = "VOC2010/test.txt";

	trainFolder = "VOC2010/Result/Train/";
	denseFolder = "VOC2010/Result/Dense/";
	statsFolder = "VOC2010/Result/Stats/";
	testFolder = "VOC2010/Result/Crf/";

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	locationBuckets = 20;
	locationFolder = "VOC2010/Result/Feature/Location/";
	locationExtension = ".loc";

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 20;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "VOC2010/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 40;
	siftNumberOfClusters = 150;
	siftSizes[0] = 5, siftSizes[1] = 7, siftSizes[2] = 9, siftSizes[3] = 11;
	siftSizeCount = 4;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "VOC2010/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 40;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 5, colourSiftSizes[1] = 7, colourSiftSizes[2] = 9, colourSiftSizes[3] = 11;
	colourSiftSizeCount = 4;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "VOC2010/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "VOC2010/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 20;
	lbpNumberOfClusters = 150;

	denseNumRoundsBoosting = 5000;
	denseBoostingSubSample = 10;
	denseNumberOfThetas = 15;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 10;
	denseMaximumRectangleSize = 250;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboost.dat";
	denseExtension = ".dns";
	denseWeight = 1.0;
	denseMaxClassRatio = 1.0;

	meanShiftXY[0] = 7.0;
	meanShiftLuv[0] = 7.0;
	meanShiftMinRegion[0] = 20;
	meanShiftFolder[0] = "VOC2010/Result/Segmentation/MeanShift70x70/";
	meanShiftXY[1] = 7.0;
	meanShiftLuv[1] = 10.0;
	meanShiftMinRegion[1] = 20;
	meanShiftFolder[1] = "VOC2010/Result/Segmentation/MeanShift70x100/";
	meanShiftXY[2] = 10.0;
	meanShiftLuv[2] = 7.0;
	meanShiftMinRegion[2] = 20;
	meanShiftFolder[2] = "VOC2010/Result/Segmentation/MeanShift100x70/";
	meanShiftXY[3] = 10.0;
	meanShiftLuv[3] = 10.0;
	meanShiftMinRegion[3] = 20;
	meanShiftFolder[3] = "VOC2010/Result/Segmentation/MeanShift100x100/";
	meanShiftExtension = ".seg";

	kMeansDistance[0] = 30;
	kMeansXyLuvRatio[0] = 1.0;
	kMeansFolder[0] = "VOC2010/Result/Segmentation/KMeans30/";
	kMeansDistance[1] = 40;
	kMeansXyLuvRatio[1] = 0.75;
	kMeansFolder[1] = "VOC2010/Result/Segmentation/KMeans40/";
	kMeansDistance[2] = 50;
	kMeansXyLuvRatio[2] = 0.6;
	kMeansFolder[2] = "VOC2010/Result/Segmentation/KMeans50/";
	kMeansDistance[3] = 60;
	kMeansXyLuvRatio[3] = 0.5;
	kMeansFolder[3] = "VOC2010/Result/Segmentation/KMeans60/";
	kMeansDistance[4] = 80;
	kMeansXyLuvRatio[4] = 0.375;
	kMeansFolder[4] = "VOC2010/Result/Segmentation/KMeans80/";
	kMeansDistance[5] = 100;
	kMeansXyLuvRatio[5] = 0.3;
	kMeansFolder[5] = "VOC2010/Result/Segmentation/KMeans100/";
	kMeansIterations = 5;
	kMeansMaxDiff = 2;
	kMeansExtension = ".seg";

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;

	pairwisePrior = 3.0;
	pairwiseFactor = 12.0;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	consistencyPrior = 0.05;

	pairwiseSegmentBuckets = 8;
	pairwiseSegmentPrior = 0.0;
	pairwiseSegmentFactor = 2.0;
	pairwiseSegmentBeta = 40.0;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 5000;
	statsRandomizationFactor = 0.1;
	statsPrior = 0;
	statsFactor = 0.3;
	statsMaxClassRatio = 1.0;
	statsAlpha = 0.05;
	statsTrainFile = "statsboost.dat";
	statsExtension = ".sts";

	cooccurenceTrainFile = "cooccurence.dat";
	cooccurenceWeight = 0.05;

	Init();

	int i;
	ForceDirectory(trainFolder);
	ForceDirectory(testFolder);
	ForceDirectory(textonFolder);
	ForceDirectory(siftFolder);
	ForceDirectory(colourSiftFolder);
	ForceDirectory(locationFolder);
	ForceDirectory(lbpFolder);
	for(i = 0; i < 4; i++) ForceDirectory(meanShiftFolder[i]);
	for(i = 0; i < 6; i++) ForceDirectory(kMeansFolder[i]);
	ForceDirectory(denseFolder);
	ForceDirectory(statsFolder);
}

void LVOCDataset::RgbToLabel(unsigned char *rgb, unsigned char *label)
{
	label[0] = 0;
   	if((rgb[0] != 255) || (rgb[1] != 255) || (rgb[2] != 255))
	{
		for(int i = 0; i < 8; i++) label[0] = (label[0] << 3) | (((rgb[0] >> i) & 1) << 0) | (((rgb[1] >> i) & 1) << 1) | (((rgb[2] >> i) & 1) << 2);
		label[0]++;
	}
}

void LVOCDataset::LabelToRgb(unsigned char *label, unsigned char *rgb)
{
	unsigned char lab = label[0];
	if(label[0] == 0) rgb[0] = rgb[1] = rgb[2] = 255;
	else
	{
		lab--;
		rgb[0] = rgb[1] = rgb[2] = 0;

		for(int i = 0; lab > 0; i++, lab >>= 3)
		{
			rgb[0] |= (unsigned char) (((lab >> 0) & 1) << (7 - i));
			rgb[1] |= (unsigned char) (((lab >> 1) & 1) << (7 - i));
			rgb[2] |= (unsigned char) (((lab >> 2) & 1) << (7 - i));
		}
	}
}

void LVOCDataset::Init()
{
	FILE *f;
	char *fileName, name[12];

	f = fopen(trainFileList, "rb");

	if(f != NULL)
	{
		size_t size = fread(name, 1, 12, f);
		while(size == 12)
		{
			name[11] = 0;

			fileName = new char[strlen(name) + 1];
			strcpy(fileName, name);
			trainImageFiles.Add(fileName);
			allImageFiles.Add(fileName);

			size = fread(name, 1, 12, f);
		}
		fclose(f);
	}

	f = fopen(testFileList, "rb");

	if(f != NULL)
	{
		size_t size = fread(name, 1, 12, f);
		while(size == 12)
		{
			name[11] = 0;

			fileName = new char[strlen(name) + 1];
			strcpy(fileName, name);
			testImageFiles.Add(fileName);
			allImageFiles.Add(fileName);

			size = fread(name, 1, 12, f);
		}
		fclose(f);
	}
}

void LVOCDataset::SaveImage(LLabelImage &labelImage, LCrfDomain *domain, char *fileName)
{
	labelImage.Save8bit(fileName);
}

void LVOCDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[10];
	LSegmentation2D *segmentation[10];

	int i;
	for(i = 0; i < 6; i++)
	{
		segmentation[i] = new LKMeansSegmentation2D(kMeansXyLuvRatio[i], kMeansDistance[i], kMeansIterations, kMeansMaxDiff, kMeansFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}
	for(i = 0; i < 4; i++)
	{
		segmentation[i + 6] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i + 6] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i + 6], cliqueTruncation);
		crf->segmentations.Add(segmentation[i + 6]);
		crf->layers.Add(superpixelLayer[i + 6]);
	}

	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);

	pixelPotential->AddFeature(textonFeature);
    pixelPotential->AddFeature(siftFeature);
    pixelPotential->AddFeature(coloursiftFeature);
	pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

	crf->potentials.Add(new LEightNeighbourPottsPairwisePixelPotential(this, crf, objDomain, baseLayer, classNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight));

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio, 1);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 10; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;

	crf->potentials.Add(statsPotential);

	LPreferenceCrfLayer *preferenceLayer = new LPreferenceCrfLayer(crf, objDomain, this, baseLayer);
	crf->layers.Add(preferenceLayer);
	crf->potentials.Add(new LCooccurencePairwiseImagePotential(this, crf, objDomain, preferenceLayer, trainFolder, cooccurenceTrainFile, classNo, cooccurenceWeight));
}

LLeuvenDataset::LLeuvenDataset() : LDataset()
{
	seed = 60000;
	featuresOnline = 0;
	unaryWeighted = 0;

	classNo = 7;
	filePermutations = 10000;
	optimizeAverage = 1;

	imageFolder = "Leuven/Images/Left/";
	imageExtension = ".png";
	groundTruthFolder = "Leuven/GroundTruth/";
	groundTruthExtension = ".png";
	trainFolder = "Leuven/Result/Train/";
	testFolder = "Leuven/Result/Crf/";

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	locationBuckets = 12;
	locationFolder = "Leuven/Result/Feature/Location/";
	locationExtension = ".loc";

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 5;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "Leuven/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 5;
	siftNumberOfClusters = 150;
	siftSizes[0] = 3, siftSizes[1] = 5, siftSizes[2] = 7, siftSizes[3] = 9;
	siftSizeCount = 4;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "Leuven/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 5;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 3, colourSiftSizes[1] = 5, colourSiftSizes[2] = 7, colourSiftSizes[3] = 9;
	colourSiftSizeCount = 4;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "Leuven/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "Leuven/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 5;
	lbpNumberOfClusters = 150;

	denseNumRoundsBoosting = 5000;
	denseBoostingSubSample = 5;
	denseNumberOfThetas = 25;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 5;
	denseMaximumRectangleSize = 200;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboost.dat";
	denseExtension = ".dns";
	denseFolder = "Leuven/Result/Dense/";
	denseWeight = 1.0;
	denseMaxClassRatio = 0.1;

	meanShiftXY[0] = 4.0;
	meanShiftLuv[0] = 2.0;
	meanShiftMinRegion[0] = 50;
	meanShiftFolder[0] = "Leuven/Result/MeanShift/40x20/";
	meanShiftXY[1] = 6.0;
	meanShiftLuv[1] = 3.0;
	meanShiftMinRegion[1] = 100;
	meanShiftFolder[1] = "Leuven/Result/MeanShift/60x30/";
	meanShiftXY[2] = 10.0;
	meanShiftLuv[2] = 3.5;
	meanShiftMinRegion[2] = 20;
	meanShiftFolder[2] = "Leuven/Result/MeanShift/100x35/";
	meanShiftExtension = ".msh";

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;
	pairwisePrior = 1.5;
	pairwiseFactor = 6.0;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 5000;
	statsRandomizationFactor = 0.1;
	statsFactor = 0.5;
	statsAlpha = 0.05;
	statsPrior = 0;
	statsMaxClassRatio = 0.5;
	statsTrainFile = "statsboost.dat";
	statsFolder = "Leuven/Result/Stats/";
	statsExtension = ".sts";

	dispTestFolder = "Leuven/Result/DispCrf/";
	disparityLeftFolder = "Leuven/Images/Left/";
	disparityRightFolder = "Leuven/Images/Right/";
	disparityGroundTruthFolder = "Leuven/DepthGroundTruth/";
	disparityGroundTruthExtension = ".png";
	disparityUnaryFactor = 0.1;
	disparityRangeMoveSize = 0;

	disparityFilterSigma = 5.0;
	disparityMaxDistance = 500;
	disparityDistanceBeta = 500;
	disparitySubSample = 1;
	disparityMaxDelta = 2;

	disparityPairwiseFactor = 0.00005;
	disparityPairwiseTruncation = 10.0;
	disparityClassNo = 100;

	cameraBaseline = 150;
	cameraHeight = 200;
	cameraFocalLength = 472.391;
	cameraAspectRatio = 0.8998;
	cameraWidthOffset = -2.0;
	cameraHeightOffset = -3.0;

	crossUnaryWeight = 0.5;
	crossPairwiseWeight = -1e-4;
	crossMinHeight = 0;
	crossMaxHeight = 1000;
	crossHeightClusters = 80;
	crossThreshold = 1e-6;
	crossTrainFile = "height.dat";

	Init();

	int i;
	ForceDirectory(trainFolder);
	ForceDirectory(testFolder, "train/");
	ForceDirectory(textonFolder, "train/");
	ForceDirectory(siftFolder, "train/");
	ForceDirectory(colourSiftFolder, "train/");
	ForceDirectory(locationFolder, "train/");
	ForceDirectory(lbpFolder, "train/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "train/");
	ForceDirectory(denseFolder, "train/");
	ForceDirectory(statsFolder, "train/");
	ForceDirectory(dispTestFolder, "train/");
	ForceDirectory(testFolder, "test/");
	ForceDirectory(textonFolder, "test/");
	ForceDirectory(siftFolder, "test/");
	ForceDirectory(colourSiftFolder, "test/");
	ForceDirectory(locationFolder, "test/");
	ForceDirectory(lbpFolder, "test/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "test/");
	ForceDirectory(denseFolder, "test/");
	ForceDirectory(statsFolder, "test/");
	ForceDirectory(dispTestFolder, "test/");
}

void LLeuvenDataset::AddFolder(char *folder, LList<char *> &fileList)
{
        char *fileName, *folderExt;
	
#ifdef _WIN32	
	_finddata_t info;
	int hnd;
	int done;

	folderExt = new char[strlen(imageFolder) + strlen(folder) + strlen(imageExtension) + 2];
	sprintf(folderExt, "%s%s*%s", imageFolder, folder, imageExtension);
	
	hnd = (int)_findfirst(folderExt, &info);
	done = (hnd == -1);

	while(!done)
	{
		info.name[strlen(info.name) - strlen(imageExtension)] = 0;
		fileName = new char[strlen(folder) + strlen(info.name) + 1];
		sprintf(fileName, "%s%s", folder, info.name);
		fileList.Add(fileName);
		allImageFiles.Add(fileName);
		done = _findnext(hnd, &info);
	}
	_findclose(hnd);
#else
	char *wholeFolder;
	struct dirent **nameList = NULL;
	int count;

	folderExt = new char[strlen(imageExtension) + 2];
	sprintf(folderExt, "*%s", imageExtension);

	wholeFolder = new char[strlen(imageFolder) + strlen(folder) + 1];
	sprintf(wholeFolder, "%s%s", imageFolder, folder);
	
	count = scandir(wholeFolder, &nameList, NULL, alphasort);
	if(count >= 0)
	{
	      for(int i = 0; i < count; i++)
	      {
		      if(!fnmatch(folderExt, nameList[i]->d_name, 0))
		      {
			      nameList[i]->d_name[strlen(nameList[i]->d_name) - strlen(imageExtension)] = 0;
			      fileName = new char[strlen(folder) + strlen(nameList[i]->d_name) + 1];
			      sprintf(fileName, "%s%s", folder, nameList[i]->d_name);
			      fileList.Add(fileName);
			      allImageFiles.Add(fileName);
		      }
		      if(nameList[i] != NULL) free(nameList[i]);
	      }
	      if(nameList != NULL) free(nameList);
	}
	delete[] wholeFolder;
#endif
	delete[] folderExt;
}

void LLeuvenDataset::Init()
{
	AddFolder("train/", trainImageFiles);
	AddFolder("test/", testImageFiles);
}

void LLeuvenDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[3];
	LSegmentation2D *segmentation[3];

	int i;
	for(i = 0; i < 3; i++)
	{
		segmentation[i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}

	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);
    pixelPotential->AddFeature(textonFeature);
    pixelPotential->AddFeature(siftFeature);
    pixelPotential->AddFeature(coloursiftFeature);
    pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 3; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;
	crf->potentials.Add(statsPotential);

	LCrfDomain *dispDomain = new LCrfDomain(crf, this, disparityClassNo, dispTestFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LLeuvenDataset::DisparityRgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LLeuvenDataset::DisparityLabelToRgb);
	crf->domains.Add(dispDomain);

	LBaseCrfLayer *dispBaseLayer = new LBaseCrfLayer(crf, dispDomain, this, disparityRangeMoveSize);
	crf->layers.Add(dispBaseLayer);

	LDisparityUnaryPixelPotential *pixelDispPotential = new LDisparityUnaryPixelPotential(this, crf, dispDomain, dispBaseLayer, disparityClassNo, disparityUnaryFactor, disparityFilterSigma, disparityMaxDistance, disparityDistanceBeta, disparitySubSample, disparityMaxDelta, disparityLeftFolder, disparityRightFolder);
	crf->potentials.Add(pixelDispPotential);

	LHeightUnaryPixelPotential *heightPotential = new LHeightUnaryPixelPotential(this, crf, objDomain, baseLayer, dispDomain, dispBaseLayer, trainFolder, crossTrainFile, classNo, crossUnaryWeight, disparityClassNo, cameraBaseline, cameraHeight, cameraFocalLength, cameraAspectRatio, cameraWidthOffset, cameraHeightOffset, crossMinHeight, crossMaxHeight, crossHeightClusters, crossThreshold, disparitySubSample);
	crf->potentials.Add(heightPotential);

	crf->potentials.Add(new LJointPairwisePixelPotential(this, crf, objDomain, baseLayer, dispDomain, dispBaseLayer, classNo, disparityClassNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight, disparityPairwiseFactor, disparityPairwiseTruncation, crossPairwiseWeight));
}

void LLeuvenDataset::DisparityRgbToLabel(unsigned char *rgb, unsigned char *label)
{
	if(rgb[0] == 255) label[0] = 0;
	else label[0] = rgb[0] + 1;
}

void LLeuvenDataset::DisparityLabelToRgb(unsigned char *label, unsigned char *rgb)
{
	rgb[0] = rgb[1] = rgb[2] = label[0] - 1;
}

void LLeuvenDataset::RgbToLabel(unsigned char *rgb, unsigned char *label)
{
	label[0] = 0;
	for(int i = 0; i < 8; i++) label[0] = (label[0] << 3) | (((rgb[0] >> i) & 1) << 0) | (((rgb[1] >> i) & 1) << 1) | (((rgb[2] >> i) & 1) << 2);
	
	switch(label[0])
	{				
		case 1: 
			label[0] = 1; break;
		case 7: 
			label[0] = 2; break;
		case 12:
			label[0] = 3; break;
		case 20:
			label[0] = 4; break;
		case 21:
			label[0] = 5; break;
		case 36:
			label[0] = 6; break;
		case 38: 
			label[0] = 7; break;
		default: 
			label[0] = 0; break;
	}
}

void LLeuvenDataset::LabelToRgb(unsigned char *label, unsigned char *rgb)
{
	unsigned char lab = label[0];
	switch(lab)
	{
		case 0:
			lab = 0; break;
		case 1:
			lab = 1; break;
		case 2:
			lab = 7; break;
		case 3:
			lab = 12; break;
		case 4:
			lab = 20; break;
		case 5:
			lab = 21; break;
		case 6:
			lab = 36; break;
		case 7:
			lab = 38; break;
		default:
			lab = 0; 
	}
	rgb[0] = rgb[1] = rgb[2] = 0;
	for(int i = 0; lab > 0; i++, lab >>= 3)
	{
		rgb[0] |= (unsigned char) (((lab >> 0) & 1) << (7 - i));
		rgb[1] |= (unsigned char) (((lab >> 1) & 1) << (7 - i));
		rgb[2] |= (unsigned char) (((lab >> 2) & 1) << (7 - i));
	}
}

LCamVidDataset::LCamVidDataset() : LDataset()
{
	seed = 60000;
	featuresOnline = 0;
	unaryWeighted = 0;

	classNo = 11;
	filePermutations = 10000;
	optimizeAverage = 1;

	imageFolder = "CamVid/Images/";
	imageExtension = ".png";
	groundTruthFolder = "CamVid/GroundTruth/";
	groundTruthExtension = ".png";
	trainFolder = "CamVid/Result/Train/";
	testFolder = "CamVid/Result/Crf/";

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	locationBuckets = 12;
	locationFolder = "CamVid/Result/Feature/Location/";
	locationExtension = ".loc";

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 10;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "CamVid/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 20;
	siftNumberOfClusters = 150;
	siftSizes[0] = 3, siftSizes[1] = 5, siftSizes[2] = 7, siftSizes[3] = 9;
	siftSizeCount = 4;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "CamVid/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 20;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 3, colourSiftSizes[1] = 5, colourSiftSizes[2] = 7, colourSiftSizes[3] = 9;
	colourSiftSizeCount = 4;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "CamVid/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "CamVid/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 10;
	lbpNumberOfClusters = 150;

	denseNumRoundsBoosting = 5000;
	denseBoostingSubSample = 5;
	denseNumberOfThetas = 25;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 5;
	denseMaximumRectangleSize = 200;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboost.dat";
	denseExtension = ".dns";
	denseFolder = "CamVid/Result/Dense/";
	denseWeight = 1.0;
	denseMaxClassRatio = 0.1;

	meanShiftXY[0] = 3.0;
	meanShiftLuv[0] = 0.3;
	meanShiftMinRegion[0] = 200;
	meanShiftFolder[0] = "CamVid/Result/MeanShift/30x03/";
	meanShiftXY[1] = 3.0;
	meanShiftLuv[1] = 0.6;
	meanShiftMinRegion[1] = 200;
	meanShiftFolder[1] = "CamVid/Result/MeanShift/30x06/";
	meanShiftXY[2] = 3.0;
	meanShiftLuv[2] = 0.9;
	meanShiftMinRegion[2] = 200;
	meanShiftFolder[2] = "CamVid/Result/MeanShift/30x09/";
	meanShiftExtension = ".msh";

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;
	pairwisePrior = 1.5;
	pairwiseFactor = 6.0;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 5000;
	statsRandomizationFactor = 0.1;
	statsFactor = 0.5;
	statsAlpha = 0.05;
	statsPrior = 0;
	statsMaxClassRatio = 0.5;
	statsTrainFile = "statsboost.dat";
	statsFolder = "CamVid/Result/Stats/";
	statsExtension = ".sts";

	Init();

	int i;
	ForceDirectory(trainFolder);
	ForceDirectory(testFolder, "Train/");
	ForceDirectory(textonFolder, "Train/");
	ForceDirectory(siftFolder, "Train/");
	ForceDirectory(colourSiftFolder, "Train/");
	ForceDirectory(locationFolder, "Train/");
	ForceDirectory(lbpFolder, "Train/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "Train/");
	ForceDirectory(denseFolder, "Train/");
	ForceDirectory(statsFolder, "Train/");
	ForceDirectory(testFolder, "Val/");
	ForceDirectory(textonFolder, "Val/");
	ForceDirectory(siftFolder, "Val/");
	ForceDirectory(colourSiftFolder, "Val/");
	ForceDirectory(locationFolder, "Val/");
	ForceDirectory(lbpFolder, "Val/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "Val/");
	ForceDirectory(denseFolder, "Val/");
	ForceDirectory(statsFolder, "Val/");
	ForceDirectory(testFolder, "Test/");
	ForceDirectory(textonFolder, "Test/");
	ForceDirectory(siftFolder, "Test/");
	ForceDirectory(colourSiftFolder, "Test/");
	ForceDirectory(locationFolder, "Test/");
	ForceDirectory(lbpFolder, "Test/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "Test/");
	ForceDirectory(denseFolder, "Test/");
	ForceDirectory(statsFolder, "Test/");
}

void LCamVidDataset::RgbToLabel(unsigned char *rgb, unsigned char *label)
{
	label[0] = 0;
	for(int i = 0; i < 8; i++) label[0] = (label[0] << 3) | (((rgb[0] >> i) & 1) << 0) | (((rgb[1] >> i) & 1) << 1) | (((rgb[2] >> i) & 1) << 2);
	
	switch(label[0])
	{				
		case 1: 
			label[0] = 1; break;
		case 3: 
			label[0] = 2; break;
		case 7:
			label[0] = 3; break;
		case 12:
			label[0] = 4; break;
		case 13:
			label[0] = 3; break;
		case 15:
			label[0] = 5; break;
		case 21: 
			label[0] = 6; break;
		case 24: 
			label[0] = 7; break;
		case 26: 
			label[0] = 8; break;
		case 27: 
			label[0] = 2; break;
		case 28: 
			label[0] = 8; break;
		case 30: 
			label[0] = 10; break;
		case 31: 
			label[0] = 9; break;
		case 34: 
			label[0] = 3; break;
		case 35: 
			label[0] = 5; break;
		case 36: 
			label[0] = 10; break;
		case 37: 
			label[0] = 6; break;
		case 38: 
			label[0] = 11; break;
		case 39: 
			label[0] = 6; break;
		case 40: 
			label[0] = 3; break;
		case 41: 
			label[0] = 6; break;
		case 45: 
			label[0] = 11; break;
		case 46: 
			label[0] = 4; break;
		case 47: 
			label[0] = 4; break;
		case 48: 
			label[0] = 5; break;
		default: 
			label[0] = 0; break;
	}
}

void LCamVidDataset::LabelToRgb(unsigned char *label, unsigned char *rgb)
{
	unsigned char lab = label[0];
	switch(lab)
	{
		case 1:
			lab = 1; break;
		case 2:
			lab = 3; break;
		case 3:
			lab = 7; break;
		case 4:
			lab = 12; break;
		case 5:
			lab = 15; break;
		case 6:
			lab = 21; break;
		case 7:
			lab = 24; break;
		case 8:
			lab = 28; break;
		case 9:
			lab = 31; break;
		case 10:
			lab = 36; break;
		case 11:
			lab = 38; break;
		default:
			lab = 0; 
	}
	rgb[0] = rgb[1] = rgb[2] = 0;
	for(int i = 0; lab > 0; i++, lab >>= 3)
	{
		rgb[0] |= (unsigned char) (((lab >> 0) & 1) << (7 - i));
		rgb[1] |= (unsigned char) (((lab >> 1) & 1) << (7 - i));
		rgb[2] |= (unsigned char) (((lab >> 2) & 1) << (7 - i));
	}
}

void LCamVidDataset::AddFolder(char *folder, LList<char *> &fileList)
{
        char *fileName, *folderExt;
	
#ifdef _WIN32	
	_finddata_t info;
	int hnd;
	int done;

	folderExt = new char[strlen(imageFolder) + strlen(folder) + strlen(imageExtension) + 2];
	sprintf(folderExt, "%s%s*%s", imageFolder, folder, imageExtension);
	
	hnd = (int)_findfirst(folderExt, &info);
	done = (hnd == -1);

	while(!done)
	{
		info.name[strlen(info.name) - strlen(imageExtension)] = 0;
		fileName = new char[strlen(folder) + strlen(info.name) + 1];
		sprintf(fileName, "%s%s", folder, info.name);
		fileList.Add(fileName);
		allImageFiles.Add(fileName);
		done = _findnext(hnd, &info);
	}
	_findclose(hnd);
#else
	char *wholeFolder;
	struct dirent **nameList = NULL;
	int count;

	folderExt = new char[strlen(imageExtension) + 2];
	sprintf(folderExt, "*%s", imageExtension);

	wholeFolder = new char[strlen(imageFolder) + strlen(folder) + 1];
	sprintf(wholeFolder, "%s%s", imageFolder, folder);
	
	count = scandir(wholeFolder, &nameList, NULL, alphasort);
	if(count >= 0)
	{
	      for(int i = 0; i < count; i++)
	      {
		      if(!fnmatch(folderExt, nameList[i]->d_name, 0))
		      {
			      nameList[i]->d_name[strlen(nameList[i]->d_name) - strlen(imageExtension)] = 0;
			      fileName = new char[strlen(folder) + strlen(nameList[i]->d_name) + 1];
			      sprintf(fileName, "%s%s", folder, nameList[i]->d_name);
			      fileList.Add(fileName);
			      allImageFiles.Add(fileName);
		      }
		      if(nameList[i] != NULL) free(nameList[i]);
	      }
	      if(nameList != NULL) free(nameList);
	}
	delete[] wholeFolder;
#endif
	delete[] folderExt;
}

void LCamVidDataset::Init()
{
	AddFolder("Train/", trainImageFiles);
	AddFolder("Val/", trainImageFiles);
	AddFolder("Test/", testImageFiles);
}

void LCamVidDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[3];
	LSegmentation2D *segmentation[3];

	int i;
	for(i = 0; i < 3; i++)
	{
		segmentation[i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}

	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);
    pixelPotential->AddFeature(textonFeature);
    pixelPotential->AddFeature(siftFeature);
    pixelPotential->AddFeature(coloursiftFeature);
    pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

	crf->potentials.Add(new LEightNeighbourPottsPairwisePixelPotential(this, crf, objDomain, baseLayer, classNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight));

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 3; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;
	crf->potentials.Add(statsPotential);
}

void readColorMap(char *label_fn, int classNo, int ***ColorMap) {
	*ColorMap = new int*[classNo+1];
	for(int i = 0; i < classNo+1; i++)
		(*ColorMap)[i] = new int[3];

	std::ifstream file(label_fn);
	std::string str;
	int l, r, g, b;

	while(file >> l >> r >> g >> b >> str) {
		(*ColorMap)[l+1][0] = r;
		(*ColorMap)[l+1][1] = g;
		(*ColorMap)[l+1][2] = b;
	}
	
	file.close();
}


LKittiDataset::LKittiDataset() : LDataset()
{
	seed = 60000;
	featuresOnline = 0;
	unaryWeighted = 0;

	//classNo = 12;
	classNo = 2; // car and background
	filePermutations = 10000;
	optimizeAverage = 1;

	imageFolder = "Kitti/Images/";
	imageExtension = ".png";
	groundTruthFolder = "Kitti/GroundTruth/";
	groundTruthExtension = ".png";
	trainFolder = "Kitti/Result/Train/";
	testFolder = "Kitti/Result/Crf/";

	//char *label_fn = "Kitti/GroundTruth/labels.txt";
	//readColorMap(label_fn, classNo, &ColorMap);

	clusterPointsPerKDTreeCluster = 30;
	clusterKMeansMaxChange = 0.01;

	locationBuckets = 12;
	locationFolder = "Kitti/Result/Feature/Location/";
	locationExtension = ".loc";

	textonNumberOfClusters = 150;
	textonFilterBankRescale = 0.7;
	textonKMeansSubSample = 5;
	textonClusteringTrainFile = "textonclustering.dat";
	textonFolder = "Kitti/Result/Feature/Texton/";
	textonExtension = ".txn";

	siftClusteringTrainFile = "siftclustering.dat";
	siftKMeansSubSample = 10;
	siftNumberOfClusters = 150;
	siftSizes[0] = 3, siftSizes[1] = 5, siftSizes[2] = 7, siftSizes[3] = 9;
	siftSizeCount = 4;
	siftWindowNumber = 3;
	sift360 = 1;
	siftAngles = 8;
	siftFolder = "Kitti/Result/Feature/Sift/";
	siftExtension = ".sft";

	colourSiftClusteringTrainFile = "coloursiftclustering.dat";
	colourSiftKMeansSubSample = 10;
	colourSiftNumberOfClusters = 150;
	colourSiftSizes[0] = 3, colourSiftSizes[1] = 5, colourSiftSizes[2] = 7, colourSiftSizes[3] = 9;
	colourSiftSizeCount = 4;
	colourSiftWindowNumber = 3;
	colourSift360 = 1;
	colourSiftAngles = 8;
	colourSiftFolder = "Kitti/Result/Feature/ColourSift/";
	colourSiftExtension = ".csf";

	lbpClusteringFile = "lbpclustering.dat";
	lbpFolder = "Kitti/Result/Feature/Lbp/";
	lbpExtension = ".lbp";
	lbpSize = 11;
	lbpKMeansSubSample = 10;
	lbpNumberOfClusters = 150;

	denseNumRoundsBoosting = 5000;
	denseBoostingSubSample = 5;
	denseNumberOfThetas = 25;
	denseThetaStart = 3;
	denseThetaIncrement = 2;
	denseNumberOfRectangles = 100;
	denseMinimumRectangleSize = 5;
	denseMaximumRectangleSize = 200;
	denseRandomizationFactor = 0.003;
	denseBoostTrainFile = "denseboostXX.dat";
	denseExtension = ".dns";
	denseFolder = "Kitti/Result/DenseXX/";
	denseWeight = 1.0;
	denseMaxClassRatio = 0.1;

	meanShiftXY[0] = 3.0;
	meanShiftLuv[0] = 0.3;
	meanShiftMinRegion[0] = 200;
	meanShiftFolder[0] = "Kitti/Result/MeanShift/30x03/";
	meanShiftXY[1] = 3.0;
	meanShiftLuv[1] = 0.6;
	meanShiftMinRegion[1] = 200;
	meanShiftFolder[1] = "Kitti/Result/MeanShift/30x06/";
	meanShiftXY[2] = 3.0;
	meanShiftLuv[2] = 0.9;
	meanShiftMinRegion[2] = 200;
	meanShiftFolder[2] = "Kitti/Result/MeanShift/30x09/";
	meanShiftExtension = ".msh";

	pairwiseLWeight = 1.0 / 3.0;
	pairwiseUWeight = 1.0 / 3.0;
	pairwiseVWeight = 1.0 / 3.0;
	pairwisePrior = 1.6;
	pairwiseFactor = 6.4;
	pairwiseBeta = 16.0;

	cliqueMinLabelRatio = 0.5;
	cliqueThresholdRatio = 0.1;
	cliqueTruncation = 0.1;

	statsThetaStart = 2;
	statsThetaIncrement = 1;
	statsNumberOfThetas = 15;
	statsNumberOfBoosts = 5000;
	statsRandomizationFactor = 0.1;
	statsFactor = 0.5;
	statsAlpha = 0.05;
	statsPrior = 0;
	statsMaxClassRatio = 0.5;
	statsTrainFile = "statsboost.dat";
	statsFolder = "Kitti/Result/Stats/";
	statsExtension = ".sts";

	Init();

	int i;
	ForceDirectory(trainFolder);
	ForceDirectory(testFolder, "Train/");
	ForceDirectory(textonFolder, "Train/");
	ForceDirectory(siftFolder, "Train/");
	ForceDirectory(colourSiftFolder, "Train/");
	ForceDirectory(locationFolder, "Train/");
	ForceDirectory(lbpFolder, "Train/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "Train/");
	ForceDirectory(denseFolder, "Train/");
	ForceDirectory(statsFolder, "Train/");
	ForceDirectory(testFolder, "Test/");
	ForceDirectory(textonFolder, "Test/");
	ForceDirectory(siftFolder, "Test/");
	ForceDirectory(colourSiftFolder, "Test/");
	ForceDirectory(locationFolder, "Test/");
	ForceDirectory(lbpFolder, "Test/");
	for(i = 0; i < 3; i++) ForceDirectory(meanShiftFolder[i], "Test/");
	ForceDirectory(denseFolder, "Test/");
	ForceDirectory(statsFolder, "Test/");
}

LKittiDataset::~LKittiDataset()
{
	//for(int i = 0; i < classNo+1; i++) delete[] ColorMap[i];
	//delete[] ColorMap;
}

void LKittiDataset::RgbToLabel(unsigned char *rgb, unsigned char *label)
{
	label[0] = 0;

	if((rgb[0] == 64) && (rgb[1] == 0) && (rgb[2] == 128))
		label[0] = 2;
	else
		label[0] = 1;

	//std::cout << (int)rgb[0] << " " << (int)rgb[1] << " " << (int)rgb[2] << ": " << (int)label[0] << std::endl;
}

void LKittiDataset::LabelToRgb(unsigned char *label, unsigned char *rgb)
{
	unsigned char lab = label[0];
	switch(lab)
	{
		case 1:
			rgb[0] = 0; rgb[1] = 0; rgb[2] = 0; break;
		case 2:
			rgb[0] = 64; rgb[1] = 0; rgb[2] = 128; break;
		default:
			rgb[0] = 0; rgb[1] = 0; rgb[2] = 0;  
	}

	//std::cout << (int)label[0] << ": " << (int)rgb[0] << " " << (int)rgb[1] << " " << (int)rgb[2] << std::endl;
}

void LKittiDataset::AddFolder(char *folder, LList<char *> &fileList)
{
        char *fileName, *folderExt;
	
#ifdef _WIN32	
	_finddata_t info;
	int hnd;
	int done;

	folderExt = new char[strlen(imageFolder) + strlen(folder) + strlen(imageExtension) + 2];
	sprintf(folderExt, "%s%s*%s", imageFolder, folder, imageExtension);
	
	hnd = (int)_findfirst(folderExt, &info);
	done = (hnd == -1);

	while(!done)
	{
		info.name[strlen(info.name) - strlen(imageExtension)] = 0;
		fileName = new char[strlen(folder) + strlen(info.name) + 1];
		sprintf(fileName, "%s%s", folder, info.name);
		fileList.Add(fileName);
		allImageFiles.Add(fileName);
		done = _findnext(hnd, &info);
	}
	_findclose(hnd);
#else
	char *wholeFolder;
	struct dirent **nameList = NULL;
	int count;

	folderExt = new char[strlen(imageExtension) + 2];
	sprintf(folderExt, "*%s", imageExtension);

	wholeFolder = new char[strlen(imageFolder) + strlen(folder) + 1];
	sprintf(wholeFolder, "%s%s", imageFolder, folder);
	
	count = scandir(wholeFolder, &nameList, NULL, alphasort);
	if(count >= 0)
	{
	      for(int i = 0; i < count; i++)
	      {
		      if(!fnmatch(folderExt, nameList[i]->d_name, 0))
		      {
			      nameList[i]->d_name[strlen(nameList[i]->d_name) - strlen(imageExtension)] = 0;
			      fileName = new char[strlen(folder) + strlen(nameList[i]->d_name) + 1];
			      sprintf(fileName, "%s%s", folder, nameList[i]->d_name);
			      fileList.Add(fileName);
			      allImageFiles.Add(fileName);
		      }
		      if(nameList[i] != NULL) free(nameList[i]);
	      }
	      if(nameList != NULL) free(nameList);
	}
	delete[] wholeFolder;
#endif
	delete[] folderExt;
}

void LKittiDataset::Init()
{
	AddFolder("Train/", trainImageFiles);
	AddFolder("Test/", testImageFiles);
}


void LKittiDataset::SetCRFStructure(LCrf *crf)
{
	LCrfDomain *objDomain = new LCrfDomain(crf, this, classNo, testFolder, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::RgbToLabel, (void (LDataset::*)(unsigned char *,unsigned char *))&LDataset::LabelToRgb);
	crf->domains.Add(objDomain);

	LBaseCrfLayer *baseLayer = new LBaseCrfLayer(crf, objDomain, this, 0);
	crf->layers.Add(baseLayer);

	LPnCrfLayer *superpixelLayer[3];
	LSegmentation2D *segmentation[3];

	int i;
	for(i = 0; i < 3; i++)
	{
		segmentation[i] = new LMeanShiftSegmentation2D(meanShiftXY[i], meanShiftLuv[i], meanShiftMinRegion[i], meanShiftFolder[i], meanShiftExtension);
		superpixelLayer[i] = new LPnCrfLayer(crf, objDomain, this, baseLayer, segmentation[i], cliqueTruncation);
		crf->segmentations.Add(segmentation[i]);
		crf->layers.Add(superpixelLayer[i]);
	}

	LTextonFeature *textonFeature = new LTextonFeature(this, trainFolder, textonClusteringTrainFile, textonFolder, textonExtension, textonFilterBankRescale, textonKMeansSubSample, textonNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);
	LLocationFeature *locationFeature = new LLocationFeature(this, locationFolder, locationExtension, locationBuckets);
	LSiftFeature *siftFeature = new LSiftFeature(this, trainFolder, siftClusteringTrainFile, siftFolder, siftExtension, siftSizeCount, siftSizes, siftWindowNumber, sift360, siftAngles, siftKMeansSubSample, siftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LColourSiftFeature *coloursiftFeature = new LColourSiftFeature(this, trainFolder, colourSiftClusteringTrainFile, colourSiftFolder, colourSiftExtension, colourSiftSizeCount, colourSiftSizes, colourSiftWindowNumber, colourSift360, colourSiftAngles, colourSiftKMeansSubSample, colourSiftNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster, 1);
	LLbpFeature *lbpFeature = new LLbpFeature(this, trainFolder, lbpClusteringFile, lbpFolder, lbpExtension, lbpSize, lbpKMeansSubSample, lbpNumberOfClusters, clusterKMeansMaxChange, clusterPointsPerKDTreeCluster);

	crf->features.Add(textonFeature);
	crf->features.Add(locationFeature);
	crf->features.Add(lbpFeature);
	crf->features.Add(siftFeature);
	crf->features.Add(coloursiftFeature);

	LDenseUnaryPixelPotential *pixelPotential = new LDenseUnaryPixelPotential(this, crf, objDomain, baseLayer, trainFolder, denseBoostTrainFile, denseFolder, denseExtension, classNo, denseWeight, denseBoostingSubSample, denseNumberOfRectangles, denseMinimumRectangleSize, denseMaximumRectangleSize, denseMaxClassRatio);
    	pixelPotential->AddFeature(textonFeature);
    	pixelPotential->AddFeature(siftFeature);
    	pixelPotential->AddFeature(coloursiftFeature);
    	pixelPotential->AddFeature(lbpFeature);
	crf->potentials.Add(pixelPotential);

	LBoosting<int> *pixelBoosting = new LBoosting<int>(trainFolder, denseBoostTrainFile, classNo, denseNumRoundsBoosting, denseThetaStart, denseThetaIncrement, denseNumberOfThetas, denseRandomizationFactor, pixelPotential, (int *(LPotential::*)(int, int))&LDenseUnaryPixelPotential::GetTrainBoostingValues, (int *(LPotential::*)(int))&LDenseUnaryPixelPotential::GetEvalBoostingValues);
	crf->learnings.Add(pixelBoosting);
	pixelPotential->learning = pixelBoosting;

	crf->potentials.Add(new LEightNeighbourPottsPairwisePixelPotential(this, crf, objDomain, baseLayer, classNo, pairwisePrior, pairwiseFactor, pairwiseBeta, pairwiseLWeight, pairwiseUWeight, pairwiseVWeight));

	LStatsUnarySegmentPotential *statsPotential = new LStatsUnarySegmentPotential(this, crf, objDomain, trainFolder, statsTrainFile, statsFolder, statsExtension, classNo, statsPrior, statsFactor, cliqueMinLabelRatio, statsAlpha, statsMaxClassRatio);
	statsPotential->AddFeature(textonFeature);
	statsPotential->AddFeature(siftFeature);
	statsPotential->AddFeature(coloursiftFeature);
	statsPotential->AddFeature(locationFeature);
	statsPotential->AddFeature(lbpFeature);
	for(i = 0; i < 3; i++) statsPotential->AddLayer(superpixelLayer[i]);

	LBoosting<double> *segmentBoosting = new LBoosting<double>(trainFolder, statsTrainFile, classNo, statsNumberOfBoosts, statsThetaStart, statsThetaIncrement, statsNumberOfThetas, statsRandomizationFactor, statsPotential, (double *(LPotential::*)(int, int))&LStatsUnarySegmentPotential::GetTrainBoostingValues, (double *(LPotential::*)(int))&LStatsUnarySegmentPotential::GetEvalBoostingValues);
	crf->learnings.Add(segmentBoosting);
	statsPotential->learning = segmentBoosting;
	crf->potentials.Add(statsPotential);
}

