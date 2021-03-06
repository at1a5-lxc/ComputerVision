#define test2
#ifdef test2

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <vector>
#include <iostream>
#include <nonfree/nonfree.hpp>
#include <cstdio>
// OS Specific sleep
#include <windows.h>
#include "serial.h"

using namespace serial;
using namespace cv;
using namespace std;
Mat src,frameImg;
int width;
int height;
int xsum=0,ysum=0;
Point leftup,rightdown;
vector<Point> srcCorner(4);
vector<Point> dstCorner(4);
int currentx=90;
int currentz=90;
Serial serial1("COM10", 19200, serial::Timeout::simpleTimeout(1000),eightbits,parity_none,stopbits_one,flowcontrol_none);
Mat imgtoshow;
void handlerX(int num)
{
	currentx=num;
	string dataout;
	dataout="<X";
	char temp[10];
	string tt;
	itoa(num,temp,10);
	tt=temp;
	dataout+=tt;
	dataout+=">";
	serial1.write(dataout);
	cout<<dataout<<endl;
	//Sleep(100);

}
void handlerZ(int num)
{
	currentz=num;
	string dataout;
	dataout="<Z";
	char temp[10];
	string tt;
	itoa(num,temp,10);
	tt=temp;
	dataout+=tt;
	dataout+=">";
	serial1.write(dataout);
	cout<<dataout<<endl;
	//Sleep(100);

}



static bool createDetectorDescriptorMatcher( const string& detectorType, const string& descriptorType, const string& matcherType,
											Ptr<FeatureDetector>& featureDetector,
											Ptr<DescriptorExtractor>& descriptorExtractor,
											Ptr<DescriptorMatcher>& descriptorMatcher )
{
	cout << "< Creating feature detector, descriptor extractor and descriptor matcher ..." << endl;
        if (detectorType=="SIFT"||detectorType=="SURF")
			
		initModule_nonfree();
	featureDetector = FeatureDetector::create( detectorType );
	descriptorExtractor = DescriptorExtractor::create( descriptorType );
	descriptorMatcher = DescriptorMatcher::create( matcherType );
	cout << ">" << endl;

	bool isCreated = !( featureDetector.empty() || descriptorExtractor.empty() || descriptorMatcher.empty() );
	if( !isCreated )
		cout << "Can not create feature detector or descriptor extractor or descriptor matcher of given types." << endl << ">" << endl;
	
	return isCreated;
}


bool refineMatchesWithHomography(const std::vector<cv::KeyPoint>& queryKeypoints,  
								 const std::vector<cv::KeyPoint>& trainKeypoints,   
								 float reprojectionThreshold,  
								 std::vector<cv::DMatch>& matches,  
								 cv::Mat& homography  )
{
	const int minNumberMatchesAllowed = 10;  
	if (matches.size() < minNumberMatchesAllowed)  
		return false;  
	// Prepare data for cv::findHomography  
	std::vector<cv::Point2f> queryPoints(matches.size());  
	std::vector<cv::Point2f> trainPoints(matches.size()); 
	imgtoshow=Scalar(0);
	for (size_t i = 0; i < matches.size(); i++)  
	{  
		
		queryPoints[i] = queryKeypoints[matches[i].queryIdx].pt;  
		trainPoints[i] = trainKeypoints[matches[i].trainIdx].pt;  
		//imgtoshow.at<uchar>(trainPoints[i].y,trainPoints[i].x)=255;
	}  
	cout<<"Train Points:"<<trainPoints[0]<<" "<<trainPoints[1]<<endl;

	 xsum=ysum=0;
	
	for(int i=0;i<matches.size();i++)
	{
		xsum+=trainPoints[i].x;
		ysum+=trainPoints[i].y;
	}
	xsum/=matches.size();
	ysum/=matches.size();
	






	//Mat element=getStructuringElement(0,Size(10,10));
	//dilate(imgtoshow,imgtoshow,element);
	//dilate(imgtoshow,imgtoshow,element);
	//imshow("imgtoshow",imgtoshow);

	// Find homography matrix and get inliers mask  
	std::vector<unsigned char> inliersMask(matches.size());  
	homography = cv::findHomography(queryPoints,   
		trainPoints,   
		CV_FM_RANSAC,   
		reprojectionThreshold,   
		inliersMask);  
	std::vector<cv::DMatch> inliers;  
	for (size_t i=0; i<inliersMask.size(); i++)  
	{  
		if (inliersMask[i])  
			inliers.push_back(matches[i]);  
	}  
	matches.swap(inliers);
	Mat homoShow;
	drawMatches(src,queryKeypoints,frameImg,trainKeypoints,matches,homoShow,Scalar::all(-1),CV_RGB(255,255,255),Mat(),2);     
	//imshow("homoShow",homoShow); 
	return matches.size() > minNumberMatchesAllowed; 

}


