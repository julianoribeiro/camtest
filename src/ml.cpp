#include "Arduino.h"
#include "SPIFFS.h"

// #include "img_converters.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/recording_micro_interpreter.h"

#include "io.h"
#include "ml.h"

void print_available_memory() {
  printf("\nMemory Heap: (%d/%d) and PSRAM: (%d/%d) and SPIFFS: (%d/%d)\n\n", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getFreePsram(), ESP.getPsramSize(), SPIFFS.usedBytes(), SPIFFS.totalBytes());
}

void preprocessImageData(uint8_t* image_data, int image_size, float* final_image_data) {
    // Preprocess the image (normalize the pixel values to the range [0, 1])
    for (int i = 0; i < image_size; i++) {
        final_image_data[i] = image_data[i] / 255.0f;
    }

    // printf("\nImage Float:\n");
    // for (int i = 1; i <= 784; i++) {
    //   printf("%f ", final_image_data[i - 1]);
    // }
    // printf("\n");

    // printf("Converted to float!\n");
}

tflite::MicroMutableOpResolver<10>* tinyMlResolvers(tflite::MicroMutableOpResolver<10>* resolver) {
    // List Resolvers
    resolver = new tflite::MicroMutableOpResolver<10>();
    resolver->AddReshape();
    resolver->AddConv2D();
    resolver->AddRelu();
    resolver->AddMaxPool2D();
    resolver->AddFullyConnected();
    resolver->AddSoftmax();
    // Add more operators as needed for your model

    return resolver;
}

tflite::MicroMutableOpResolver<10>* leNetResolvers(tflite::MicroMutableOpResolver<10>* resolver) {
    // List Resolvers
    resolver = new tflite::MicroMutableOpResolver<10>();
    resolver->AddReshape();
    resolver->AddConv2D();
    resolver->AddRelu();
    resolver->AddAveragePool2D();
    resolver->AddFullyConnected();
    resolver->AddSoftmax();
    // Add more operators as needed for your model

    return resolver;
}

tflite::MicroMutableOpResolver<10>* squeezeResolvers(tflite::MicroMutableOpResolver<10>* resolver) {
    // List Resolvers
    resolver = new tflite::MicroMutableOpResolver<10>();
    resolver->AddReshape();
    resolver->AddConv2D();
    resolver->AddRelu();
    resolver->AddMaxPool2D();
    resolver->AddAveragePool2D();
    resolver->AddFullyConnected();
    resolver->AddSoftmax();
    resolver->AddConcatenation();
    resolver->AddMean();
    // Add more operators as needed for your model

    return resolver;
}

tflite::MicroMutableOpResolver<10>* mobileNetResolvers(tflite::MicroMutableOpResolver<10>* resolver) {
    // List Resolvers
    resolver = new tflite::MicroMutableOpResolver<10>();
    resolver->AddReshape();
    resolver->AddConv2D();
    resolver->AddDepthwiseConv2D();
    resolver->AddRelu();
    resolver->AddAveragePool2D();
    resolver->AddMaxPool2D();
    resolver->AddFullyConnected();
    resolver->AddSoftmax();
    // Add more operators as needed for your model

    return resolver;
}

tflite::MicroMutableOpResolver<10>* vgg16Resolvers(tflite::MicroMutableOpResolver<10>* resolver) {
    // List Resolvers
    resolver = new tflite::MicroMutableOpResolver<10>();
    resolver->AddReshape();
    resolver->AddConv2D();
    resolver->AddDepthwiseConv2D();
    resolver->AddRelu();
    resolver->AddAveragePool2D();
    resolver->AddMaxPool2D();
    resolver->AddFullyConnected();
    resolver->AddSoftmax();
    resolver->AddMul();
    resolver->AddAdd();
    // Add more operators as needed for your model

    return resolver;
}

