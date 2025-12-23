#include "xcl2.hpp"
#include "event_timer.hpp"
#include <vector>

// DATA_SIZE should be multiple of 16 as Kernel Code is using int16 vector
// datatype
// to read the operands from Global Memory. So every read/write to global memory
// will read 16 integers value.

#define HEIGHT 128
#define WIDTH 128
#define DATA_SIZE 16384 // HEIGHT*WIDTH
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

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    EventTimer et;

    std::string binaryFile = argv[1];

    et.add("Allocate Memory in Host Memory");
    // Allocate Memory in Host Memory
    size_t vector_size_bytes = sizeof(int) * DATA_SIZE;
    std::vector<unsigned int, aligned_allocator<unsigned int>> source_A(DATA_SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>> source_B(DATA_SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>> source_hw_C(DATA_SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>> source_hw_C_filt(DATA_SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>> source_sw_C(DATA_SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>> source_sw_C_filt(DATA_SIZE);
    et.finish();

    // Test Data Creation
    et.add("Fill the buffers");
    // Seed for reproducible results
    srand(3);
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            source_A[i*WIDTH+j] = rand() % 256;
            source_B[i*WIDTH+j] = rand() % 256;
        }
    }
    et.finish();

    et.add("Software VADD run");
    // Software Result
    for (int i = 0; i < DATA_SIZE; i++) {
        source_sw_results[i] = source_in1[i] + source_in2[i];
    }
    et.finish();

    et.add("OpenCL host code");
    // OPENCL HOST CODE AREA START
    cl_int err;
    cl::CommandQueue q;
    cl::Context context;
    cl::Kernel krnl_vector_add;
    auto devices = xcl::get_xil_devices();
    et.finish();

    et.add("Load Binary File to Alveo U200");
    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, krnl_imageDiffPosterize = cl::Kernel(program, "imageDiffPosterize", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }
    et.finish();

    et.add("Allocate Buffer in Global Memory");
    // Allocate Buffer in Global Memory
    OCL_CHECK(err, cl::Buffer buffer_A(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
    source_A.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_B(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes,
    source_B.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_C(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, vector_size_bytes, source_hw_C.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_C_filt(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, vector_size_bytes, source_hw_C_filt.data(), &err));
    et.finish();

    et.add("Set the Kernel Arguments");
    int size = DATA_SIZE;
    // Set the Kernel Arguments
    int nargs = 0;
    OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(nargs++, buffer_A));
    OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(nargs++, buffer_B));
    OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(nargs++, buffer_C));
    OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(nargs++, buffer_C_filt));
    OCL_CHECK(err, err = krnl_imageDiffPosterize.setArg(nargs++, size));
    et.finish();

    et.add("Copy input data to device global memory");
    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_A, buffer_B}, 0 /* 0 means from host*/));
    et.finish();

    et.add("Launch the Kernel");
    // Launch the Kernel
    OCL_CHECK(err, err = q.enqueueTask(krnl_imageDiffPosterize));
    et.finish();

    et.add("Copy Result from Device Global Memory to Host Local Memory");
    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_C, buffer_C_filt}, CL_MIGRATE_MEM_OBJECT_HOST));
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
