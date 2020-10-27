#include "Posenet.h"
#include <string>
#include <stdio.h>
#include <android/log.h>
#define LOG_TAG "POSENET.CC"

#define LOG(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

//TfLiteInterpreterOptions* TfLiteInterpreterOptionsCreate();
using namespace std;

namespace ORB_SLAM2
{




//std::unique_ptr<TfLiteInterpreter> interpreter;
//TfLiteInterpreter interpreter;






Posenet::Posenet(const char* pFilename, Device pDevice) {
    filename = pFilename;
    device = pDevice;
    model = TfLiteModelCreateFromFile(pFilename);
    if (model == NULL) {
        LOG("Posenet model initialization failed");
    }
    else {
        LOG("Posenet model init success");
    }
}

//default constructor to avoid errors when we declare a new posenet in header files
Posenet::Posenet()
{}


    //set up the Tensorflow inference interpreter
    TfLiteInterpreter* Posenet::getInterpreter() {
        LOG("checking..");
        //get the Posenet Interpreter instance if it already exists
        if (interpreter != NULL) {
            return interpreter;
        }

        //otherwise we need to create a new interpreter
        LOG("getInterpreter(): need to create new");

        //create interpreter options
        options = TfLiteInterpreterOptionsCreate();

        //set number of threads to use
        TfLiteInterpreterOptionsSetNumThreads(options, NUM_LITE_THREADS);

        if (device == Device::GPU) {
            //set up GPU delegate

            //gpuDelegate = GpuDelegate()
            //options.addDelegate(gpuDelegate)
        }
        else if (device == Device::NNAPI) {
            //what to do here?
            //Device.NNAPI -> options.setUseNNAPI(true)
        }


/*
        //customize the gpu delegate
        const TfLiteGpuDelegateOptionsV2 gpuDelegate = {
                .is_precision_loss_allowed = false,
                .inference_preference = TFLITE_GPU_INFERENCE_PREFERENCE_FAST_SINGLE_ANSWER,
                .inference_priority1 = TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY,
                .inference_priority2 = TFLITE_GPU_INFERENCE_PRIORITY_AUTO,
                .inference_priority3 = TFLITE_GPU_INFERENCE_PRIORITY_AUTO
        }; //TfLiteGpuDelegateOptionsV2Default() doesn't fix the error either
*/

        //TfLiteDelegate* delegate = TfLiteGpuDelegateV2Create(&gpuDelegate);

        //TfLiteDelegate* delegate = TfLiteGpuDelegateV2Default(&gpuDelegate);


        //if (interpreter->ModifyGraphWithDelegate(delegate) != kTfLiteOk) return false;


        //TfLiteInterpreterOptionsAddDelegate(options, delegate);

        LOG("getInterpreter(): running interpretercreate()");

        //instantiate the interpreter using the model we loaded
        TfLiteInterpreter* newInterpreter = TfLiteInterpreterCreate(model, options);

        if (newInterpreter == NULL) {
            LOG("interpreter create failed");
            return NULL;
        }

        LOG("Interpreter create success");

        LOG("getInterpreter(): allocating tensors for new interpreter");

        //allocate tensors for the interpreter
        if (TfLiteInterpreterAllocateTensors(newInterpreter) != kTfLiteOk) {
            LOG("TfLite allocate tensors failed");
            return NULL;
        }

        LOG("getInterpreter(): tensor allocation finished successfully");

        //save the newly created interpreter
        interpreter = newInterpreter;

        return newInterpreter;
/*
        TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
        const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        //TfLiteStatus from_status = TfLiteTensorCopyFromBuffer(input_tensor, input_data, TfLiteTensorByteSize(input_tensor));


        //TfLiteStatus interpreter_invoke_status = TfLiteInterpreterInvoke(interpreter);


        //TfLiteStatus to_status = TfLiteTensorCopyToBuffer(output_tensor, output_data, TfLiteTensorByteSize(output_tensor));


        //if (interpreter->ModifyGraphWithDelegate(delegate) != kTfLiteOk) return;


        //ops::builtin::BuiltinOpResolver op_resolver;

        //TfLiteInterpreter* interpreter;

        //InterpreterBuilder(*model, op_resolver)(&interpreter);


        return interpreter;*/
}

