/*//Implementation of the Gaussian mixture model background subtraction from:
//
//"Self-Adaptive Gaussian Mixture Model for urban traffic monitoring"
// Sezhi Shen
//Example usage with as cpp class
// BackgroundSubtractorMOG3 bg_model;
//For each new image the model is updates using:
// bg_model(img, fgmask);
//
//Example usage as part of the CvBGStatModel:
// CvBGStatModel* bg_model = cvCreateGaussianBGModel2( first_frame );
//
// //update for each frame
// cvUpdateBGStatModel( tmp_frame, bg_model );//segmentation result is in bg_model->foreground
//
// //release at the program termination
// cvReleaseBGStatModel( &bg_model );
//
//////////
//*/

#include "background_subtraction.h"

#include <opencv2/opencv.hpp>


#include "precomp.h"

using namespace std;
using namespace cv;

// default parameters of gaussian background detection algorithm
static const int   defaultHistory2         = 500; // Learning rate; alpha = 1/defaultHistory2
static const float defaultVarThreshold2    = 4.0f*4.0f;
static const int   defaultNMixtures2       = 5; // maximal number of Gaussians in mixture
static const float defaultBackgroundRatio2 = 0.9f; // threshold sum of weights for background test
static const float defaultVarThresholdGen2 = 3.0f*3.0f;
static const float defaultVarInit2         = 15.0f; // initial variance for new components
static const float defaultVarMax2          = 5*defaultVarInit2;
static const float defaultVarMin2          = 4.0f;

// additional parameters
static const float defaultfCT2             = 0.05f; // complexity reduction prior constant 0 - no reduction of number of components
static const float defaultfTau             = 0.5f; // Tau - shadow threshold, see the paper for explanation
static const unsigned char defaultnShadowDetection2 = (unsigned char)127; // value to use in the segmentation mask for shadows, set 0 not to do shadow detection


//const float BackgroundSubtractorMOG3::Alpha        = 0.001f; //speed of update, the time interval =1/Alfa.
//const float BackgroundSubtractorMOG3::Cf          = 0.05f;
const float BackgroundSubtractorMOG3::Alpha       = 0.00001f; //speed of update, the time interval =1/Alfa.
const float BackgroundSubtractorMOG3::Cf          = 0.001f;
const int   BackgroundSubtractorMOG3::GaussiansNo = 4;       // Max. number of Gaussian per pixel
const float BackgroundSubtractorMOG3::Sigma       = 11.0f;    // Initial sigma.
const float BackgroundSubtractorMOG3::SigmaMax    = 5.0f*BackgroundSubtractorMOG3::Sigma;    // Initial sigma.
const float BackgroundSubtractorMOG3::SigmaMin    = 4.0f;    // Initial sigma.
const float BackgroundSubtractorMOG3::T           = 1.0f - BackgroundSubtractorMOG3::Cf;
const float BackgroundSubtractorMOG3::PixelRange  = 4.0f*4.0f; // A threshold to decide if a pixel value match one of a existent Gaussian
const float BackgroundSubtractorMOG3::PixelGen    = 3.0f*3.0f; // Normally, Gen < Range
const float BackgroundSubtractorMOG3::CT          = 0.05f;
const float BackgroundSubtractorMOG3::Tau         = 0.5f ;




struct GaussBGStatModel2Params
{
    //image info
    int nWidth;
    int nHeight;
    int nND;//number of data dimensions (image channels)

    bool bPostFiltering;//defult 1 - do postfiltering - will make shadow detection results also give value 255
    double  minArea; // for postfiltering

    bool bInit;//default 1, faster updates at start

    /////////////////////////
    //very important parameters - things you will change
    ////////////////////////
    float fAlphaT;
    //alpha - speed of update - if the time interval you want to average over is T
    //set alpha=1/T. It is also usefull at start to make T slowly increase
    //from 1 until the desired T
    float fTb;
    //Tb - threshold on the squared Mahalan. dist. to decide if it is well described
    //by the background model or not. Related to Cthr from the paper.
    //This does not influence the update of the background. A typical value could be 4 sigma
    //and that is Tb=4*4=16;

