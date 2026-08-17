#ifndef NUCLEAR_DATA_HH
#define NUCLEAR_DATA_HH

#include "QS_Precision.hh"

#include <cstdio>
#include <string>
#include "QS_Vector.hh"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "qs_assert.hh"
#include "DeclareMacro.hh"

class Polynomial
{
 public:
   Polynomial(qs_real aa, qs_real bb, qs_real cc, qs_real dd, qs_real ee)
   :
   _aa(aa), _bb(bb), _cc(cc), _dd(dd), _ee(ee){}

   qs_real operator()(qs_real xx) const
   {
      return _ee + xx * (_dd + xx * (_cc + xx * (_bb + xx * (_aa))));
   }

 private:
   qs_real _aa, _bb, _cc, _dd, _ee;
};

// Lowest level class at the reaction level
class NuclearDataReaction
{
 public:
   // The types of reactions
   enum Enum
   {
      Undefined = 0,
      Scatter,
      Absorption,
      Fission
   };
   
   NuclearDataReaction(){};

   NuclearDataReaction(Enum reactionType, qs_real nuBar, const qs_vector<qs_real>& energies,
                       const Polynomial& polynomial, qs_real reationCrossSection);
   

   HOST_DEVICE_CUDA
   qs_real getCrossSection(unsigned int group);
   HOST_DEVICE_CUDA
   void sampleCollision(qs_real incidentEnergy, qs_real material_mass, qs_real* energyOut,
                        qs_real* angleOut, int &nOut, uint64_t* seed, int max_production_size);
   
   
   qs_vector<qs_real> _crossSection; //!< tabular data for microscopic cross section
   Enum _reactionType;                //!< What type of reaction is this
   qs_real _nuBar;                     //!< If this is a fission, specify the nu bar

};

// This class holds an array of reactions for neutrons
class NuclearDataSpecies
{
 public:
   
   void addReaction(NuclearDataReaction::Enum type, qs_real nuBar, qs_vector<qs_real>& energies,
                    const Polynomial& polynomial, qs_real reactionCrossSection);
   
   qs_vector<NuclearDataReaction> _reactions;
};

// For this isotope, store the cross sections. In this case the species is just neutron.
class NuclearDataIsotope
{
 public:
   
   NuclearDataIsotope()
   : _species(1,VAR_MEM){}
   
   qs_vector<NuclearDataSpecies> _species;

};

// Top level class to handle all things related to nuclear data
class NuclearData
{
 public:
   
   NuclearData(int numGroups, qs_real energyLow, qs_real energyHigh);

   int addIsotope(int nReactions,
                  const Polynomial& fissionFunction,
                  const Polynomial& scatterFunction,
                  const Polynomial& absorptionFunction,
                  qs_real nuBar,
                  qs_real totalCrossSection,
                  qs_real fissionWeight, qs_real scatterWeight, qs_real absorptionWeight);

   HOST_DEVICE_CUDA
   int getEnergyGroup(qs_real energy);
   HOST_DEVICE_CUDA
   int getNumberReactions(unsigned int isotopeIndex);
   HOST_DEVICE_CUDA
   qs_real getTotalCrossSection(unsigned int isotopeIndex, unsigned int group);
   HOST_DEVICE_CUDA
   qs_real getReactionCrossSection(unsigned int reactIndex, unsigned int isotopeIndex, unsigned int group);

   int _numEnergyGroups;
   // Store the cross sections and reactions by isotope, which stores
   // it by species
   qs_vector<NuclearDataIsotope> _isotopes;
   // This is the overall energy layout. If we had more than just
   // neutrons, this array would be a vector of vectors.
   qs_vector<qs_real> _energies;

};

#endif

// The input for the nuclear data comes from the material section
// The input looks may like
//
// material NAME
// nIsotope=XXX
// nReactions=XXX
// fissionCrossSection="XXX"
// scatterCrossSection="XXX"
// absorptionCrossSection="XXX"
// nuBar=XXX
// totalCrossSection=XXX
// fissionWeight=XXX
// scatterWeight=XXX
// absorptionWeight=XXX
//
// Material NAME2
// ...
//
// table NAME
// a=XXX
// b=XXX
// c=XXX
// d=XXX
// e=XXX
//
// table NAME2
//
// Each isotope inside a material will have identical cross sections.
// However, it will be treated as unique in the nuclear data.
// Cross sectionsare strings that refer to tables