    //clean up the interpreter and possibly the gpuDelegate
    void Posenet::close() {
        //delete the interpreter we made, if it exists
        if (interpreter != NULL) {
            TfLiteInterpreterDelete(interpreter);
        }

        if (options != NULL) {
            TfLiteInterpreterOptionsDelete(options);
        }

        if (model != NULL) {
            TfLiteModelDelete(model);
        }
    }

    //Scale the image pixels to a float array of [-1,1] values.
    std::vector<float> Posenet::initInputArray(cv::Mat incomingImg) { //the mat will be in RGBA format WRONG it will be in grayscale!!
        int bytesPerChannel = 4;
        int inputChannels = incomingImg.channels();

        LOG("incoming Mat has %d channels", inputChannels);

        int batchSize = 1;

        int cols = incomingImg.cols;
        int rows = incomingImg.rows;

        LOG("incoming Mat has %d rows, %d cols", rows, cols);

        //allocate a float array for all 3 input channels (for each channel allocate space for the entire Mat)
        std::vector<float> inputBuffer(batchSize * bytesPerChannel * cols * rows * inputChannels / 4); //div by 4 because we were doing bytes

        float mean = 128.0;
        float std = 128.0;

        //create an int array that's size of the bitmap
        //int intValues[cols * rows];
/*
        //convert the incoming image to 32-bit signed integers (combines the channels)
        incomingImg.convertTo(incomingImg, CV_32SC1);

        int cols2 = incomingImg.cols;
        int rows2 = incomingImg.rows;

        LOG("incoming Mat AFTER CONVERSION has %d rows, %d cols", rows2, cols2);

        //get pointer to the data (array of ints)
        const int* intValues = (int*) incomingImg.data;

        //get all pixels from Mat and store them in intValues
        //bitmap.getPixels(intValues, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)


        //put one float into the ByteBuffer for I'm guessing each input channel
        for (int i = 0; i < cols * rows; i++) {
          inputBuffer.push_back(((intValues[i] >> 16 & 0xFF) - mean) / std);
          inputBuffer.push_back(((intValues[i] >> 8 & 0xFF) - mean) / std);
          inputBuffer.push_back(((intValues[i] & 0xFF) - mean) / std);
        }*/

        uint8_t* in = incomingImg.data;
        int Len = rows * cols * inputChannels;

        for (int i = 0; i < Len; i++) {
            inputBuffer.push_back((in[i] - 127.5) / 127.5);
        }

        return inputBuffer;
  }


  //Returns value within [0,1].
  float Posenet::sigmoid(float x) {
    return (1.0 / (1.0 + exp(-x)));
  }