    /////////////////////////
    //less important parameters - things you might change but be carefull
    ////////////////////////
    float fTg;
    //Tg - threshold on the squared Mahalan. dist. to decide
    //when a sample is close to the existing components. If it is not close
    //to any a new component will be generated. I use 3 sigma => Tg=3*3=9.
    //Smaller Tg leads to more generated components and higher Tg might make
    //lead to small number of components but they can grow too large
    float fTB;//1-cf from the paper
    //TB - threshold when the component becomes significant enough to be included into
    //the background model. It is the TB=1-cf from the paper. So I use cf=0.1 => TB=0.
    //For alpha=0.001 it means that the mode should exist for approximately 105 frames before
    //it is considered foreground
    float fVarInit;
    float fVarMax;
    float fVarMin;
    //initial standard deviation  for the newly generated components.
    //It will will influence the speed of adaptation. A good guess should be made.
    //A simple way is to estimate the typical standard deviation from the images.
    //I used here 10 as a reasonable value
    float fCT;//CT - complexity reduction prior
    //this is related to the number of samples needed to accept that a component
    //actually exists. We use CT=0.05 of all the samples. By setting CT=0 you get
    //the standard Stauffer&Grimson algorithm (maybe not exact but very similar)

    //even less important parameters
    int nM;//max number of modes - const - 4 is usually enough

    //shadow detection parameters
    bool bShadowDetection;//default 1 - do shadow detection
    unsigned char nShadowDetection;//do shadow detection - insert this value as the detection result
    float fTau;
    // Tau - shadow threshold. The shadow is detected if the pixel is darker
    //version of the background. Tau is a threshold on how much darker the shadow can be.
    //Tau= 0.5 means that if pixel is more than 2 times darker then it is not shadow
    //See: Prati,Mikic,Trivedi,Cucchiarra,"Detecting Moving Shadows...",IEEE PAMI,2003.
};

struct GMM
{
    float weight;
    float variance;
    
    GMM()
    :weight(1.0f),variance(11.0f) {}
    
    GMM(float _w, float _v)
    :weight(_w),variance(_v) {}
    
    //copy constructor
    GMM (const GMM &_m) { *this = _m; }
    
    GMM & operator = (const GMM & rhs) {
        if ( *this != rhs ) {
            weight   = rhs.weight;
            variance = rhs.variance;
        }
        return *this;
    }
    
    bool operator != (const GMM & rhs) const {
        return (weight != rhs.weight) || (variance != rhs.variance);
    }
    
    bool operator == (const GMM & rhs) const {
        return (weight == rhs.weight) && (variance == rhs.variance);
    }   

};

struct MEAN 
{
    float meanR;
    float meanG;
    float meanB;
    
    MEAN (float _r, float _g, float _b) 
    : meanR(_r), meanG(_g), meanB(_b) { } 
    
    //copy constructor
    MEAN (const MEAN &_m) { *this = _m; }
    
    MEAN & operator = (const MEAN & rhs) {
        if ( *this != rhs ) {
            meanR = rhs.meanR;
            meanG = rhs.meanG;
            meanB = rhs.meanB;
        }
        return *this;
    }
    
    bool operator != (const MEAN & rhs) const {
        return 
        (meanR != rhs.meanR) ||
        (meanG != rhs.meanG) ||
        (meanB != rhs.meanB); 
    }
    
    bool operator == (const MEAN & rhs) const {
        return 
        (meanR == rhs.meanR) &&
        (meanG == rhs.meanG) && 
        (meanB == rhs.meanB); 
    }   
};

// shadow detection performed per pixel
// should work for rgb data, could be usefull for gray scale and depth data as well
// See: Prati,Mikic,Trivedi,Cucchiarra,"Detecting Moving Shadows...",IEEE PAMI,2003.
static CV_INLINE bool
detectShadowGMM(const float* data, int nchannels, int nmodes,
                const GMM* gmm, const float* mean,
                float Tb, float TB, float tau)
{
    float tWeight = 0;

    // check all the components  marked as background:
    for( int mode = 0; mode < nmodes; mode++, mean += nchannels )
    {
        GMM g = gmm[mode];

        float numerator = 0.0f;
        float denominator = 0.0f;
        for( int c = 0; c < nchannels; c++ )
        {
            numerator   += data[c] * mean[c];
            denominator += mean[c] * mean[c];
        }

        // no division by zero allowed
        if( denominator == 0 )
            return false;

        // if tau < a < 1 then also check the color distortion
        if( numerator <= denominator && numerator >= tau*denominator )
        {
            float a = numerator / denominator;
            float dist2a = 0.0f;

            for( int c = 0; c < nchannels; c++ )
            {
                float dD= a*mean[c] - data[c];
                dist2a += dD*dD;
            }

            if (dist2a < Tb*g.variance*a*a)
                return true;
        };

        tWeight += g.weight;
        if( tWeight > TB )
            return false;
    };
    return false;
}

