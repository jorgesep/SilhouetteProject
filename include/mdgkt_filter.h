//
//  mdgkt_filter.h
//  mdgkt_filter
//
//  Created by Jorge Sepulveda on 4/10/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef _mdgkt_filter_h
#define _mdgkt_filter_h
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;

/**
 * Implementation of spatio-temporal pre-processing filter for
 * smoothing transform.
 */
class mdgkt
{
public:
    static mdgkt* Instance();
    static void deleteInstance();
    void SpatioTemporalPreprocessing(const Mat&, Mat&);
    void initialize();
    void initializeFirstImage(const Mat&);
    void initializeFirstImage(const vector<Mat>&);

private:
    
    mdgkt() { };
    
    virtual ~mdgkt() { };
    mdgkt(const mdgkt &) { };
    mdgkt& operator=(mdgkt const&){ return *this; };
    
    vector<Mat> kernelImageR;
    vector<Mat> kernelImageG;
    vector<Mat> kernelImageB;

    Mat temporalGaussFilter;
    

    static const int SPATIO_WINDOW;
    static const int TIME_WINDOW;
    static const float SIGMA;
    
    static mdgkt* ptrInstance;
    static int numInstances;

    bool has_been_initialized;

};




















#endif
