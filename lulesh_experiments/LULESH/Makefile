#default build suggestion of MPI + OPENMP with gcc on Livermore machines you might have to change the compiler name

SHELL = /bin/sh
.SUFFIXES: .cc .o

LULESH_EXEC_BASE = lulesh2.0
FP32_EXEC = $(LULESH_EXEC_BASE)_fp32
FP64_EXEC = $(LULESH_EXEC_BASE)_fp64
MIXED_EXEC = $(LULESH_EXEC_BASE)_mixed
MIXED_CLOSED_EXEC = $(LULESH_EXEC_BASE)_mixed_closed
MIXED_TIMESTEP_EXEC = $(LULESH_EXEC_BASE)_mixed_timestep
MIXED_HYDRO_EXEC = $(LULESH_EXEC_BASE)_mixed_hydro
MIXED_THERMO_EXEC = $(LULESH_EXEC_BASE)_mixed_thermo
MIXED_VISCOSITY_EXEC = $(LULESH_EXEC_BASE)_mixed_viscosity
MIXED_VOLUME_EXEC = $(LULESH_EXEC_BASE)_mixed_volume
MIXED_HYDRO_VOLUME_EXEC = $(LULESH_EXEC_BASE)_mixed_hydro_volume
MIXED_REFERENCE_EXEC = $(LULESH_EXEC_BASE)_mixed_reference
MIXED_PARAMETERS_EXEC = $(LULESH_EXEC_BASE)_mixed_parameters
FPCHECKER_EXEC = $(LULESH_EXEC_BASE)_fpchecker

MPI_INC = /opt/local/include/openmpi
MPI_LIB = /opt/local/lib

SERCXX = /opt/miniconda3/envs/tutorial_env/bin/clang++ -DUSE_MPI=0
MPICXX = mpig++ -DUSE_MPI=1
#CXX = $(MPICXX)
CXX = $(SERCXX)

FPCHECKER_FLAGS = -g -fno-vectorize -fno-slp-vectorize -include \
/Users/lagunaperalt1/IGNACIO/FPChecker/build/install/src/Runtime_error.h \
-fpass-plugin=/Users/lagunaperalt1/IGNACIO/FPChecker/build/install/lib/libfpchecker_error.dylib

SOURCES2.0 = \
	lulesh.cc \
	lulesh-comm.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc
FP32_OBJECTS = $(SOURCES2.0:.cc=.fp32.o)
FP64_OBJECTS = $(SOURCES2.0:.cc=.fp64.o)
MIXED_OBJECTS = $(SOURCES2.0:.cc=.mixed.o)
MIXED_CLOSED_OBJECTS = $(SOURCES2.0:.cc=.mixed_closed.o)
MIXED_TIMESTEP_OBJECTS = $(SOURCES2.0:.cc=.mixed_timestep.o)
MIXED_HYDRO_OBJECTS = $(SOURCES2.0:.cc=.mixed_hydro.o)
MIXED_THERMO_OBJECTS = $(SOURCES2.0:.cc=.mixed_thermo.o)
MIXED_VISCOSITY_OBJECTS = $(SOURCES2.0:.cc=.mixed_viscosity.o)
MIXED_VOLUME_OBJECTS = $(SOURCES2.0:.cc=.mixed_volume.o)
MIXED_HYDRO_VOLUME_OBJECTS = $(SOURCES2.0:.cc=.mixed_hydro_volume.o)
MIXED_REFERENCE_OBJECTS = $(SOURCES2.0:.cc=.mixed_reference.o)
MIXED_PARAMETERS_OBJECTS = $(SOURCES2.0:.cc=.mixed_parameters.o)
FPCHECKER_OBJECTS = $(SOURCES2.0:.cc=.fpchecker.o)

#Default build suggestions with OpenMP for g++
BASE_CXXFLAGS = -g -O2 -I. -Wall
LDFLAGS = -g -O2

#Below are reasonable default flags for a serial build
#CXXFLAGS = -g -O3 -I. -Wall
#LDFLAGS = -g -O3 

#common places you might find silo on the Livermore machines.
#SILO_INCDIR = /opt/local/include
#SILO_LIBDIR = /opt/local/lib
#SILO_INCDIR = ./silo/4.9/1.8.10.1/include
#SILO_LIBDIR = ./silo/4.9/1.8.10.1/lib

