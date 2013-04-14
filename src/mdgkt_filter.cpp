//
//  mdgkt.cpp
//  sagmm
//
//  Created by Jorge Sepulveda on 4/13/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "mdgkt_filter.h"

const float mdgkt::SIGMA = 0.5;
const int mdgkt::SPATIO_WINDOW = 3;
const int mdgkt::TIME_WINDOW = 3;

mdgkt* mdgkt::ptrInstance = NULL;
int mdgkt::numInstances = 0;


// Internal method to initialize to zero all vectors
void mdgkt::initialize() 
{
    // Matrix with gaussian values for processing temporal frames.
    temporalGaussFilter = getGaussianKernel(TIME_WINDOW,SIGMA,CV_32F);
}


void mdgkt::initializeFirstImage(const Mat& img)
{
    this->initialize();
    //Initialize vectors
    for (int i=0; i<SPATIO_WINDOW; i++) {
        kernelImageR.push_back(Mat::zeros(img.size(), CV_32FC1));
        kernelImageG.push_back(Mat::zeros(img.size(), CV_32FC1));
        kernelImageB.push_back(Mat::zeros(img.size(), CV_32FC1));
    }
    
    has_been_initialized = true;

}


void mdgkt::initializeFirstImage(const vector<Mat> & sequences) 
{
    if (sequences.size() > 0) {
        this->initializeFirstImage(sequences.at(0));
    }
    
}

void mdgkt::SpatioTemporalPreprocessing(const Mat& src, Mat& dst)
{
    vector<Mat> nchannels;
    vector<Mat> brg;
    vector<Mat> temporal_average;

    for (int i=0; i<TIME_WINDOW; i++) {
        temporal_average.push_back(Mat::zeros(src.size(), CV_32FC1));        
    }

    
    Mat E;
    src.convertTo(E, CV_32FC3);
    split(E,nchannels);
    
    
    brg.push_back( Mat(src.size(), CV_32FC1));
    brg.push_back( Mat(src.size(), CV_32FC1));
    brg.push_back( Mat(src.size(), CV_32FC1));
    
    //Spatial pre-processing
    GaussianBlur(nchannels.at(2), brg.at(2), Size(3,3), 0.5);
    GaussianBlur(nchannels.at(1), brg.at(1), Size(3,3), 0.5);
    GaussianBlur(nchannels.at(0), brg.at(0), Size(3,3), 0.5);

    
    kernelImageR.push_back(brg.at(2));
    kernelImageG.push_back(brg.at(1));
    kernelImageB.push_back(brg.at(0));
    
    if (kernelImageB.size() > SPATIO_WINDOW )
        kernelImageB.erase(kernelImageB.begin());
    if (kernelImageR.size() > SPATIO_WINDOW )
        kernelImageR.erase(kernelImageR.begin());
    if (kernelImageG.size() > SPATIO_WINDOW )
        kernelImageG.erase(kernelImageG.begin());
    
    //Temporal pre-processing
    const float* fptr=temporalGaussFilter.ptr<float>(0);

    
    for (int i=0; i<TIME_WINDOW; i++) {
        temporal_average.at(0)+= kernelImageR.at(i)*fptr[i];
        temporal_average.at(1)+= kernelImageG.at(i)*fptr[i];
        temporal_average.at(2)+= kernelImageB.at(i)*fptr[i];
    }
  
    merge(temporal_average,dst);
    

}

void mdgkt::deleteInstance () {
    
    if (ptrInstance) {
        delete ptrInstance;
        ptrInstance = NULL;
    }
    
}

mdgkt* mdgkt::Instance() {
    
    if (!ptrInstance) {
        ptrInstance = new mdgkt();
    }
    else
        cout << "INSTANCE ALREADY CREATED" << endl;
    
    //numInstances++;
    return ptrInstance;
}