tflite::MicroInterpreter* initializeInterpreter(char* model_file, const tflite::Model* model, tflite::MicroMutableOpResolver<10>* resolver, int tensor_arena_size, uint8_t* tensor_arena) {
    printf("Initializing Interpreter...");
    
    int model_size = file_size(model_file);
    uint8_t* model_data = (uint8_t*)malloc(model_size);
    read_file_data(model_file, model_size, model_data);
    printf("Model...");

    // const tflite::Model* model = NULL;
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("\nModel provided is schema version %d not equal to supported version %d.", model->version(), TFLITE_SCHEMA_VERSION);
        return NULL;
    }
    printf("Loaded...");

    resolver = vgg16Resolvers(resolver);
    printf("Resolvers...");

    tensor_arena = (uint8_t*)malloc(tensor_arena_size);
    if (tensor_arena == NULL) {
        printf("\nUnable to allocate memory\n");
        return NULL;
    }

    // Create a TensorFlow Lite interpreter
    tflite::MicroInterpreter* interpreter = new tflite::MicroInterpreter(model, *resolver, tensor_arena, tensor_arena_size);
    if (interpreter == NULL) {
        printf("\nUnable to create the interpreter\n");
        return NULL;
    }
    printf("Interpreter...");

    // Allocate tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        printf("\nUnable to allocate tensors\n");
        return NULL;
    }
    printf("OK!\n");

    free(model_data);

    return interpreter;
}

int findMaxIndex(float predictions[]) {
    int maxIndex = 0;
    float maxValue = predictions[0];

    for (int i = 1; i < 10; i++) {
        if (predictions[i] > maxValue) {
            maxValue = predictions[i];
            maxIndex = i;
        }
    }

    return maxIndex;
}

int predict(tflite::MicroInterpreter* interpreter, float* image_data, int image_size) {
    if (kTfLiteOk != interpreter->initialization_status()) {
        printf("Interpreter not OK\n");
        return -1;
    }

    // Obtain pointers to the model's input and output tensors.
    TfLiteTensor* input = interpreter->input(0);
    TfLiteTensor* output = interpreter->output(0);

    // printf("Input Type: %i\n", input->type);
    // printf("Input Bytes: %i\n", input->bytes);
    // printf("Input Quant Type: %i\n", input->quantization.type);
    // printf("Input Dims Size: %i\n", input->dims->size);
    // printf("Input Dims Data 0: %i\n", input->dims->data[0]);
    // printf("Input Dims Data 1: %i\n", input->dims->data[1]);
    // printf("Input Dims Data 2: %i\n", input->dims->data[2]);
    // printf("Input Dims Data 3: %i\n", input->dims->data[3]);

    // Run inference
    memcpy(input->data.f, image_data, image_size * sizeof(float));

    if (kTfLiteOk != interpreter->Invoke()) {
        printf("\nUnable to invoke\n");
    }

    // printf("Output Type: %i\n", output->type);
    // printf("Output Bytes: %i\n", output->bytes);
    // printf("Output Quant Type: %i\n", output->quantization.type);    
    // printf("Output Dims Size: %i\n", output->dims->size);
    // printf("Output Data 0: %i\n", output->dims->data[0]);
    // printf("Output Data 1: %i\n", output->dims->data[1]);

    int prediction = findMaxIndex(output->data.f);

    // Print the predicted class
    // printf("\nOutput:   0      1      2      3      4      5      6      7      8      9\nChance:");
    // for (int i = 0; i < 10; i++) {
    //     printf(" %.3f ", output->data.f[i]);
    // }
    // printf("\nPrediction: %d", prediction);
    // printf("\n\n");

    return prediction;
}

void include_prediction_in_confusion_matrix(int* confusion_matrix, char* file_name, int prediction) {
  char row_char = file_name[0];
  int row = row_char - '0';
  int index = row * 10 + prediction;
  confusion_matrix[index]++;
}

void print_confusion_matrix(int* confusion_matrix) {
  printf("\nDigit:,   0,     1,     2,     3,     4,     5,     6,     7,     8,     9,\n");
  for (int row = 0; row < 10; row++) {
    printf("  %d  :,", row);
    for (int column = 0; column < 10; column++) {
      int index = row * 10 + column;
      printf(" %05d,", confusion_matrix[index]);
    }
    printf("\n");
  }
}