#If you do not have silo and visit you can get them at:
#silo:  https://wci.llnl.gov/codes/silo/downloads.html
#visit: https://wci.llnl.gov/codes/visit/download.html

#below is and example of how to make with silo, hdf5 to get vizulization by default all this is turned off.  All paths are Livermore specific.
#CXXFLAGS = -g -DVIZ_MESH -I${SILO_INCDIR} -Wall -Wno-pragmas
#LDFLAGS = -g -L${SILO_LIBDIR} -Wl,-rpath -Wl,${SILO_LIBDIR} -lsiloh5 -lhdf5


%.fp32.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -o $@ $<

%.fp64.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP64 -o $@ $<

%.mixed.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED -o $@ $<

%.mixed_closed.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_CLOSED -o $@ $<

%.mixed_timestep.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_TIMESTEP -o $@ $<

%.mixed_hydro.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_HYDRO -o $@ $<

%.mixed_thermo.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_THERMO -o $@ $<

%.mixed_viscosity.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_VISCOSITY -o $@ $<

%.mixed_volume.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_VOLUME -o $@ $<

%.mixed_hydro_volume.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_HYDRO -DLULESH_MIXED_VOLUME -o $@ $<

%.mixed_reference.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_REFERENCE -o $@ $<

%.mixed_parameters.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 -DLULESH_MIXED_PARAMETERS -o $@ $<

%.fpchecker.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(BASE_CXXFLAGS) -DLULESH_REAL_IS_FP32 $(FPCHECKER_FLAGS) -o $@ $<

.PHONY: all fp32 fp64 mixed mixed_closed mixed_timestep mixed_hydro mixed_thermo mixed_viscosity mixed_volume mixed_hydro_volume mixed_reference mixed_parameters fpchecker clean tar

all: fp32

fp32: $(FP32_EXEC)

fp64: $(FP64_EXEC)

mixed: $(MIXED_EXEC)

mixed_closed: $(MIXED_CLOSED_EXEC)

mixed_timestep: $(MIXED_TIMESTEP_EXEC)

mixed_hydro: $(MIXED_HYDRO_EXEC)

mixed_thermo: $(MIXED_THERMO_EXEC)

mixed_viscosity: $(MIXED_VISCOSITY_EXEC)

mixed_volume: $(MIXED_VOLUME_EXEC)

mixed_hydro_volume: $(MIXED_HYDRO_VOLUME_EXEC)

mixed_reference: $(MIXED_REFERENCE_EXEC)

mixed_parameters: $(MIXED_PARAMETERS_EXEC)

fpchecker: $(FPCHECKER_EXEC)

$(FP32_EXEC): $(FP32_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(FP64_EXEC): $(FP64_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_EXEC): $(MIXED_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_CLOSED_EXEC): $(MIXED_CLOSED_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_TIMESTEP_EXEC): $(MIXED_TIMESTEP_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_HYDRO_EXEC): $(MIXED_HYDRO_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_THERMO_EXEC): $(MIXED_THERMO_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_VISCOSITY_EXEC): $(MIXED_VISCOSITY_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_VOLUME_EXEC): $(MIXED_VOLUME_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_HYDRO_VOLUME_EXEC): $(MIXED_HYDRO_VOLUME_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_REFERENCE_EXEC): $(MIXED_REFERENCE_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(MIXED_PARAMETERS_EXEC): $(MIXED_PARAMETERS_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

$(FPCHECKER_EXEC): $(FPCHECKER_OBJECTS)
	@echo "Linking"
	$(CXX) $^ $(LDFLAGS) -lm -o $@

clean:
	/bin/rm -f *.o *~ $(FP32_EXEC) $(FP64_EXEC) $(MIXED_EXEC) $(MIXED_CLOSED_EXEC) $(MIXED_TIMESTEP_EXEC) $(MIXED_HYDRO_EXEC) $(MIXED_THERMO_EXEC) $(MIXED_VISCOSITY_EXEC) $(MIXED_VOLUME_EXEC) $(MIXED_HYDRO_VOLUME_EXEC) $(MIXED_REFERENCE_EXEC) $(MIXED_PARAMETERS_EXEC) $(FPCHECKER_EXEC)
	/bin/rm -rf *.dSYM

tar: clean
	cd .. ; tar cvf lulesh-2.0.tar LULESH-2.0 ; mv lulesh-2.0.tar LULESH-2.0