  //Initializes a 4D outputMap of 1 * x * y * z float arrays for the model processing to populate.
  std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > Posenet::initOutputMap() {
    LOG("initoutputmap called");

    //make a map from int to something (some object)
    std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > outputMap;

    int32_t out = TfLiteInterpreterGetOutputTensorCount(interpreter);

    LOG("initOutputMap(): interpreter has %d output tensors", out);

    //HEATMAP -- 1 * 9 * 9 * 17 contains heatmaps
    const TfLiteTensor* t0 = TfLiteInterpreterGetOutputTensor(interpreter, 0);

    int32_t numDims = TfLiteTensorNumDims(t0);

    //initialize int array to hold all dimens
    std::vector<int32_t> heatmapsShape;

    //iterate over the num of dimensions this tensor has, getting each one and storing it
    for (int i = 0; i < numDims; i++) {
        //get this dimension and add it to the list
        heatmapsShape.push_back(TfLiteTensorDim(t0, i));
    }

    //4D array of floats for the keypoints heatmap

    //intialize level 0
    std::vector<std::vector<std::vector<std::vector<float>>>> heatmaps(heatmapsShape[0]);

    //initialize level 1
    for (int l0 = 0; l0 < heatmapsShape[0]; l0++) {
        heatmaps[l0] = std::vector<std::vector<std::vector<float>>>(heatmapsShape[1]);

        //initialize level 2
        for (int l1 = 0; l1 < heatmapsShape[1]; l1++) {
            heatmaps[l0][l1] = std::vector<std::vector<float>>(heatmapsShape[2]);

            //initialize level 3
            for (int l2 = 0; l2 < heatmapsShape[2]; l2++) {
                heatmaps[l0][l1][l2] = std::vector<float>(heatmapsShape[3]);
            }
        }
    }

    LOG("Heatmaps shape is %d x %d x %d x %d", heatmapsShape[0], heatmapsShape[1], heatmapsShape[2], heatmapsShape[3]);

    outputMap[0] = heatmaps;



    //OFFSETS -- 1 * 9 * 9 * 34 contains offsets

    const TfLiteTensor* t1 = TfLiteInterpreterGetOutputTensor(interpreter, 1);


        numDims = TfLiteTensorNumDims(t1);

        //initialize int array to hold all dimens
        std::vector<int32_t> offsetsShape;

        //iterate over the num of dimensions this tensor has, getting each one and storing it
        for (int i = 0; i < numDims; i++) {
            //get this dimension and add it to the list
            offsetsShape.push_back(TfLiteTensorDim(t1, i));
        }

        //4D array of floats for the keypoints heatmap

        //intialize level 0
        std::vector<std::vector<std::vector<std::vector<float>>>> offsets(offsetsShape[0]);

        //initialize level 1
        for (int l0 = 0; l0 < offsetsShape[0]; l0++) {
            offsets[l0] = std::vector<std::vector<std::vector<float>>>(offsetsShape[1]);

            //initialize level 2
            for (int l1 = 0; l1 < offsetsShape[1]; l1++) {
                offsets[l0][l1] = std::vector<std::vector<float>>(offsetsShape[2]);

                //initialize level 3
                for (int l2 = 0; l2 < offsetsShape[2]; l2++) {
                    offsets[l0][l1][l2] = std::vector<float>(offsetsShape[3]);
                }
            }
        }

         LOG("Offsets shape is %d x %d x %d x %d", offsetsShape[0], offsetsShape[1], offsetsShape[2], offsetsShape[3]);

        outputMap[1] = offsets;

    //FORWARD DISPLACEMENTS -- 1 * 9 * 9 * 32 contains forward displacements
    const TfLiteTensor* t2 = TfLiteInterpreterGetOutputTensor(interpreter, 2);


            numDims = TfLiteTensorNumDims(t2);

            //initialize int array to hold all dimens
            std::vector<int32_t> displacementsFwdShape;

            //iterate over the num of dimensions this tensor has, getting each one and storing it
            for (int i = 0; i < numDims; i++) {
                //get this dimension and add it to the list
                displacementsFwdShape.push_back(TfLiteTensorDim(t2, i));
            }

            //4D array of floats for the keypoints heatmap

            //intialize level 0
            std::vector<std::vector<std::vector<std::vector<float>>>> displacementsFwd(displacementsFwdShape[0]);

            //initialize level 1
            for (int l0 = 0; l0 < displacementsFwdShape[0]; l0++) {
                displacementsFwd[l0] = std::vector<std::vector<std::vector<float>>>(displacementsFwdShape[1]);

                //initialize level 2
                for (int l1 = 0; l1 < displacementsFwdShape[1]; l1++) {
                    displacementsFwd[l0][l1] = std::vector<std::vector<float>>(displacementsFwdShape[2]);

                    //initialize level 3
                    for (int l2 = 0; l2 < displacementsFwdShape[2]; l2++) {
                        displacementsFwd[l0][l1][l2] = std::vector<float>(displacementsFwdShape[3]);
                    }
                }
            }

            LOG("Disp fwd shape is %d x %d x %d x %d", displacementsFwdShape[0], displacementsFwdShape[1], displacementsFwdShape[2], displacementsFwdShape[3]);

            outputMap[2] = displacementsFwd;



    //BACKWARD DISPLACEMENTS -- 1 * 9 * 9 * 32 contains backward displacements
    const TfLiteTensor* t3 = TfLiteInterpreterGetOutputTensor(interpreter, 3);


        numDims = TfLiteTensorNumDims(t3);

        //initialize int array to hold all dimens
        std::vector<int32_t> displacementsBwdShape;

        //iterate over the num of dimensions this tensor has, getting each one and storing it
        for (int i = 0; i < numDims; i++) {
            //get this dimension and add it to the list
            displacementsBwdShape.push_back(TfLiteTensorDim(t3, i));
        }

        //4D array of floats for the keypoints heatmap

        //initialize level 0
        std::vector<std::vector<std::vector<std::vector<float>>>> displacementsBwd(displacementsBwdShape[0]);

        //initialize level 1
        for (int l0 = 0; l0 < displacementsBwdShape[0]; l0++) {
            displacementsBwd[l0] = std::vector<std::vector<std::vector<float>>>(displacementsBwdShape[1]);

            //initialize level 2
            for (int l1 = 0; l1 < displacementsBwdShape[1]; l1++) {
                displacementsBwd[l0][l1] = std::vector<std::vector<float>>(displacementsBwdShape[2]);

                //initialize level 3
                for (int l2 = 0; l2 < displacementsBwdShape[2]; l2++) {
                    displacementsBwd[l0][l1][l2] = std::vector<float>(displacementsBwdShape[3]);
                }
            }
        }
        LOG("Disp bwd shape is %d x %d x %d x %d", displacementsBwdShape[0], displacementsBwdShape[1], displacementsBwdShape[2], displacementsBwdShape[3]);
        outputMap[3] = displacementsBwd;


    return outputMap;
  }