class BackgroundSubtractionInvoker : public ParallelLoopBody
{
public:    
    BackgroundSubtractionInvoker(
                                const Mat& _src, 
                                Mat& _dst,
                                GMM* _gmm, 
                                float* _mean,
                                uchar* _modesUsed,
                                int _nmixtures, 
                                float _alphaT,
                                float _Tb, 
                                float _TB, float _Tg,
                                float _varInit,
                                float _varMin,
                                float _varMax,
                                float _prune,
                                float _tau,
                                bool _detectShadows,
                                uchar _shadowVal,
                                float _globalChange,
                                float* _Cm,
                                float* _Bg,float* _Fg) 
{
    src = &_src;
    dst = &_dst;
    gmm0 = _gmm;
    mean0 = _mean;
    modesUsed0 = _modesUsed;
    nmixtures = _nmixtures;
    alphaT = _alphaT;
    Tb = _Tb;
    TB = _TB;
    Tg = _Tg;
    varInit = _varInit;
    varMin = MIN(_varMin, _varMax);
    varMax = MAX(_varMin, _varMax);
    prune = _prune;
    tau = _tau;
    detectShadows = _detectShadows;
    shadowVal = _shadowVal;
    
    globalChange = _globalChange;
    Cm0 = _Cm;
    Bg0 = _Bg;
    Bg0 = _Fg;

    cvtfunc = src->depth() != CV_32F ? getConvertFunc(src->depth(), CV_32F) : 0;
}

/*
 parallel_for_(Range(0, image.rows),
 BackgroundSubtractionInvoker(
 image, 
 fgmask,
 (GMM*)bgmodel.data,
 (float*)(bgmodel.data + sizeof(GMM)*nmixtures*image.rows*image.cols),
 bgmodelUsedModes.data, 
 nmixtures, 
 (float)learningRate,
 (float)varThreshold,
 backgroundRatio, 
 varThresholdGen,
 fVarInit, fVarMin, fVarMax, 
 float(-learningRate*fCT), 
 fTau,
 bShadowDetection, 
 nShadowDetection));
 */
    
void operator()(const Range& range) const
{
    int y0 = range.start;
    int y1 = range.end;
    
    int ncols     = src->cols;
    int nchannels = src->channels();
    
    AutoBuffer<float> buf(src->cols*nchannels);
    
    float alpha1 = 1.f - alphaT;
    float dData[CV_CN_MAX];

    for( int y = y0; y < y1; y++ )
    {
        const float* data = buf;//data is pointer, which points to const float
        if( cvtfunc )
            cvtfunc( src->ptr(y), src->step, 0, 0, (uchar*)data, 0, Size(ncols*nchannels, 1), 0);
        else
            data = src->ptr<float>(y);

        float* mean      = mean0 + ncols*nmixtures*nchannels*y;
        GMM*   gmm       = gmm0  + ncols*nmixtures*y;
        uchar* modesUsed = modesUsed0 + ncols*y;
        uchar* mask      = dst->ptr(y);
        float* cm        = Cm0 + ncols*nmixtures*y;
        float* bg        = Bg0 + ncols*nchannels*y;
        float* fg        = Fg0 + ncols*y;

        //After each iteration per mixture:
        // increment x
        // data (buffer) incremented by number of channels.
        // ggm incremented number of Gaussians
        // data:
        //
        // |--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
        // |R |G |B |  |  |  |  |  |  |  |  |  |  |  |  |  |
        // |--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
        //
        for( int x = 0; x < ncols; x++, data += nchannels, gmm += nmixtures, mean += nmixtures*nchannels )
        {
            //calculate distances to the modes (+ sort)
            //here we need to go in descending order!!!
            bool background   = false;//return value -> true - the pixel classified as background

            //internal:
            bool fitsPDF      = false;//if it remains zero a new GMM mode will be added
            int nmodes        = modesUsed[x];
            int nNewModes     = nmodes;//current number of modes in GMM
            float totalWeight = 0.f;

            float* mean_m     = mean;
            float* bg_m       = bg;
            float* fg_m       = fg;
            float* bg_cnt     = cm;

            //////
            //go through all modes
            for( int mode = 0; mode < nmodes; mode++, mean_m += nchannels, bg_m +=nchannels )
            {
                // prune = -learningRate*fCT = 1./500*0.05 = -0.0001
                // Ownership Om set zero to obtain weight if fit is not found.
                // Eq (14) ownership in zero
                float weight = alpha1*gmm[mode].weight + prune;//need only weight if fit is found
                
                //// 
                //fit not found yet, at init fitsPDF <-- false
                if( !fitsPDF )
                {
                    //check if it belongs to some of the remaining modes
                    float var = gmm[mode].variance;

                    //calculate difference and distance
                    float dist2;

                    // dData[CV_CN_MAX] == dData[512]
                    // d_dirac_m = x[t] - mu_m
                    if( nchannels == 3 )
                    {
                        dData[0] = mean_m[0] - data[0]*globalChange;
                        dData[1] = mean_m[1] - data[1]*globalChange;
                        dData[2] = mean_m[2] - data[2]*globalChange;
                        dist2 = dData[0]*dData[0] + dData[1]*dData[1] + dData[2]*dData[2];
                    }
                    else
                    {
                        dist2 = 0.f;
                        for( int c = 0; c < nchannels; c++ )
                        {
                            dData[c] = mean_m[c] - data[c]*globalChange;
                            dist2 += dData[c]*dData[c];
                        }
                    }

                    //background? - Tb - usually larger than Tg
                    if( totalWeight < TB && dist2 < Tb*var ) {
                        background = true;
                        /*
                        for( int c = 0; c < nchannels; c++ )
                            bg_m[c] = dData[c];
                         */
                    }

                    //check fit
                    if( dist2 < Tg*var )
                    {
                        /////
                        //belongs to the mode
                        fitsPDF = true;

                        //update distribution
                        modesUsed[mode] = mode + 1;
                                               
                        //New Beta dynamic learning rate, se eq. 4.7
                        //Beta=alfa(h+Cm)/Cm
                        //If the background changes quickly, Cm will become smaller, new beta learning rate will increase
                        float Beta = alphaT/bg_cnt[mode]+alphaT;
                        
                        //
                        float k = Beta/gmm[mode].weight;
                        
                        // Update Weight
                        // Eq (14) of Zivkovic paper
                        // prune = -learningRate*fCT
                        //weight = alpha1*gmm[mode].weight+alphaT + prune;
                        weight += alphaT;
                        
                        // Update mean
                        // Eq (5) 
                        // TODO: Check for more than three channels
                        for( int c = 0; c < nchannels; c++ )
                            mean_m[c] -= k*dData[c];
                        
                        // Eq(6)
                        // update variance
                        float varnew = var + k*(dist2-var);
                        //limit the variance
                        varnew = MAX(varnew, varMin);
                        varnew = MIN(varnew, varMax);
                        gmm[mode].variance = varnew;

                         
                        //sort
                        //all other weights are at the same place and
                        //only the matched (iModes) is higher -> just find the new place for it
                        for( int i = mode; i > 0; i-- )
                        {
                            //check one up
                            if( weight < gmm[i-1].weight )
                                break;

                            //swap one up
                            std::swap(gmm[i], gmm[i-1]);
                            for( int c = 0; c < nchannels; c++ )
                                std::swap(mean[i*nchannels + c], mean[(i-1)*nchannels + c]);
                        }
                        //belongs to the mode - bFitsPDF becomes 1
                        /////
                    }
                }//!bFitsPDF)

                //check prune
                if( weight < -prune )
                {
                    weight = 1.0E-6;
                    nmodes--;
                }

                gmm[mode].weight = weight;//update weight by the calculated value
                totalWeight += weight;
            }
            //go through all modes
            //////

            //renormalize weights
            totalWeight = 1.f/totalWeight;
            for( int mode = 0; mode < nmodes; mode++ )
                gmm[mode].weight *= totalWeight;

            nmodes = nNewModes;

            //make new mode if needed and exit
            if( !fitsPDF )
            {
                // replace the weakest or add a new one
                int mode = nmodes == nmixtures ? nmixtures-1 : nmodes++;

                if (nmodes==1)
                    gmm[mode].weight = 1.f;
                else
                {
                    gmm[mode].weight = alphaT;

                    // renormalize all other weights
                    for( int i = 0; i < nmodes-1; i++ )
                        gmm[i].weight *= alpha1;
                }

                // init
                for( int c = 0; c < nchannels; c++ )
                    mean[mode*nchannels + c] = data[c];

                gmm[mode].variance = varInit;

                //sort
                //find the new place for it
                for( int i = nmodes - 1; i > 0; i-- )
                {
                    // check one up
                    if( alphaT < gmm[i-1].weight )
                        break;

                    // swap one up
                    std::swap(gmm[i], gmm[i-1]);
                    for( int c = 0; c < nchannels; c++ )
                        std::swap(mean[i*nchannels + c], mean[(i-1)*nchannels + c]);
                }
            }

            //set the number of modes
            modesUsed[x] = uchar(nmodes);
            mask[x] = background ? 0 :
                detectShadows && detectShadowGMM(data, nchannels, nmodes, gmm, mean, Tb, TB, tau) ?
                shadowVal : 255;
        }
    }
}

