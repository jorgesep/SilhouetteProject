/*******************************************************************************
 * <Self-Adaptive Gaussian Mixture Model.>
 * Copyright (C) <2013>  <name of author>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>

#include <iomanip> 

#include "sagmm.h"
#include "mdgkt_filter.h"

#include <iostream>
#include <fstream>



using namespace cv;
using namespace std;



int main( int argc, char** argv )
{

    mdgkt* preProc = mdgkt::Instance();
    
    string fileName = "/Users/jsepulve/Downloads/template.jpg";
    
    Mat currentImage= imread(fileName);

    cout << "CURRENT IMAGE DEPTH: " << currentImage.depth() << endl;
    
    Mat img;
    preProc->initializeFirstImage(currentImage);
    preProc->SpatioTemporalPreprocessing(currentImage, img);
    preProc->SpatioTemporalPreprocessing(currentImage, img);
    preProc->SpatioTemporalPreprocessing(currentImage, img);
    
    
    ofstream myfile;
    myfile.open ("example.txt");
    
    vector<Mat> channels;
    split(img,channels);
    myfile << "KERNEL WEIGHT(0): " << endl << " " << channels.at(0) << endl << endl;
    myfile << "KERNEL WEIGHT(1): " << endl << " " << channels.at(1) << endl << endl;
    myfile << "KERNEL WEIGHT(2): " << endl << " " << channels.at(2) << endl << endl;
    myfile.close();
    
    return 0;
}