  void Posenet::readFlatIntoMultiDimensionalArray(float* data, std::vector<std::vector<std::vector<std::vector<float>>>> map) {
        //the map is already initialized, so we'll know what dimensions/cutoffs we're looking for

        int counter = 0;

        //topmost level (should just be one)
        for (int l0 = 0; l0 < map.size(); l0++) {
            //second level (should be 9)
            for (int l1 = 0; l1 < map[0].size(); l1++) {
                for (int l2 = 0; l2 < map[0][0].size(); l2++) {

                    //lowest level (i.e. should be 17, 32, or 34)
                    for (int l3 = 0; l3 < map[0][0][0].size(); l3++) {
                        //copy data into the 4D map
                        map[l0][l1][l2][l3] = data[counter++];
                    }
                }
            }
        }
  }


  void Posenet::runForMultipleInputsOutputs(std::vector<float> inputs
  , std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > outputs) {

        //this.inferenceDurationNanoseconds = -1L;

        //make sure we have some input data
        if (inputs.size() != 0) {
            //make sure we have output map initialized
            if (!outputs.empty()) {
                /*
                //iterate over all floats in input
                for (int i = 0; i < inputs.size(); ++i) {
                    //get each input tensor
                    Tensor tensor = TfLiteInterpreterGetInputTensor(interpreter, i);

                    int[] newShape = tensor.getInputShapeIfDifferent(inputs[i]);

                    if (newShape != null) {
                        this.resizeInput(i, newShape);
                    }
                }*/

                LOG("model has %d input tensors", TfLiteInterpreterGetInputTensorCount(interpreter));

                LOG("Inputs size is %d", inputs.size());


                TfLiteTensor* curr_input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
                if (curr_input_tensor == NULL) {
                    LOG("This input tensor came up NULL");
                    return;
                }

                //copy the input float data to the input tensor
                if (TfLiteTensorCopyFromBuffer(curr_input_tensor, inputs.data(), TfLiteTensorByteSize(curr_input_tensor)) != kTfLiteOk) {
                    LOG("TfLite copyFROMbuffer failure! Returning...");
                    return;
                }
                else {
                    LOG("TfLite copyFROMbuffer success");
                }

                //long inferenceStartNanos = System.nanoTime();

                LOG("Running inference...");
                //invoke interpreter to run the model
                if (TfLiteInterpreterInvoke(interpreter) != kTfLiteOk) {
                    LOG("TfLiteInterpreterInvoke FAILED");
                    return;
                }
                else {
                    LOG("TfLiteInterpreterInvoke SUCCESS");
                }


                //long inferenceDurationNanoseconds = System.nanoTime() - inferenceStartNanos;

                int ctr = 0;

                //check how many output tensors -- FOUR
                int32_t out_tens = TfLiteInterpreterGetOutputTensorCount(interpreter);
                LOG("We have %d output tensors", out_tens);

                LOG("Our map size is %d", outputs.size());


                //iterate over each key-value pair in the output map (should iterate 4 times)
                for (std::pair<int, std::vector<std::vector<std::vector<std::vector<float>> > > > element : outputs) {
                    LOG("Running pair iteration...");
                    //copy output tensor data into output map
                    const TfLiteTensor* curr_output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, element.first);


                    if (curr_output_tensor == NULL) {
                        LOG("This output tensor came up NULL");
                        return;
                    }
                    else {
                        LOG("This output tensor found successfully");
                    }

                    LOG("This output tensor has size %d, or %d floats", TfLiteTensorByteSize(curr_output_tensor), TfLiteTensorByteSize(curr_output_tensor)/4);


                    //get underlying data as float*
                    float* data = (float*)TfLiteTensorData(curr_output_tensor);

                    //make sure we got data successfully
                    if (data == NULL) {
                        LOG("Problem getting underlying data buffer from this output tensor");
                        return;
                    }



                    //I think we need a function similar to
                    //private static native void readMultiDimensionalArray(long var0, Object var2), since our output tensors have one FLAT data buffer and we want
                    //to copy the data into our multidimensional (4D) initialized output arrays
                    readFlatIntoMultiDimensionalArray(data, element.second);




                    ctr++;
                    LOG("Iteration #%d complete", ctr);

                }

                //this.inferenceDurationNanoseconds = inferenceDurationNanoseconds;
            }

            else {
                LOG("runForMultipleInputsOutpus: Input error: Outputs should not be null or empty.");
            }
        }

