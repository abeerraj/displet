#include "main.h"

#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
 
	ilInit();

	int from = 0, to = -1;
	if(argc == 3) from = atoi(argv[1]), to = atoi(argv[2]);
	if(argc == 4) from = atoi(argv[2]), to = atoi(argv[3]);

//	LDataset *dataset = new LMsrcDataset();
//	LDataset *dataset = new LVOCDataset();
//	LDataset *dataset = new LCamVidDataset();
//	LDataset *dataset = new LCorelDataset();
//	LDataset *dataset = new LSowerbyDataset();
//	LDataset *dataset = new LLeuvenDataset();

	LDataset *dataset = new LKittiDataset();

	LCrf *crf = new LCrf(dataset);
	dataset->SetCRFStructure(crf);

//	crf->Segment(dataset->trainImageFiles, from, to);
//	crf->TrainFeatures(dataset->trainImageFiles);
//	crf->EvaluateFeatures(dataset->trainImageFiles, from, to);
//	crf->TrainPotentials(dataset->trainImageFiles);

//	crf->EvaluatePotentials(dataset->trainImageFiles, from, to);
//	crf->Solve(dataset->trainImageFiles, from, to);
//	crf->Confusion(dataset->trainImageFiles, "resultTrain.txt");

	crf->Segment(dataset->testImageFiles, from, to);
	crf->EvaluateFeatures(dataset->testImageFiles, from, to);
	crf->EvaluatePotentials(dataset->testImageFiles, from, to);
	crf->Solve(dataset->testImageFiles, from, to);
//	crf->Confusion(dataset->testImageFiles, "resultTest.txt");

	delete crf;
	delete dataset;

	return(0);

}


