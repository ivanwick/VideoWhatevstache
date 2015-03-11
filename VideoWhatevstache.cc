
#include <OpenCV/OpenCV.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cassert>
#include <iostream>


const char  * WINDOW_NAME  = "UR FACE WHATEVS";
const CFIndex CASCADE_NAME_LEN = 2048;
      char    CASCADE_NAME[CASCADE_NAME_LEN] = "haarcascade_frontalface_alt2.xml"; // this is a dummy

const CFIndex WHATEVSTACHE_NAME_LEN = 2048; // eventually accommodates full path
	  char    WHATEVSTACHE_NAME[WHATEVSTACHE_NAME_LEN] = "";

using namespace std;

IplImage * whatevstache_img;

void overlayWhatevstache(IplImage * img, CvRect r, CvPoint center, int radius);

int main (int argc, char * const argv[]) 
{
    const int scale = 2;

    // locate resources from inside application bundle
    // (this is the mac way to package application resources)
	CFBundleRef mainBundle;
	CFURLRef bundle_resource_url;
	Boolean got_it;
	
    mainBundle  = CFBundleGetMainBundle ();
    assert (mainBundle);
    
	bundle_resource_url = CFBundleCopyResourceURL(mainBundle,
												  CFSTR("haarcascade_frontalface_alt2"),
												  CFSTR("xml"), NULL);
    assert (bundle_resource_url);
    got_it = CFURLGetFileSystemRepresentation(bundle_resource_url, true, 
                                              reinterpret_cast<UInt8 *>(CASCADE_NAME), CASCADE_NAME_LEN);
    if (! got_it) abort ();
    
	bundle_resource_url = CFBundleCopyResourceURL(mainBundle, CFSTR("whatevstache-alpha-red"), CFSTR("png"), NULL);
	assert(bundle_resource_url);
	got_it = CFURLGetFileSystemRepresentation(bundle_resource_url, true,
											  reinterpret_cast<UInt8 *>(WHATEVSTACHE_NAME), WHATEVSTACHE_NAME_LEN);
    if (! got_it) abort();
	
	// create all necessary instances
    cvNamedWindow (WINDOW_NAME, CV_WINDOW_AUTOSIZE);
    CvCapture * camera = cvCreateCameraCapture (CV_CAP_ANY);
    CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*) cvLoad (CASCADE_NAME, 0, 0, 0);
    CvMemStorage* storage = cvCreateMemStorage(0);
    assert (storage);

    // you do own an iSight, don't you ?!?
    if (! camera)
        abort ();

    // did we load the cascade?!?
    if (! cascade)
        abort ();

    // get an initial frame and duplicate it for later work
    IplImage *  current_frame = cvQueryFrame (camera);
    IplImage *  draw_image    = cvCreateImage(cvSize (current_frame->width, current_frame->height), IPL_DEPTH_8U, 3);
    IplImage *  gray_image    = cvCreateImage(cvSize (current_frame->width, current_frame->height), IPL_DEPTH_8U, 1);
    IplImage *  small_image   = cvCreateImage(cvSize (current_frame->width / scale, current_frame->height / scale), IPL_DEPTH_8U, 1);
    assert (current_frame && gray_image && draw_image);
    
	whatevstache_img  = cvLoadImage(WHATEVSTACHE_NAME, CV_LOAD_IMAGE_UNCHANGED);
	
    // as long as there are images ...
    while (current_frame = cvQueryFrame (camera))
    {
        // convert to gray and downsize
        cvCvtColor (current_frame, gray_image, CV_BGR2GRAY);
        cvResize (gray_image, small_image);
        
        // detect faces
        CvSeq* faces = cvHaarDetectObjects (small_image, cascade, storage,
                                            1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
                                            cvSize (30, 30));
        
        // draw faces
        cvFlip(current_frame, draw_image, 1);

        for (int i = 0; i < (faces ? faces->total : 0); i++)
        {
            CvRect* r = (CvRect*) cvGetSeqElem (faces, i);
            CvPoint center;
            int radius;
            center.x = cvRound((small_image->width - r->width*0.5 - r->x) *scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
			
            //cvCircle (draw_image, center, radius, CV_RGB(0,255,0), 3, 8, 0 );

			// all this arithmetic crept into the funcall args here because the
			// image is flipped in order to provide a more natural "mirror"
			// effect to a front-facing camera.
			// TODO - generalize the coordinate flipping
			overlayWhatevstache(draw_image, cvRect(center.x - ((r->width * 0.5) * scale),
 												   center.y - ((r->height * 0.5) * scale),
												   r->width * scale, r->height * scale),
								center, radius);
        }
        
        // just show the image
        cvShowImage (WINDOW_NAME, draw_image);
        
        // wait a tenth of a second for keypress and window drawing
        int key = cvWaitKey (100);
        if (key == 'q' || key == 'Q' || key == 27)
            break;
    }
    
    // be nice and return no error
    return 0;
}


/* given a rectangle in_rect, resize it so that it remains within the bounds
 defined by the region_size.
 
 WARNING: THIS FUNCTION MISSES A LOT OF CASES AND ONLY MILDY WORKS
          e.g. what if in_rect.x < -(in_rect.width) or r.y < -(in_rect.height)
					   in_rect.x + in_rect.width > region_size.width
 
          or more generally, when in_rect is fully out of bounds of region_size
		  (no overlap at all) and cannot be simply truncated to fit.
 */
CvRect resizeRectWithinRegion(CvRect in_rect, CvSize region_size)
{
	CvRect r = in_rect;
	r.x = max(r.x, 0);
	r.y = max(r.y, 0);
	
	r.width = min(r.width + r.x, region_size.width) - r.x;
	r.height = min(r.height + r.y, region_size.height) - r.y;
	
	return r;
}

void overlayWhatevstache(IplImage * img, CvRect rect, CvPoint center, int radius)
{
	// move rect down a bit so that the overlay is at the upper-lip position
    rect.y += rect.height * 0.32;

	CvSize img_size = cvGetSize(img);
	
	// keep from drawing off the bottom of the captured image
	//if (rect.y + rect.height > img_size.height) {
	//	rect.height = img_size.height - rect.y;
	//}
	//  ... or to express it another way:
	//rect.height = min(rect.height + rect.y, img_size.height) - rect.y;
	//  ... or to just call out to the function
	rect = resizeRectWithinRegion(rect, img_size);
	
	IplImage *resized_image = cvCreateImage(cvSize(rect.width, rect.height),IPL_DEPTH_8U, 3);
	cvResize(whatevstache_img, resized_image); 

	cvSetImageROI(img, rect);
	cvAddWeighted(resized_image, 1.0, img, 1.0, 0.0, img);
//	cvCopy(resized_image, img);
	
	
	cvResetImageROI(img);
	cvReleaseImage(&resized_image);
}


#if 0
/* TRASHCODE */
int main(int argc, char *const argv[])
{
	cv::Mat_<float> mask = imread("/Users/ivan/opencv-code/whatevstache-mask.png", 0);
	threshold(mask, mask, 254., 1., THRESH_BINARY);
	
	std::vector<Mat> marr(3, mask);
	Mat_<Vec<float, 3> > maskRGB;
	merge(marr, maskRGB);
	
	imshow("Target", target);
	imshow("Mask", mask*255);
	imshow("Applied", target.mul(maskRGB));
	waitKey();
}
#endif

