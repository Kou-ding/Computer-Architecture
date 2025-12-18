#include "xcl2.hpp"
#include "event_timer.hpp"
#include <algorithm>
#include <vector>
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
  size_t vector_size_bytes = sizeof(int) * DATA_SIZE;
  cl_int err;
  cl::Context context;
  cl::Kernel krnl_vector_add;
  cl::CommandQueue q;
  // Allocate Memory in Host Memory
  // When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the
  // hood user ptr
  // is used if it is properly aligned. when not aligned, runtime had no choice
  // but to create
  // its own host side buffer. So it is recommended to use this allocator if
  // user wish to
  // create buffer using CL_MEM_USE_HOST_PTR to align user buffer to page
  // boundary. It will
  // ensure that user buffer is used when user create Buffer/Mem object with
  // CL_MEM_USE_HOST_PTR
  et.add("Allocate Memory in Host Memory");
  std::vector<unsigned int, aligned_allocator<int>> source_A(DATA_SIZE);
  std::vector<unsigned int, aligned_allocator<int>> source_B(DATA_SIZE);
  std::vector<unsigned int, aligned_allocator<int>> source_C(DATA_SIZE);
  std::vector<unsigned int, aligned_allocator<int>> source_C_filt(DATA_SIZE);
  std::vector<unsigned int, aligned_allocator<int>> source_C_SW(DATA_SIZE);
  std::vector<unsigned int, aligned_allocator<int>> source_C_SW_filt(DATA_SIZE);
  et.finish();

  // Create the test data
  et.add("Fill the buffers");
  std::generate(source_A.begin(), source_A.end(), std::rand);
  std::generate(source_B.begin(), source_A.end(), std::rand);

  // Run SW version of imageDiffPosterize
  sw_ref(source_A.data(), source_B.data(), source_C_SW.data(), source_C_SW_filt.data());

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
	  OCL_CHECK(err, krnl_vector_add = cl::Kernel(program, "vadd", &err));
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
  // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
  // Device-to-host communication
  et.add("Allocate Buffer in Global Memory");
	OCL_CHECK(err, cl::Buffer buffer_A(
					   context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
					   vector_size_bytes, source_A.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_B(
					   context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
					   vector_size_bytes, source_B.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_C(
					   context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
					   vector_size_bytes, source_C.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_C_filt(
					   context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
					   vector_size_bytes, source_C_filt.data(), &err));
  et.finish();

  et.add("Set the Kernel Arguments");
  int size = DATA_SIZE;
  OCL_CHECK(err, err = krnl_vector_add.setArg(0, buffer_A)); // A
  OCL_CHECK(err, err = krnl_vector_add.setArg(1, buffer_B)); // B
  OCL_CHECK(err, err = krnl_vector_add.setArg(2, buffer_C)); // C
  OCL_CHECK(err, err = krnl_vector_add.setArg(3, buffer_C_filt)); // C_filt
  OCL_CHECK(err, err = krnl_vector_add.setArg(4, size)); // size
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
  OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));
  et.finish();

  // Copy Result from Device Global Memory to Host Local Memory
  et.add("Copy Result from Device Global Memory to Host Local Memory");
  OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_C_filt}, CL_MIGRATE_MEM_OBJECT_HOST));
  OCL_CHECK(err, err = q.finish());
  et.finish();
  // OPENCL HOST CODE AREA END

  // Compare the results of the Device to the simulation
  int errors = 0;
	et.add("Compare the results of the Device to the simulation");
	bool match = true;
	// Calculate SW and HW differences
	for(int i=0;i<HEIGHT;i++){
	  for(int j=0;j<WIDTH;j++){
		  if(source_C_SW_filt[i*WIDTH+j]!=source_C_filt[i*WIDTH+j]){
			  errors++;
		  }
	  }
	}
	if (errors>=1) {
	  std::cout << "Error: There are" << errors << "different pixels.\n" << std::endl;
	  match = false;
	}else{
		std::cout << "Test passed!\n" << std::endl;
	}

  et.finish();

  std::cout <<"----------------- Key execution times -----------------" << std::endl;
  et.print();

  std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
  return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