    const Mat* src;
    Mat* dst;
    GMM* gmm0;
    float* mean0;
    uchar* modesUsed0;

    int nmixtures;
    float alphaT, Tb, TB, Tg;
    float varInit, varMin, varMax, prune, tau;

    bool detectShadows;
    uchar shadowVal;

    float globalChange;
    float* Cm0;
    float* Bg0;
    float* Fg0;
    
    BinaryFunc cvtfunc;
};

/*
BackgroundSubtractorMOG3::BackgroundSubtractorMOG3()
{
    frameSize        = Size(0,0);
    frameType        = 0;

    nframes          = 0;
    history          = defaultHistory2; //500 --> 1/500 = 0.002
    varThreshold     = defaultVarThreshold2;
    bShadowDetection = 1;

    nmixtures        = defaultNMixtures2;
    backgroundRatio  = defaultBackgroundRatio2;
    fVarInit         = defaultVarInit2;
    fVarMax          = defaultVarMax2;
    fVarMin          = defaultVarMin2;

    varThresholdGen  = defaultVarThresholdGen2;
    fCT              = defaultfCT2;
    nShadowDetection =  defaultnShadowDetection2;
    fTau             = defaultfTau;
}
*/


BackgroundSubtractorMOG3::BackgroundSubtractorMOG3()
{
    frameSize        = Size(0,0);
    frameType        = 0;
    
    nframes          = 0;
    history          = defaultHistory2; //500 --> 1/500 = 0.002
    varThreshold     = PixelRange;
    bShadowDetection = 1;
    
    nmixtures        = GaussiansNo;
    backgroundRatio  = T;
    fVarInit         = Sigma;
    fVarMax          = SigmaMax;
    fVarMin          = SigmaMin;
    
    varThresholdGen  = PixelGen;
    fCT              = CT;
    nShadowDetection =  defaultnShadowDetection2;
    fTau             = Tau;
}