bool matchingDescriptor(const vector<KeyPoint>& queryKeyPoints,const vector<KeyPoint>& trainKeyPoints,
						const Mat& queryDescriptors,const Mat& trainDescriptors, 
						Ptr<DescriptorMatcher>& descriptorMatcher,
						bool enableRatioTest = true)
{
	vector<vector<DMatch>> m_knnMatches;
	vector<DMatch>m_Matches;
	
	if (enableRatioTest)
	{
		//cout<<"KNN Matching"<<endl;
		const float minRatio = 1.f / 1.5f;
		descriptorMatcher->knnMatch(queryDescriptors,trainDescriptors,m_knnMatches,2);
		for (size_t i=0; i<m_knnMatches.size(); i++)
		{
			const cv::DMatch& bestMatch = m_knnMatches[i][0];
			const cv::DMatch& betterMatch = m_knnMatches[i][1];
			float distanceRatio = bestMatch.distance / betterMatch.distance;
			if (distanceRatio < minRatio)
			{
				m_Matches.push_back(bestMatch);
			}
		}

	}
	else
	{
		cout<<"Cross-Check"<<endl;
		Ptr<cv::DescriptorMatcher> BFMatcher(new cv::BFMatcher(cv::NORM_HAMMING, true));
		BFMatcher->match(queryDescriptors,trainDescriptors, m_Matches );
	}
	Mat homo;
	float homographyReprojectionThreshold = 1.0;
	bool homographyFound = refineMatchesWithHomography(
		queryKeyPoints,trainKeyPoints,homographyReprojectionThreshold,m_Matches,homo);
	
	if (!homographyFound)
		return false;
	else
	{
		double h[9];
		bool isFound=true;
		h[0]=homo.at<double>(0,0);  
		h[1]=homo.at<double>(0,1);  
		h[2]=homo.at<double>(0,2);   
		h[3]=homo.at<double>(1,0);  
		h[4]=homo.at<double>(1,1);  
		h[5]=homo.at<double>(1,2);  
		h[6]=homo.at<double>(2,0);  
		h[7]=homo.at<double>(2,1);  
		h[8]=homo.at<double>(2,2);
	/*	printf("homo\n"  
			"%f %f %f\n"  
			"%f %f %f\n"  
			"%f %f %f\n",   
			homo.at<double>(0,0), homo.at<double>(0,1), homo.at<double>(0,2),   
			homo.at<double>(1,0), homo.at<double>(1,1), homo.at<double>(1,2),   
			homo.at<double>(2,0), homo.at<double>(2,1), homo.at<double>(2,2));   
			*/
		for (int i=0;i<4;i++)
		{
			float x = (float)srcCorner[i].x;
			float y = (float)srcCorner[i].y;  
			float Z = (float)1./(h[6]*x + h[7]*y + h[8]);  
			float X = (float)(h[0]*x + h[1]*y + h[2])*Z;  
			float Y = (float)(h[3]*x + h[4]*y + h[5])*Z; 
			//if(X>=0&&X<width&&Y>=0&&Y<height)
			{
				dstCorner[i]=Point(int(X),int(Y));
			}
			//else
			{
				//isFound &= false;
			}
		}
		if (isFound)
		{

			Point center_(xsum,ysum);
	circle(frameImg,center_,5,Scalar(0,0,255));
	


	if(xsum<leftup.x)
	{
		currentx--;
	}
	else if(xsum>rightdown.x)
	{
		currentx++;
	}

     if(ysum<leftup.y)
	 {
		 currentz--;
	 }
	 else if(ysum>rightdown.y)
	 {
		 currentz++;
	 }
	 handlerX(currentx);
	 handlerZ(currentz);









			line(frameImg,dstCorner[0],dstCorner[1],CV_RGB(255,0,0),2);
			line(frameImg,dstCorner[1],dstCorner[2],CV_RGB(255,0,0),2);
			line(frameImg,dstCorner[2],dstCorner[3],CV_RGB(255,0,0),2);
			line(frameImg,dstCorner[3],dstCorner[0],CV_RGB(255,0,0),2);
			return true;
		}
		return true;
	}
		

}





