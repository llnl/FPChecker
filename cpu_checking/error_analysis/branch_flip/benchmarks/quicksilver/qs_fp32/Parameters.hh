/// \file
/// Read parameters from command line arguments and input file.

#ifndef PARAMETERS_HH
#define PARAMETERS_HH

#include "QS_Precision.hh"

#include <string>
#include <vector>
#include <map>
#include <iostream>

struct GeometryParameters
{
   enum Shape{UNDEFINED, BRICK, SPHERE};

   GeometryParameters()
   : materialName(),
     shape(UNDEFINED),
     radius(qs_real(0.0)),
     xCenter(qs_real(0.0)),
     yCenter(qs_real(0.0)),
     zCenter(qs_real(0.0)),
     xMin(qs_real(0.0)),
     yMin(qs_real(0.0)),
     zMin(qs_real(0.0)),
     xMax(qs_real(0.0)),
     yMax(qs_real(0.0)),
     zMax(qs_real(0.0))
   {};

   std::string materialName;
   Shape shape;
   qs_real radius;
   qs_real xCenter;
   qs_real yCenter;
   qs_real zCenter;
   qs_real xMin;
   qs_real yMin;
   qs_real zMin;
   qs_real xMax;
   qs_real yMax;
   qs_real zMax;
};

struct MaterialParameters
{
   MaterialParameters()
   : name(),
     mass(qs_real(1000.0)),
     totalCrossSection(qs_real(1.0)),
     nIsotopes(10),
     nReactions(9),
     sourceRate(qs_real(0.0)),
     scatteringCrossSection(),
     absorptionCrossSection(),
     fissionCrossSection(),
     scatteringCrossSectionRatio(qs_real(1.0)),
     absorptionCrossSectionRatio(qs_real(1.0)),
     fissionCrossSectionRatio(qs_real(1.0))
   {};

   std::string name;
   qs_real mass;
   qs_real totalCrossSection;
   int nIsotopes;
   int nReactions;
   qs_real sourceRate;
   std::string scatteringCrossSection;
   std::string absorptionCrossSection;
   std::string fissionCrossSection;
   qs_real scatteringCrossSectionRatio;
   qs_real absorptionCrossSectionRatio;
   qs_real fissionCrossSectionRatio;
};

struct CrossSectionParameters
{
   CrossSectionParameters()
   : name(),
     aa(qs_real(0.0)),
     bb(qs_real(0.0)),
     cc(qs_real(0.0)),
     dd(qs_real(0.0)),
     ee(qs_real(1.0)),
     nuBar(qs_real(2.4))
   {};

   std::string name;
   qs_real aa;
   qs_real bb;
   qs_real cc;
   qs_real dd;
   qs_real ee;
   qs_real nuBar;
};

struct SimulationParameters
{
   SimulationParameters()
   : inputFile(),
     crossSectionsOut(""),
     boundaryCondition("reflect"),
     energySpectrum(""),
     loadBalance(0),
     cycleTimers(0),
     debugThreads(0),
     nParticles(1000000), // 10^6
     batchSize(0), // default to use nBatches
     nBatches(10),
     nSteps(10),
     nx(10), //speed up early testing
     ny(10),
     nz(10),
//     nx(100),
//     ny(100),
//     nz(100),
     seed(1029384756),
     xDom(0),
     yDom(0),
     zDom(0),
     dt(qs_real(1e-8)),
     fMax(qs_real(0.1)),
     lx(qs_real(100.0)),
     ly(qs_real(100.0)),
     lz(qs_real(100.0)),
     eMin(qs_real(1e-9)),
     eMax(20),
     nGroups(230), 
     lowWeightCutoff(qs_real(0.001)),
     balanceTallyReplications(1),
     fluxTallyReplications(1),
     cellTallyReplications(1),
     coralBenchmark(0)
   {};

   std::string inputFile;        //!< name of input file
   std::string energySpectrum;   //!< enble computing and printing energy spectrum via of energy spectrum file 
   std::string crossSectionsOut; //!< enable or disable printing cross section data to a file
   std::string boundaryCondition;//!< specifies boundary conditions
   int loadBalance;              //!< enable or disable load balancing
   int cycleTimers;              //!< enable or disable cycle timers 
   int debugThreads;             //!< enable or disable thread debugging lines
   uint64_t nParticles;          //!< number of particles
   uint64_t batchSize;           //!< number of particles in a batch
   uint64_t nBatches;            //!< number of batches to start
   int nSteps;                   //!< number of time steps
   int nx;                       //!< number of mesh elements in x-direction
   int ny;                       //!< number of mesh elements in y-direction
   int nz;                       //!< number of mesh elements in z-direction
   int seed;                     //!< random number seed
   int xDom;                     //!< number of MPI ranks in x-direction
   int yDom;                     //!< number of MPI ranks in y-direction
   int zDom;                     //!< number of MPI ranks in z-direction
   qs_real dt;                    //!< time step (seconds)
   qs_real fMax;                  //!< max random fractional displacement of mesh
   qs_real lx;                    //!< size of problem domain in x-direction (cm)
   qs_real ly;                    //!< size of problem domain in y-direction (cm)
   qs_real lz;                    //!< size of problem domain in z-direction (cm)
   qs_real eMin;                  //!< min energy of cross section
   qs_real eMax;                  //!< max energy of cross section
   int nGroups;                  //!< number of groups for cross sections
   qs_real lowWeightCutoff;       //!< low weight roulette cutoff
   int balanceTallyReplications; //!< Number of replications for the balance tallies
   int fluxTallyReplications;    //!< Number of replications for the scalar flux tally
   int cellTallyReplications;    //!< Number of replications for the scalar cell tally
   int coralBenchmark;           //!< enable correctness check for Coral2 benchmark
};

struct Parameters
{
   SimulationParameters                          simulationParams;
   std::vector<GeometryParameters>               geometryParams;
   std::map<std::string, MaterialParameters>     materialParams;
   std::map<std::string, CrossSectionParameters> crossSectionParams;
};

Parameters getParameters(int argc, char** argv);
void printParameters(const Parameters& params, std::ostream& out);

std::ostream& operator<<(std::ostream& out, const SimulationParameters& pp);
std::ostream& operator<<(std::ostream& out, const GeometryParameters& pp);
std::ostream& operator<<(std::ostream& out, const MaterialParameters& pp);
std::ostream& operator<<(std::ostream& out, const CrossSectionParameters& pp);

#endif
