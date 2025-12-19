#include "xcl2.hpp"
#include "event_timer.hpp"
#include <algorithm>
#include <vector>
#include <cstdlib> // For rand() and srand()
#include <cstdio>  // For printf()

#define HEIGHT 64
#define WIDTH 64
#define DATA_SIZE HEIGHT*WIDTH
#define BUFFER_SIZE 14
#define T1 32
#define T2 96

int clipper(int element){
    if (element>255){
        element=255;
    }
    else if (element<0){
        element=0;
    }
    return element;
}
// Kernel Function - HW
void imageDiffPosterize(unsigned int* A, unsigned int* B, unsigned int* C, unsigned int* C_filt, int size);

// SW
void sw_ref(unsigned int *A, unsigned int *B, unsigned int *C_SW, unsigned int *C_SW_filt){

    unsigned long int D;
    unsigned int S[(WIDTH-2)*(HEIGHT-2)]={0};
    int F[3][3] = {0, -1, 0, -1, 5, -1, 0, -1, 0};

    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            D = A[i*WIDTH + j] - B[i*WIDTH + j];
            if(D<0) D=-D;
            if(D<T1){
                C_SW[i*WIDTH + j]=0;
            } else if(D<T2){
                C_SW[i*WIDTH + j]=128;
            } else {
                C_SW[i*WIDTH + j]=255;
            }
        }
    }

    for (int i = 1; i <= HEIGHT - 2; i++) {
        for (int j = 1; j <= WIDTH - 2; j++) {
            for (int a = -1; a <= 1; a++) {
                for (int b = -1; b <= 1; b++) {
                    S[(i - 1)*(WIDTH-2)+(j - 1)] += F[a + 1][b + 1] * C_SW[(i + a)*WIDTH+(j + b)];
                }
            }
        }
    }

    // Clipping
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if(i>=1 && i<=HEIGHT-2 && j>=1 && j<=WIDTH-2){
                C_SW_filt[i*WIDTH+j]=clipper(S[(i-1)*(WIDTH-2)+(j-1)]);
            }else{
                C_SW_filt[i*WIDTH+j]=clipper(C_SW[i*WIDTH+j]);
            }
        }
    }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
    return EXIT_FAILURE;
  }

  EventTimer et;

  std::string binaryFile = argv[1];
  size_t vector_size_bytes = sizeof(unsigned int) * DATA_SIZE; // Changed to unsigned int
  cl_int err;
  cl::Context context;
  cl::Kernel krnl_imageDiffPosterize; // Renamed kernel variable
  cl::CommandQueue q;

  // Modified Host Memory Allocation
  et.add("Allocate Memory in Host Memory");
  std::vector<unsigned int, aligned_allocator<unsigned int>> source_A(DATA_SIZE); // Changed type
  std::vector<unsigned int, aligned_allocator<unsigned int>> source_B(DATA_SIZE); // Changed type
  std::vector<unsigned int, aligned_allocator<unsigned int>> source_hw_C(DATA_SIZE); // Changed type
  std::vector<unsigned int, aligned_allocator<unsigned int>> source_hw_C_filt(DATA_SIZE); // Changed type
  std::vector<unsigned int, aligned_allocator<unsigned int>> source_sw_C(DATA_SIZE); // Changed type
  std::vector<unsigned int, aligned_allocator<unsigned int>> source_sw_C_filt(DATA_SIZE); // Changed type
  et.finish();

  // Test Data Creation
  et.add("Fill the buffers");
  // Seed for reproducible results (as per original code)
  srand(3);
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
        source_A[i*WIDTH+j] = rand() % 256;
        source_B[i*WIDTH+j] = rand() % 256;
    }
  }
  // Initialize output vectors to zero
  std::fill(source_hw_C.begin(), source_hw_C.end(), 0);
  std::fill(source_hw_C_filt.begin(), source_hw_C_filt.end(), 0);
  std::fill(source_sw_C.begin(), source_sw_C.end(), 0);
  std::fill(source_sw_C_filt.begin(), source_sw_C_filt.end(), 0);
  et.finish();

  // Run Software Reference
  et.add("Run Software Reference");
  sw_ref(source_A.data(), source_B.data(), source_sw_C.data(), source_sw_C_filt.data());
  et.finish();

  // OPENCL HOST CODE AREA START
  // get_xil_devices() is a utility API which will find the xilinx
  // platforms and will return list of devices connected to Xilinx platform
  auto devices = xcl::get_xil_devices();
  // read_binary_file() is a utility API which will load the binaryFile
  // and will return the pointer to file buffer.
  et.add("Load Binary File to Alveo U200");
  auto fileBuf = xcl::read_binary_file(binaryFile);
  cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
  int valid_device = 0;
  for (unsigned int i = 0; i < devices.size(); i++) {
    auto device = devices[i];
    // Creating Context and Command Queue for selected Device
    OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, q = cl::CommandQueue(context, device,
                                        CL_QUEUE_PROFILING_ENABLE, &err));
    std::cout << "Trying to program device[" << i
              << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
    cl::Program program(context, {device}, bins, NULL, &err);
    if (err != CL_SUCCESS) {
      std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
    } else {
      std::cout << "Device[" << i << "]: program successful!\n";
      OCL_CHECK(err, krnl_imageDiffPosterize = cl::Kernel(program, "imageDiffPosterize", &err));
      valid_device++;
      break; // we break because we found a valid device
    }
  }
  if (valid_device == 0) {
    std::cout << "Failed to program any device found, exit!\n";
    exit(EXIT_FAILURE);
  }
  et.finish();

  // Allocate Buffer in Global Memory
  et.add("Allocate Buffer in Global Memory");
  OCL_CHECK(err, cl::Buffer buffer_A(
                     context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                     vector_size_bytes, source_A.data(), &err));
  OCL_CHECK(err, cl::Buffer buffer_B(
                     context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                     vector_size_bytes, source_B.data(), &err));
  OCL_CHECK(err, cl::Buffer buffer_C(
                     context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                     vector_size_bytes, source_hw_C.data(), &err));
  OCL_CHECK(err, cl::Buffer buffer_C_filt(
                     context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                     vector_size_bytes, source_hw_C_filt.data(), &err));
  et.finish();

  et.add("Set the Kernel Arguments");
  int size = DATA_SIZE;
  OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(0, buffer_A)); // Adjust argument indices as needed
  OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(1, buffer_B));
  OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(2, buffer_C));
  OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(3, buffer_C_filt));
  OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(4, size)); // Adjust argument index as needed
  et.finish();

  // Copy input data to device global memory
  et.add("Copy input data to device global memory");
  OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_A, buffer_B}, 0 /* 0 means from host*/));
  et.finish();

  // Launch the Kernel
  // For HLS kernels global and local size is always (1,1,1). So, it is
  // recommended
  // to always use enqueueTask() for invoking HLS kernel
  et.add("Launch the Kernel");
  OCL_CHECK(err, err = q.enqueueTask(krnl_imageDiffPosterize)); // Renamed kernel variable
  et.finish();

  // Copy Result from Device Global Memory to Host Local Memory
  et.add("Copy Result from Device Global Memory to Host Local Memory");
  OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_C, buffer_C_filt}, CL_MIGRATE_MEM_OBJECT_HOST)); // Copy both outputs
  OCL_CHECK(err, err = q.finish());
  et.finish();
  // OPENCL HOST CODE AREA END

  // Compare the results of the Device to the simulation
  et.add("Compare the results of the Device to the simulation");
  int errors = 0;
  for (int i = 0; i < HEIGHT; i++) {
      for (int j = 0; j < WIDTH; j++) {
          if (source_hw_C_filt[i*WIDTH+j] != source_sw_C_filt[i*WIDTH+j]) {
              errors++;
              // Print the first 5 mismatches for debugging
              if(errors <= 5) {
                  std::cout << "Error: Result mismatch at [" << i << "][" << j << "]" << std::endl;
                  std::cout << "CPU filt = " << source_sw_C_filt[i*WIDTH+j]
                            << ", Device filt = " << source_hw_C_filt[i*WIDTH+j] << std::endl;
              }
          }
      }
  }
  bool match = (errors == 0);
  et.finish();

  std::cout <<"----------------- Key execution times -----------------" << std::endl;
  et.print();

  // Report Result
  if(errors >= 1){
      std::cout << "There are " << errors << " different pixels." << std::endl;
      std::cout << "TEST FAILED" << std::endl;
  } else {
      std::cout << "TEST PASSED" << std::endl;
  }

  return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
