#ifndef IMG_H
#define IMG_H
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

std::string ocr(std::string str);

std::vector<cv::String> getOutputsNames(const cv::dnn::Net& net);

void postprocess(cv::Mat& frame, const std::vector<cv::Mat>& outs);

void drawPred(int classId, float conf, int left, int top, int right, int bottom, cv::Mat& frame);

std::string processImage(std::string filename);






#endif