BackgroundSubtractorMOG3::BackgroundSubtractorMOG3(int _history,  float _varThreshold, bool _bShadowDetection)
{
    frameSize        = Size(0,0);
    frameType        = 0;

    nframes          = 0;
    history          = _history > 0 ? _history : defaultHistory2;
    varThreshold     = (_varThreshold>0)? _varThreshold : PixelRange;
    bShadowDetection = _bShadowDetection;

    nmixtures        = GaussiansNo;
    backgroundRatio  = T;
    fVarInit         = Sigma;
    fVarMax          = SigmaMax;
    fVarMin          = SigmaMin;

    varThresholdGen  = PixelGen;
    fCT              = CT;
    nShadowDetection =  defaultnShadowDetection2;
    fTau             = Tau;
}

BackgroundSubtractorMOG3::~BackgroundSubtractorMOG3()
{
}


void BackgroundSubtractorMOG3::initialize(Size _frameSize, int _frameType)
{
    frameSize = _frameSize;
    frameType = _frameType;
    nframes = 0;

    int nchannels = CV_MAT_CN(frameType);
    CV_Assert( nchannels <= CV_CN_MAX );

    // for each gaussian mixture of each pixel bg model we store ...
    // the mixture weight (w),
    // the mean (nchannels values) and
    // the covariance
    //bgmodel.create( 1, frameSize.height*frameSize.width*nmixtures*(2 + nchannels), CV_32F );
    
    //make the array for keeping track of the used modes per pixel - all zeros at start
    //bgmodelUsedModes.create(frameSize,CV_8U);
    //bgmodelUsedModes = Scalar::all(0);
    
    
    int matSize   = frameSize.height*frameSize.width;
    int columnsNo = matSize*nmixtures*(2 + nchannels);
    
    GaussianModel.create(1, columnsNo, CV_32F );
    GMM* ptrGMM = (GMM*)GaussianModel.data;
    GMM*   data  = ptrGMM;
    
    MEAN* ptrMean = (MEAN*)(GaussianModel.data + 2*nmixtures*matSize);
    MEAN*  ptrm  = ptrMean;
    
    for (int i=0; i<matSize; i++) {
        data = ptrGMM + i*nmixtures;
        data->weight     = 1.0f;
        data->variance = 11.0f;
        
        //initialize first gaussian mean (RGB)
        ptrm = ptrMean + i*nmixtures;
        ptrm->meanR = 1.0f;
        ptrm->meanG = 1.0f;
        ptrm->meanB = 1.0f;
    }
    
    CurrentGaussianModel.create(frameSize, CV_8U);
    //CurrentGaussianModel = Scalar(1,0,0,0);
    CurrentGaussianModel = Scalar::all(0);
    
    
    BackgroundNumberCounter.create(1, frameSize.height*frameSize.width*nmixtures, CV_32F);
    float* ptrCnt = (float*)BackgroundNumberCounter.data;
    int nc= BackgroundNumberCounter.cols * BackgroundNumberCounter.channels();
    for (int i=0; i<nc; i+=nmixtures)
        ptrCnt[i]= 1.0f;
    
    //Keep a result of background and foreground every call processing
    Background.create(1, matSize*nchannels, CV_32F);
    Foreground.create(1, matSize,           CV_32F);
    
    
}

