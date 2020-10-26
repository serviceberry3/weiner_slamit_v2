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
    this->filename = pFilename;
    this->device = pDevice;
    this->model = TfLiteModelCreateFromFile(pFilename);
}

//default constructor to avoid errors when we declare a new posenet in header files
Posenet::Posenet()
{}


    //set up the Tensorflow inference interpreter
    TfLiteInterpreter* Posenet::getInterpreter() {

        //get the Posenet Interpreter instance if it already exists
        if (interpreter != NULL) {
            return interpreter;
        }

        //otherwise we need to create a new interpreter

        //create interpreter options
        options = TfLiteInterpreterOptionsCreate();

        //set number of threads to use
        TfLiteInterpreterOptionsSetNumThreads(options, NUM_LITE_THREADS);

        if (this->device == Device::GPU) {
            //set up GPU delegate

            //gpuDelegate = GpuDelegate()
            //options.addDelegate(gpuDelegate)
        }
        else if (device == Device::NNAPI) {
            //what to do here?
            //Device.NNAPI -> options.setUseNNAPI(true)
        }



        //customize the gpu delegate
        const TfLiteGpuDelegateOptionsV2 gpuDelegate = {
                .is_precision_loss_allowed = false,
                .inference_preference = TFLITE_GPU_INFERENCE_PREFERENCE_FAST_SINGLE_ANSWER,
                .inference_priority1 = TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY,
                .inference_priority2 = TFLITE_GPU_INFERENCE_PRIORITY_AUTO,
                .inference_priority3 = TFLITE_GPU_INFERENCE_PRIORITY_AUTO
        }; //TfLiteGpuDelegateOptionsV2Default() doesn't fix the error either


        //TfLiteDelegate* delegate = TfLiteGpuDelegateV2Create(&gpuDelegate);

        //TfLiteDelegate* delegate = TfLiteGpuDelegateV2Default(&gpuDelegate);


        //if (interpreter->ModifyGraphWithDelegate(delegate) != kTfLiteOk) return false;


        //TfLiteInterpreterOptionsAddDelegate(options, delegate);

        //instantiate the interpreter using the model we loaded
        TfLiteInterpreter* interpreter = TfLiteInterpreterCreate(model, options);

        //allocate tensors for the interpreter
        TfLiteInterpreterAllocateTensors(interpreter);
        return interpreter;
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
        //delete the interpreter we made
        TfLiteInterpreterDelete(interpreter);
        TfLiteInterpreterOptionsDelete(options);
        TfLiteModelDelete(model);
    }

    //Scale the image pixels to a float array of [-1,1] values.
    std::vector<float> Posenet::initInputArray(cv::Mat incomingImg) { //the mat will be in RGBA format
        int bytesPerChannel = 4;
        int inputChannels = incomingImg.channels();
        LOG("incoming Mat has %d channels", inputChannels);
        int batchSize = 1;

        int cols = incomingImg.cols;
        int rows = incomingImg.rows;

        LOG("incoming Mat has %d rows, %d cols", rows, cols);

        //allocate a float array for all 3 input channels (for each channel allocate space for the entire Mat)
        std::vector<float> inputBuffer(batchSize * bytesPerChannel * cols * rows * inputChannels / 4, 0); //div by 4 because we were doing bytes

        float mean = 128.0;
        float std = 128.0;

        //create an int array that's size of the bitmap
        //int intValues[cols * rows];

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
        }

        return inputBuffer;
  }


  //Returns value within [0,1].
  float Posenet::sigmoid(float x) {
    return (1.0 / (1.0 + exp(-x)));
  }


  //Initializes a 4D outputMap of 1 * x * y * z float arrays for the model processing to populate.
  std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > Posenet::initOutputMap(
  TfLiteInterpreter* interpreter) {

    //make a map from int to something (some object)
    std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > outputMap;


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

            //intialize level 0
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

            outputMap[3] = displacementsBwd;



    return outputMap;
  }


  void Posenet::runForMultipleInputsOutputs(std::vector<float> inputs
  , std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > outputs) {
  /*
        this.inferenceDurationNanoseconds = -1L;
                if (inputs != null && inputs.length != 0) {
                    if (outputs != null && !outputs.isEmpty()) {
                        for(int i = 0; i < inputs.length; ++i) {
                            Tensor tensor = this.getInputTensor(i);
                            int[] newShape = tensor.getInputShapeIfDifferent(inputs[i]);
                            if (newShape != null) {
                                this.resizeInput(i, newShape);
                            }
                        }

                        boolean needsAllocation = !this.isMemoryAllocated;
                        if (needsAllocation) {
                            allocateTensors(this.interpreterHandle, this.errorHandle);
                            this.isMemoryAllocated = true;
                        }

                        for(int i = 0; i < inputs.length; ++i) {
                            this.getInputTensor(i).setTo(inputs[i]);
                        }

                        long inferenceStartNanos = System.nanoTime();

                        //invoke
                        run(this.interpreterHandle, this.errorHandle);


                        long inferenceDurationNanoseconds = System.nanoTime() - inferenceStartNanos;
                        if (needsAllocation) {
                            for(int i = 0; i < this.outputTensors.length; ++i) {
                                if (this.outputTensors[i] != null) {
                                    this.outputTensors[i].refreshShape();
                                }
                            }
                        }

                        Iterator var13 = outputs.entrySet().iterator();

                        while(var13.hasNext()) {
                            Entry<Integer, Object> output = (Entry)var13.next();
                            this.getOutputTensor((Integer)output.getKey()).copyTo(output.getValue());
                        }

                        this.inferenceDurationNanoseconds = inferenceDurationNanoseconds;
                    } else {
                        throw new IllegalArgumentException("Input error: Outputs should not be null or empty.");
                    }
                } else {
                    throw new IllegalArgumentException("Input error: Inputs should not be null or empty.");
                }
                */
  }




 Person Posenet::estimateSinglePose(cv::Mat img) {
    clock_t estimationStartTimeNanos = clock();

    std::vector<float> inputArray = initInputArray(img);

    //print out how long scaling took
    //Log.i("posenet", String.format("Scaling to [-1,1] took %.2f ms", 1.0f * (SystemClock.elapsedRealtimeNanos() - estimationStartTimeNanos) / 1_000_000))

    std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > outputMap = initOutputMap(getInterpreter());

    //get the elapsed time since system boot
    clock_t inferenceStartTimeNanos = clock();

    //from https://www.tensorflow.org/lite/guide/inference: each entry in inputArray corresponds to an input tensor and
    //outputMap maps indices of output tensors to the corresponding output data.
    runForMultipleInputsOutputs(inputArray, outputMap);

    //get the elapsed time since system boot again, and subtract the first split we took to find how long running the model took
    clock_t lastInferenceTimeNanos = clock() - inferenceStartTimeNanos;

    //print out how long the interpreter took
    //Log.i("posenet", String.format("Interpreter took %.2f ms", 1.0f * lastInferenceTimeNanos / 1_000_000))



    /*
    val heatmaps = outputMap[0] as Array<Array<Array<FloatArray>>>
    val offsets = outputMap[1] as Array<Array<Array<FloatArray>>>

    val height = heatmaps[0].size
    val width = heatmaps[0][0].size
    val numKeypoints = heatmaps[0][0][0].size

    // Finds the (row, col) locations of where the keypoints are most likely to be.
    val keypointPositions = Array(numKeypoints) { Pair(0, 0) }
    for (keypoint in 0 until numKeypoints) {
      var maxVal = heatmaps[0][0][0][keypoint]
      var maxRow = 0
      var maxCol = 0
      for (row in 0 until height) {
        for (col in 0 until width) {
          if (heatmaps[0][row][col][keypoint] > maxVal) {
            maxVal = heatmaps[0][row][col][keypoint]
            maxRow = row
            maxCol = col
          }
        }
      }
      keypointPositions[keypoint] = Pair(maxRow, maxCol)
    }

    // Calculating the x and y coordinates of the keypoints with offset adjustment.
    val xCoords = IntArray(numKeypoints)
    val yCoords = IntArray(numKeypoints)
    val confidenceScores = FloatArray(numKeypoints)
    keypointPositions.forEachIndexed { idx, position ->
      val positionY = keypointPositions[idx].first
      val positionX = keypointPositions[idx].second

      yCoords[idx] = (position.first / (height - 1).toFloat() * bitmap.height + offsets[0][positionY][positionX][idx]).toInt()

      xCoords[idx] = (position.second / (width - 1).toFloat() * bitmap.width + offsets[0][positionY][positionX][idx + numKeypoints]).toInt()

      confidenceScores[idx] = sigmoid(heatmaps[0][positionY][positionX][idx])
    }

    Person person = Person();

    val keypointList = Array(numKeypoints) { KeyPoint() }
    var totalScore = 0.0f

    enumValues<BodyPart>().forEachIndexed { idx, it ->
      keypointList[idx].bodyPart = it
      keypointList[idx].position.x = xCoords[idx].toFloat();
      keypointList[idx].position.y = yCoords[idx].toFloat();
      keypointList[idx].score = confidenceScores[idx]
      totalScore += confidenceScores[idx]
    }

    person.keyPoints = keypointList.toList()
    person.score = totalScore / numKeypoints

    return person;*/
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