        else {
            LOG("runForMultipleInputsOutputs: Inputs should not be null or empty.");
        }
  }




 Person Posenet::estimateSinglePose(cv::Mat img, TfLiteInterpreter* pInterpreter) {
    clock_t estimationStartTimeNanos = clock();

    std::vector<float> inputArray = initInputArray(img);

    //print out how long scaling took
    //Log.i("posenet", String.format("Scaling to [-1,1] took %.2f ms", 1.0f * (SystemClock.elapsedRealtimeNanos() - estimationStartTimeNanos) / 1_000_000))

    TfLiteInterpreter* mInterpreter;

    if (interpreter == NULL) {
        mInterpreter = getInterpreter();
    }
    else {
        LOG("estimateSinglePose: already have interpreter");
        mInterpreter = interpreter;
    }

    std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > outputMap = initOutputMap();

    //get the elapsed time since system boot
    clock_t inferenceStartTimeNanos = clock();

    //from https://www.tensorflow.org/lite/guide/inference: each entry in inputArray corresponds to an input tensor and
    //outputMap maps indices of output tensors to the corresponding output data.
    runForMultipleInputsOutputs(inputArray, outputMap);

    LOG("Runformult complete");

    //get the elapsed time since system boot again, and subtract the first split we took to find how long running the model took
    clock_t lastInferenceTimeNanos = clock() - inferenceStartTimeNanos;

    //print out how long the interpreter took
    //Log.i("posenet", String.format("Interpreter took %.2f ms", 1.0f * lastInferenceTimeNanos / 1_000_000))



    //at this point the output data we need from the model is in outputMap

    std::vector<std::vector<std::vector<std::vector<float>>>> heatmaps = outputMap[0];
    std::vector<std::vector<std::vector<std::vector<float>>>> offsets = outputMap[1];

    //get dimensions of levels 1 and 2 of heatmap (should be 9 and 9)
    int height = heatmaps[0].size();
    int width = heatmaps[0][0].size();

    //get dim of level 3 of heatmap (should be 17, for 17 joints found by the model)
    int numKeypoints = heatmaps[0][0][0].size();

    //Finds the (row, col) locations of where the keypoints are most likely to be.
    std::vector<std::pair<int, int>> keypointPositions(numKeypoints, std::pair<int, int>(0, 0));


    //iterate over the number of keypoints (17?)
    for (int keypoint = 0; keypoint < numKeypoints; keypoint++) {
      float maxVal = heatmaps[0][0][0][keypoint];

      int maxRow = 0;
      int maxCol = 0;

      //iterate over every vector in our 9x9 grid of float vectors
      for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            //check the float at this joint slot at this place in our 9x9 grid
            if (heatmaps[0][row][col][keypoint] > maxVal) {
                //if this float was higher than our running max, then we accept this location as our current "most likely to hold joint" location
                maxVal = heatmaps[0][row][col][keypoint];
                maxRow = row;
                maxCol = col;
          }
        }
      }

      //add the location that was most likely to contain this joint point to our keypoint positions array
      keypointPositions[keypoint] = std::pair<int, int>(maxRow, maxCol);
    }



    //Calculating the x and y coordinates of the keypoints with offset adjustment.
    std::vector<int> xCoords(numKeypoints);

    std::vector<int> yCoords(numKeypoints);


    std::vector<float> confidenceScores(numKeypoints);


    for (int i = 0; i < numKeypoints; i++) {
        std::pair<int, int> thisKP = keypointPositions[i];
        int positionY = thisKP.first;
        int positionX = thisKP.second;

        //store the y coordinate of these keypoint in the image as calculated offset + the most likely position of this joint div by (9 * 257)
        yCoords[i] = (int)(positionY / (float)(height - 1) * img.rows + offsets[0][positionY][positionX][i]);

        //store the y coordinate of these keypoint in the image as calculated offset + the most likely position of this joint div by (9 * 257)
        xCoords[i] = (int)(positionX / (float)(width - 1) * img.cols + offsets[0][positionY][positionX][i + numKeypoints]);

        //compute arbitrary confidence value between 0 and 1
        confidenceScores[i] = sigmoid(heatmaps[0][positionY][positionX][i]);
    }

    //instantiate new person to return
    Person person = Person();

    //initialize array of KeyPoints of size 17
    std::vector<KeyPoint> keypointList(numKeypoints, KeyPoint());

    float totalScore = 0.0;

    //copy data into the keypoint list
    for (int i = 0; i < numKeypoints; i++) {
        KeyPoint thisKeypoint = keypointList[i];

        thisKeypoint.bodyPart = static_cast<BodyPart>(i);
        thisKeypoint.position.x = (float)xCoords[i];
        thisKeypoint.position.y = (float)yCoords[i];

        thisKeypoint.score = confidenceScores[i];
        totalScore += confidenceScores[i];
    }


    person.keyPoints = keypointList;

    person.score = totalScore / numKeypoints;

    return person;
  }










