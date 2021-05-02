#include "imageProcessing.h"
std::vector<std::string> preds;
std::vector<std::string> classes;
float confThreshold = 0.5; // Confidence threshold
float nmsThreshold = 0.4;  // Non-maximum suppression threshold
int main() {
    std::string str = processImage("output.jpg");
    str += ". ";
    /*std::string str2 = ocr("output.jpg");
    if (str2.length() < 2) {
        str2 = "nothing";
    }
    while(str2.find("\n") != std::string::npos)
    {
        str2.erase(str2.find("\n"), 1);
    }
    str += str2;
    str += ", was detected from the image as text.";*/
    std::ofstream out("out.txt");
    std::cout << str;
    out << str;
    out.flush();
    out.close();
    return 0;
}
std::string ocr(std::string str) {
    cv::Mat img = cv::imread(str);
    cv::threshold(img, img, 127, 255, cv::THRESH_BINARY);
    cv::imwrite("ocr.jpg", img);

    //write the image so we can read it with ocr.
    //cv::imwrite(maskImage, textImage);


    //cv::imshow("Masked Image", imageMasked);
    //cv::imshow("Modified Masked Image", textImage);
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    // Initialize tesseract-ocr with English, without specifying tessdata path
    if (api->Init("C:/Users/buzz/source/repos/Road Sign Detection/Road Sign Detection/tessdata/", "eng")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    Pix* pixImage;
    char* outText = nullptr;

    // Open input image with leptonica library
    pixImage = pixRead("ocr.jpg");
    api->SetImage(pixImage);
    // Get OCR result
    outText = api->GetUTF8Text();
    std::string str2 = outText;
    //write it to the highlighted image above the rect.

    //printf("OCR output:\n%s", outText);
    

    api->End();
    delete api;
    delete[] outText;
    pixDestroy(&pixImage);
    return str2;
}
// Get the names of the output layers
std::vector<cv::String> getOutputsNames(const cv::dnn::Net& net)
{
    static std::vector<cv::String> names;
    if (names.empty())
    {
        //Get the indices of the output layers, i.e. the layers with unconnected outputs
        std::vector<int> outLayers = net.getUnconnectedOutLayers();

        //get the names of all the layers in the network
        std::vector<cv::String> layersNames = net.getLayerNames();

        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}

// Remove the bounding boxes with low confidence using non-maxima suppression
void postprocess(cv::Mat& frame, const std::vector<cv::Mat>& outs)
{
    std::vector<int> classIds;
    std:: vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (size_t i = 0; i < outs.size(); ++i)
    {
        // Scan through all the bounding boxes output from the network and keep only the
        // ones with high confidence scores. Assign the box's class label as the class
        // with the highest score for the box.
        float* data = (float*)outs[i].data;
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
        {
            cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            cv::Point classIdPoint;
            double confidence;
            // Get the value and location of the maximum score
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if (confidence > confThreshold)
            {
                int centerX = (int)(data[0] * frame.cols);
                int centerY = (int)(data[1] * frame.rows);
                int width = (int)(data[2] * frame.cols);
                int height = (int)(data[3] * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;

                classIds.push_back(classIdPoint.x);
                confidences.push_back((float)confidence);
                boxes.push_back(cv::Rect(left, top, width, height));
            }
        }
    }

    // Perform non maximum suppression to eliminate redundant overlapping boxes with
    // lower confidences
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        cv::Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y,
            box.x + box.width, box.y + box.height, frame);
    }
}

// Draw the predicted bounding box
void drawPred(int classId, float conf, int left, int top, int right, int bottom, cv::Mat& frame)
{
    //Draw a rectangle displaying the bounding box
    cv::rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(255, 178, 50), 3);

    //Get the label for the class name and its confidence
    std::string label = cv::format("%.2f", conf);
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ":" + label;
    }

    //Display the label at the top of the bounding box
    int baseLine;
    cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = std::max(top, labelSize.height);
    rectangle(frame, cv::Point(left, top - round(1.5 * labelSize.height)), cv::Point(left + round(1.5 * labelSize.width), top + baseLine), cv::Scalar(255, 255, 255), cv::FILLED);
    putText(frame, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 0, 0), 1);
    //TODO LATER
    std::string color = "";
    preds.push_back(color + classes[classId]);
}

std::string processImage(std::string filename) {
	cv::Mat img = cv::imread(filename);

	cv::rotate(img, img, cv::ROTATE_90_CLOCKWISE);
    //cv::imshow("input image", img);
    //YOLO object detection
                //https://opencv-tutorial.readthedocs.io/en/latest/yolo/yolo.html
                //https://answers.opencv.org/question/238740/cvdnnblobfromimage-works-in-python-but-fails-in-c/
    //Load names of classesand get random colors
   // std::string names[80] = { "person","bicycle","car","motorcycle","airplane","bus","train","truck","boat","traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair","couch","potted plant","bed","dining table","toilet","tv","laptop","mouse","remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear","hair drier","toothbrush", };
    std::string file = "coco.names";// parser.get<String>("classes");
    std::ifstream ifs(file.c_str());
    if (!ifs.is_open())
        perror("Error::StsError File not found");
    std::string line;
    while (std::getline(ifs, line))
    {
        classes.push_back(line);
    }
   
   //random colors
    
    cv::dnn::Net net = cv::dnn::readNetFromDarknet("yolov3.cfg", "yolov3.weights");
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(0);

    // Create a window
    static const std::string kWinName = "Deep learning image classification in OpenCV";
    //cv::namedWindow(kWinName, cv::WINDOW_NORMAL);


    std::vector<cv::String> ln = net.getLayerNames();

    //create a blob
    cv::Mat blob;// = cv::dnn::blobFromImage(img, 1.0 / 255.0, cv::Size(416, 416), 0, true, false);
    cv::dnn::blobFromImage(img, blob, 1.0/255.0, cv::Size(416, 416), cv::Scalar(104,117,123), true, false);


    //cv::Mat r = blob[0, 0, :, : ]
    net.setInput(blob);
    // Runs the forward pass to get output of the output layers
    std::vector<cv::Mat> outs;
    net.forward(outs, getOutputsNames(net));

    // Remove the bounding boxes with low confidence
    postprocess(img, outs);

    // Put efficiency information. The function getPerfProfile returns the overall time for inference(t) and the timings for each of the layers(in layersTimes)
    std::vector<double> layersTimes;
    double freq = cv::getTickFrequency() / 1000;
    double t = net.getPerfProfile(layersTimes) / freq;
    std::string label = cv::format("Inference time for a frame : %.2f ms", t);
    putText(img, label, cv::Point(0, 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));

    // Write the frame with the detection boxes
    cv::Mat detectedFrame;
    img.convertTo(detectedFrame, CV_8U);

    /*cv::imshow(kWinName, img);
    cv::imshow("test", detectedFrame);
	cv::waitKey();*/
    std::string retMessage;
    for (std::vector<std::string>::size_type i = 0; i != preds.size(); i++) {
        if (preds[i][0] == 'A' || preds[i][0] == 'E' || preds[i][0] == 'I' || preds[i][0] == 'O' || preds[i][0] == 'U')
            retMessage += "an ";
        else
            retMessage += "a ";
        retMessage += preds[i];
        if (i + 1 != preds.size()) {
            retMessage += ", and ";
        }
    }
    std::cout << retMessage << std::endl;
	return retMessage;
}
