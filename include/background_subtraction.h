#ifndef _BACKGROUND_SUBTRACTION_H_
#define _BACKGROUND_SUBTRACTION_H_

#include "opencv2/video/background_segm.hpp"
#include "opencv2/core/core.hpp"
#include <list>


using namespace cv;

/*!
 The class implements the algorithm:
 "Self-adaptive Gaussian Mixture Model for urban traffic monitoring system"
 Seshi Zhen
*/
class CV_EXPORTS BackgroundSubtractorMOG3 : public BackgroundSubtractor
{
public:
    //! the default constructor
    BackgroundSubtractorMOG3();
    //! the full constructor that takes the length of the history, the number of gaussian mixtures, the background ratio parameter and the noise strength
    BackgroundSubtractorMOG3(int history,  float varThreshold, bool bShadowDetection=true);
    //! the destructor
    virtual ~BackgroundSubtractorMOG3();
    //! the update operator
    virtual void operator()(InputArray image, OutputArray fgmask, double learningRate=-1);

    //! computes a background image which are the mean of all background gaussians
    virtual void getBackgroundImage(OutputArray backgroundImage) const;

    //! re-initiaization method
    virtual void initialize(Size frameSize, int frameType);

    //virtual AlgorithmInfo* info() const;

protected:
    Size frameSize;
    int frameType;
    Mat bgmodel;
    Mat bgmodelUsedModes;//keep track of number of modes per pixel
    int nframes;
    int history;
    int nmixtures;
    //! here it is the maximum allowed number of mixture components.
    //! Actual number is determined dynamically per pixel
    double varThreshold;
    // threshold on the squared Mahalanobis distance to decide if it is well described
    // by the background model or not. Related to Cthr from the paper.
    // This does not influence the update of the background. A typical value could be 4 sigma
    // and that is varThreshold=4*4=16; Corresponds to Tb in the paper.

    /////////////////////////
    // less important parameters - things you might change but be carefull
    ////////////////////////
    float backgroundRatio;
    // corresponds to fTB=1-cf from the paper
    // TB - threshold when the component becomes significant enough to be included into
    // the background model. It is the TB=1-cf from the paper. So I use cf=0.1 => TB=0.
    // For alpha=0.001 it means that the mode should exist for approximately 105 frames before
    // it is considered foreground
    // float noiseSigma;
    float varThresholdGen;
    //correspondts to Tg - threshold on the squared Mahalan. dist. to decide
    //when a sample is close to the existing components. If it is not close
    //to any a new component will be generated. I use 3 sigma => Tg=3*3=9.
    //Smaller Tg leads to more generated components and higher Tg might make
    //lead to small number of components but they can grow too large
    float fVarInit;
    float fVarMin;
    float fVarMax;
    //initial variance  for the newly generated components.
    //It will will influence the speed of adaptation. A good guess should be made.
    //A simple way is to estimate the typical standard deviation from the images.
    //I used here 10 as a reasonable value
    // min and max can be used to further control the variance
    float fCT;//CT - complexity reduction prior
    //this is related to the number of samples needed to accept that a component
    //actually exists. We use CT=0.05 of all the samples. By setting CT=0 you get
    //the standard Stauffer&Grimson algorithm (maybe not exact but very similar)

    //shadow detection parameters
    bool bShadowDetection;//default 1 - do shadow detection
    unsigned char nShadowDetection;//do shadow detection - insert this value as the detection result - 127 default value
    float fTau;
    // Tau - shadow threshold. The shadow is detected if the pixel is darker
    //version of the background. Tau is a threshold on how much darker the shadow can be.
    //Tau= 0.5 means that if pixel is more than 2 times darker then it is not shadow
    //See: Prati,Mikic,Trivedi,Cucchiarra,"Detecting Moving Shadows...",IEEE PAMI,2003.
    
    
    // Max. number of Gaussian per pixel
    static const int GaussiansNo;

    // Initial sigma.
    static const float Sigma;
    static const float SigmaMax;
    static const float SigmaMin;
    
    // log(1-cf)/log(1-Alfa) = 100 frmaes
    //speed of update, the time interval =1/Alfa.
    static const float Alpha;
    
    // A measure of the minimum portion of the data that be accounted for by the background.
    // If a bigger value of cf is chosen, the background model is uaually
    // unimodal.
    static const float Cf;
    static const float T;
    
    // A threshold to decide if a pixel value match one of a existent Gaussian
    // distribution according to the distance.
    static const float PixelRange;
    
    // A threshold to decide if a pixel value match one of a existent Gaussian
    // distribution according to the distance. If it is not close to any a new
    // component will be generate a new one.
    // Normally, Gen < Range
    static const float PixelGen;
    
    // complexity reduction prior. It is related to the number of samples needed
    // to accept a component actually exist. We use cT = 0.05 of all the saples.
    // By setting cT = 0 you get the standard Stauffer and Grimson algorithm.
    // Eq (13)
    static const float CT;
    
    // the threshold for shadow detection.
    static const float Tau;
    
    Mat GaussianModel;
    Mat CurrentGaussianModel;
    Mat BackgroundNumberCounter;
    Mat Background;
    Mat Foreground;

};

#endif