/*
  //FROM GITHUB

  int i, x, y, j;
         static Point Pnt[17];                       //heatmap
          static float Cnf[17];                       //confidence table
          static Point Loc[17];                       //location in image
          const float confidence_threshold = -1.0;    //confidence can be negative


          GetImageTFLite(
              //get tensor as float* from first of inputs
              interpreter->typed_tensor<float>(interpreter->inputs()[0]), src);

          //run the model
          interpreter->Invoke();

          //the model run is now complete

          //1 * 9 * 9 * 17 contains heatmaps
          const float* heatmapShape = interpreter->tensor(interpreter->outputs()[0])->data.f;

          //1 * 9 * 9 * 34 contains offsets
          const float* offsetShape = interpreter->tensor(interpreter->outputs()[1])->data.f;


          //1 * 9 * 9 * 32 contains forward displacements
          //const float* dispFwdShape = interpreter->tensor(interpreter->outputs()[2])->data.f;


          //1 * 9 * 9 * 32 contains backward displacements
          //const float* dispBckShape = interpreter->tensor(interpreter->outputs()[3])->data.f;

          //Finds the (row, col) locations of where the keypoints are most likely to be.
          for (i = 0; i < 17; i++) {
              //copy from heatmaps array to the confidence table
              Cnf[i] = heatmapShape[i];     //x = y = 0 -> j = 17 * (9 * 0 + 0) + i; -> j = i


              for (y = 0; y < 9; y++) {
                  for (x = 0; x < 9; x++) {
                      j = 17 * (9 * y + x) + i;
                      if (heatmapShape[j] > Cnf[i]) {
                          Cnf[i] = heatmapShape[j];
                          Pnt[i].x = x;
                          Pnt[i].y=y;
                      }
                  }
              }
          }

          //Calculating the x and y coordinates of the keypoints with offset adjustment.
          for(i=0;i<17;i++){
              x=Pnt[i].x; y=Pnt[i].y; j=34*(9*y+x)+i;
              Loc[i].y=(y*src.rows)/8 + offsetShape[j   ];
              Loc[i].x=(x*src.cols)/8 + offsetShape[j+17];
          }


          //DRAWING
          for (i=5;i<17;i++){
              if (Cnf[i]>confidence_threshold){
                  circle(src,Loc[i],4,Scalar( 255, 255, 0 ),FILLED);
              }
          }


          if(Cnf[ 5]>confidence_threshold){
              if(Cnf[ 6]>confidence_threshold) line(src,Loc[ 5],Loc[ 6],Scalar( 255, 255, 0 ),2);
              if(Cnf[ 7]>confidence_threshold) line(src,Loc[ 5],Loc[ 7],Scalar( 255, 255, 0 ),2);
              if(Cnf[11]>confidence_threshold) line(src,Loc[ 5],Loc[11],Scalar( 255, 255, 0 ),2);
          }
          if(Cnf[ 6]>confidence_threshold){
              if(Cnf[ 8]>confidence_threshold) line(src,Loc[ 6],Loc[ 8],Scalar( 255, 255, 0 ),2);
              if(Cnf[12]>confidence_threshold) line(src,Loc[ 6],Loc[12],Scalar( 255, 255, 0 ),2);
          }
          if(Cnf[ 7]>confidence_threshold){
              if(Cnf[ 9]>confidence_threshold) line(src,Loc[ 7],Loc[ 9],Scalar( 255, 255, 0 ),2);
          }
          if(Cnf[ 8]>confidence_threshold){
              if(Cnf[10]>confidence_threshold) line(src,Loc[ 8],Loc[10],Scalar( 255, 255, 0 ),2);
          }
          if(Cnf[11]>confidence_threshold){
              if(Cnf[12]>confidence_threshold) line(src,Loc[11],Loc[12],Scalar( 255, 255, 0 ),2);
              if(Cnf[13]>confidence_threshold) line(src,Loc[11],Loc[13],Scalar( 255, 255, 0 ),2);
          }
          if(Cnf[13]>confidence_threshold){
              if(Cnf[15]>confidence_threshold) line(src,Loc[13],Loc[15],Scalar( 255, 255, 0 ),2);
          }
          if(Cnf[14]>confidence_threshold){
              if(Cnf[12]>confidence_threshold) line(src,Loc[14],Loc[12],Scalar( 255, 255, 0 ),2);
              if(Cnf[16]>confidence_threshold) line(src,Loc[14],Loc[16],Scalar( 255, 255, 0 ),2);
          }





  */

}



