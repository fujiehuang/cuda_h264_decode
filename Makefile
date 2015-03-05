# compilers
CUDA=nvcc
STD=g++

#filenames
MAIN=main.cu
OUTPUT=out
OBJECTS=V4L2stream.o BitPos.o H264parser.o CUVIDdecoder.o cuda.o

#headers
INC=-I/usr/local/cuda-6.5/include

#libraries
CUDA_LIBS=-lnvcuvid -lcudart
LIB_PATHS=-L/usr/lib/nvidia-340

#flags
FLAGS=-std=gnu++11
CUFLAGS=-std=c++11

#targets
all: classes main
	@# runs classes and main targets in sequence
	@# default behavior is to compile the entire program

# compile only main.cpp, then link preexisting object files from other sources
main: $(MAIN)
	$(CUDA) $(MAIN) $(OBJECTS) -o $(OUTPUT) $(INC) $(LIB_PATHS) $(CUDA_LIBS) $(CUFLAGS)

# generate object files from all source files sans main.cpp
classes: v4l2 bitpos parser cuvid cuda
	@# will automatically make targets

v4l2: V4L2stream.cpp
	$(STD) V4L2stream.cpp -c $(INC) $(FLAGS)

bitpos: BitPos.cpp
	$(STD) BitPos.cpp -c $(INC) $(FLAGS)

parser: H264parser.cpp
	$(STD) H264parser.cpp -c $(INC) $(FLAGS)

cuvid: CUVIDdecoder.cpp
	$(CUDA) CUVIDdecoder.cpp -c $(INC) $(LIB_PATHS) $(CUDA_LIBS) $(CUFLAGS)

cuda: cuda.cu
	$(CUDA) cuda.cu -c -lcudart $(CUFLAGS)

# run target is for convenience, executes the program
run: $(OUTPUT)
	./$(OUTPUT)

# remove the executable and all object files (useful for svc, e.g. git commits)
clean:
	rm $(OUTPUT) $(OBJECTS)