void BackgroundSubtractorMOG3::operator()(InputArray _image, OutputArray _fgmask, double learningRate)
{
    Mat image = _image.getMat();
    bool needToInitialize = nframes == 0 || 
                            learningRate >= 1 || 
                            image.size() != frameSize || 
                            image.type() != frameType;

    if( needToInitialize )
        initialize(image.size(), image.type());

    _fgmask.create( image.size(), CV_8U );
    Mat fgmask = _fgmask.getMat();

    ++nframes;

    //learningRate = learningRate >= 0 && nframes > 1 ? learningRate : 1./min( 2*nframes, history );
    learningRate = Alpha;
    CV_Assert(learningRate >= 0);

    //Global illumination changing factor 'g' between reference image ir and current image ic.
    float globalIlluminationFactor = 1.0;
    
  
    BackgroundSubtractionInvoker invoker(
            image, 
            fgmask, 
            (GMM*)GaussianModel.data, 
            (float*)(GaussianModel.data + sizeof(GMM)*nmixtures*image.rows*image.cols),
            CurrentGaussianModel.data, 
            nmixtures, 
            (float)learningRate,
            (float)varThreshold,
            backgroundRatio, 
            varThresholdGen,
            fVarInit, 
            fVarMin, 
            fVarMax, 
            float(-learningRate*fCT), 
            fTau,
            bShadowDetection, 
            nShadowDetection,
            globalIlluminationFactor,
            (float *)BackgroundNumberCounter.data,
            (float *)Background.data, (float *)Foreground.data);
    
    parallel_for_(Range(0, image.rows), invoker);

}

void BackgroundSubtractorMOG3::getBackgroundImage(OutputArray backgroundImage) const
{
    int nchannels = CV_MAT_CN(frameType);
    CV_Assert( nchannels == 3 );
    Mat meanBackground(frameSize, CV_8UC3, Scalar::all(0));

    int firstGaussianIdx = 0;
    const GMM* gmm = (GMM*)GaussianModel.data;
    const Vec3f* mean = reinterpret_cast<const Vec3f*>(gmm + frameSize.width*frameSize.height*nmixtures);
    for(int row=0; row<meanBackground.rows; row++)
    {
        for(int col=0; col<meanBackground.cols; col++)
        {
            int nmodes = CurrentGaussianModel.at<uchar>(row, col);
            Vec3f meanVal;
            float totalWeight = 0.f;
            for(int gaussianIdx = firstGaussianIdx; gaussianIdx < firstGaussianIdx + nmodes; gaussianIdx++)
            {
                GMM gaussian = gmm[gaussianIdx];
                meanVal += gaussian.weight * mean[gaussianIdx];
                totalWeight += gaussian.weight;

                if(totalWeight > backgroundRatio)
                    break;
            }

            meanVal *= (1.f / totalWeight);
            meanBackground.at<Vec3b>(row, col) = Vec3b(meanVal);
            firstGaussianIdx += nmixtures;
        }
    }

    switch(CV_MAT_CN(frameType))
    {
    case 1:
    {
        vector<Mat> channels;
        split(meanBackground, channels);
        channels[0].copyTo(backgroundImage);
        break;
    }

    case 3:
    {
        meanBackground.copyTo(backgroundImage);
        break;
    }

    default:
        CV_Error(CV_StsUnsupportedFormat, "");
    }
}


/* End of file. */
