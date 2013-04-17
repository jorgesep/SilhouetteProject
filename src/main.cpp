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

#include "main.h"
#include "mdgkt_filter.h"
#include "background_subtraction.h"

#include <iostream>
#include <fstream>



using namespace cv;
using namespace std;



int main( int argc, char** argv )
{

    mdgkt* preProc = mdgkt::Instance();
    
    string videoName= "/Users/jsepulve/Downloads/Last_Downloads/Matlab/dvcam/testxvid.avi";
    
    
    namedWindow("image", CV_WINDOW_NORMAL);
    namedWindow("foreground mask", CV_WINDOW_NORMAL);
    namedWindow("foreground image", CV_WINDOW_NORMAL);
    namedWindow("mean background image", CV_WINDOW_NORMAL);
    
    
    VideoCapture video(videoName);
    
    // Check video has been opened sucessfully
    if (!video.isOpened())
        return 1;
    
    BackgroundSubtractorMOG3 bg_model;
    Mat img, fgmask, fgimg;
    bool update_bg_model = true;

    
    Mat frame;
    
    video >> frame;
    preProc->initializeFirstImage(frame);
    preProc->SpatioTemporalPreprocessing(frame, img);
    video >> frame;
    preProc->SpatioTemporalPreprocessing(frame, img);
    video >> frame;
    preProc->SpatioTemporalPreprocessing(frame, img);

    
    double rate= video.get(CV_CAP_PROP_FPS);
    int delay= 1000/rate;
    
    
    
    for(;;)
    {
        video >> img;
        //video >> frame;
        
        //preProc->SpatioTemporalPreprocessing(frame, img);

        
        if( img.empty() )
            break;
        
        if( fgimg.empty() )
            fgimg.create(img.size(), img.type());
       
        
        bg_model(img, fgmask, update_bg_model ? -1 : 0);
        
        fgimg = Scalar::all(0);
        
        img.copyTo(fgimg, fgmask);
        
        Mat bgimg;
        bg_model.getBackgroundImage(bgimg);
        
        imshow("image", img);
        imshow("foreground mask", fgmask);
        imshow("foreground image", fgimg);
        if(!bgimg.empty())
            imshow("mean background image", bgimg );

        
        //imshow("IMAGE", img);
        /*
        //update the model
        
        char k = (char)waitKey(30);
        if( k == 27 ) break;
        if( k == ' ' )
        {
            update_bg_model = !update_bg_model;
            if(update_bg_model)
                printf("Background update is on\n");
            else
                printf("Background update is off\n");
        }
         */

        if (cv::waitKey(delay)>=0)
            break;
    }

    /*
    Mat frame;
    Mat img;
    
    video >> frame;
    preProc->initializeFirstImage(frame);
    preProc->SpatioTemporalPreprocessing(frame, img);
    video >> frame;
    preProc->SpatioTemporalPreprocessing(frame, img);
    video >> frame;
    preProc->SpatioTemporalPreprocessing(frame, img);
    
    
    ofstream myfile;
    myfile.open ("example.txt");
    
    vector<Mat> channels;
    split(img,channels);
    myfile << "KERNEL WEIGHT(0): " << endl << " " << channels.at(0) << endl << endl;
    myfile << "KERNEL WEIGHT(1): " << endl << " " << channels.at(1) << endl << endl;
    myfile << "KERNEL WEIGHT(2): " << endl << " " << channels.at(2) << endl << endl;
    myfile.close();
    */
    return 0;
}

