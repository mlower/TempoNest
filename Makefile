FC = gfortran
CC = gcc
CXX = g++
CU = nvcc
CUFLAGS += -arch sm_13 -Xcompiler -O3
FFLAGS += -O3 
CFLAGS += -O3 -DHAVE_CONFIG_H  

LAPACKLIB =  -llapack

LIBPATH64=-L${CULA_LIB_PATH_64}
CULALIB = -lcula_core -lcula_lapack -lcula_lapack_fortran -lcublas -lcudart -lcuda
CULAINC = -I/apgpu04/data/ltl21/Code/cula/include 

NESTLIBDIR = /import/apgpu04/ltl21/Code/TempoFit/TempoNestRelease/MultiNest
TEMPO2LIB = /data/ltl21/Code/Tempo2Fit/Tempo2-2013.3.1Install/lib
TEMPO2INC = /data/ltl21/Code/Tempo2Fit/Tempo2-2013.3.1Install/include
TEMPO2SRC = /data/ltl21/Code/Tempo2Fit/Tempo2-3.1/tempo2-2013.3.1

LIBS =  -L$(NESTLIBDIR) -lnest3 $(LAPACKLIB)  $(LIBPATH64) $(CULALIB) -L$(TEMPO2LIB) -lsofa -ltempo2 -lgsl -lgslcblas

OBJFILES = TempoNestGPUFuncs.o dpotrs.o dgesvd.o dgemm.o dgemv.o dpotri.o dpotrf.o TempoNestParams.o MultiNestParams.o TempoNestTextOutput.o TempoNestUtilities.o TempoNestUpdateLinear.o TempoNestFindMaxGPU.o TempoNestSim.o TempoNestLikeFuncs.o TempoNestGPULikeFuncs.o TempoNestLinearLikeFuncs.o TempoNestGPULinearLikeFuncs.o TempoNestGPU.o

all: TempoNest 

%.o: %.cu
	$(CU) $(CUFLAGS) $(CULAINC) -c $*.cu	

%.o: %.c
	$(CXX) $(CFLAGS) -I$(NESTLIBDIR) $(CULAINC) -I$(TEMPO2INC) -I$(TEMPO2SRC) -c $*.c

 
TempoNest : $(OBJFILES)
	$(FC) -o TempoNest  $(OBJFILES) \
	$(FFLAGS) $(LIBS)

clean:
	rm -f *.o  TempoNest