int main()
{
	string filename = "x:\\book.png";
	src = imread(filename,0);
	width = src.cols;
	height = src.rows;
	string detectorType = "SURF";
	string descriptorType = "SURF";
	string matcherType = "BruteForce";


	cvNamedWindow("foundImg",1);
	int startx=90,startz=90;
	cvCreateTrackbar("X","foundImg",&startx,180,handlerX);
	cvCreateTrackbar("Z","foundImg",&startz,180,handlerZ);
	
	Ptr<FeatureDetector> featureDetector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	Ptr<DescriptorMatcher> descriptorMatcher;
	if( !createDetectorDescriptorMatcher( detectorType, descriptorType, matcherType, featureDetector, descriptorExtractor, descriptorMatcher ) )
	{
		cout<<"Creat Detector Descriptor Matcher False!"<<endl;
		return -1;
	}
	//Intial: read the pattern img keyPoint
	 vector<KeyPoint> queryKeypoints;
	 Mat queryDescriptor;
	 featureDetector->detect(src,queryKeypoints);
	 descriptorExtractor->compute(src,queryKeypoints,queryDescriptor);
	 //cout<<queryDescriptor;

	 VideoCapture cap(1); // open the default camera
	 if(!cap.isOpened())  // check if we succeeded
	 {
		 cout<<"Can't Open Camera!"<<endl;
		 return -1;
	 }
	 srcCorner[0] = Point(0,0);
	 srcCorner[1] = Point(width,0);
	 srcCorner[2] = Point(width,height);
	 srcCorner[3] = Point(0,height);

	 vector<KeyPoint> trainKeypoints;
	 Mat trainDescriptor;

	 Mat frame,grayFrame;
	 char key=0;
	 
	//frame = imread("D:\\Img\\Heads\\00.jpg");
	cap>>frame;

	leftup.x=frame.cols/2-35;
	leftup.y=frame.rows/2-35;
	rightdown.x=frame.cols/2+35;
	rightdown.y=frame.rows/2+35;
	Mat imgtoshow2(frame.size(),CV_8UC1);
	imgtoshow=imgtoshow2.clone();
	 while (key!=27)
	 {
		 cap>>frame;

		 frame.copyTo(frameImg);
		 cvtColor(frame,grayFrame,CV_BGR2GRAY);
		 trainKeypoints.clear();
		// if (!trainDescriptor.empty())
			trainDescriptor.setTo(0);
		 featureDetector->detect(grayFrame,trainKeypoints);
		
		 if(trainKeypoints.size()!=0)
		 {
			 descriptorExtractor->compute(grayFrame,trainKeypoints,trainDescriptor);
			 bool isFound = matchingDescriptor(queryKeypoints,trainKeypoints,queryDescriptor,trainDescriptor,descriptorMatcher);
			 rectangle(frameImg,leftup,rightdown,Scalar(255,0,0),2);
			 imshow("foundImg",frameImg);
			 
		 }
		 key = waitKey(1);
		 
	 }
	 
}

#endif