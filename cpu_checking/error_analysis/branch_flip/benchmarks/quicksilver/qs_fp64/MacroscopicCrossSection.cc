#include "MacroscopicCrossSection.hh"
#include "QS_Precision.hh"
#include "MonteCarlo.hh"
#include "MaterialDatabase.hh"
#include "NuclearData.hh"
#include "MC_Cell_State.hh"
#include "DeclareMacro.hh"

//----------------------------------------------------------------------------------------------------------------------
//  Routine MacroscopicCrossSection calculates the number-density-weighted macroscopic cross
//  section of a cell.
//
//  A reactionIndex of -1 means total cross section.
//----------------------------------------------------------------------------------------------------------------------
HOST_DEVICE 
qs_real macroscopicCrossSection(MonteCarlo* monteCarlo, int reactionIndex, int domainIndex, int cellIndex,
                               int isoIndex, int energyGroup)
{
   // Initialize various data items.
   int globalMatIndex = monteCarlo->domain[domainIndex].cell_state[cellIndex]._material;

   qs_real atomFraction = monteCarlo->_materialDatabase->_mat[globalMatIndex]._iso[isoIndex]._atomFraction;

   qs_real microscopicCrossSection = qs_real(0.0);
   // The cell number density is the fraction of the atoms in cell
   // volume of this isotope.  We set this (elsewhere) to 1/nIsotopes.
   // This is a statement that we treat materials as if all of their
   // isotopes are present in equal amounts
   qs_real cellNumberDensity = monteCarlo->domain[domainIndex].cell_state[cellIndex]._cellNumberDensity;

   int isotopeGid = monteCarlo->_materialDatabase->_mat[globalMatIndex]._iso[isoIndex]._gid;
   if ( atomFraction == qs_real(0.0) || cellNumberDensity == qs_real(0.0)) { return qs_real(1e-20); }

   if (reactionIndex < 0)
   {
      // Return total cross section
      microscopicCrossSection = monteCarlo->_nuclearData->getTotalCrossSection(isotopeGid, energyGroup);
   }
   else
   {
      // Return the reaction cross section
      microscopicCrossSection = monteCarlo->_nuclearData->getReactionCrossSection((unsigned int)reactionIndex,
                isotopeGid, energyGroup);
   }

   return atomFraction * cellNumberDensity * microscopicCrossSection;

}
HOST_DEVICE_END


//----------------------------------------------------------------------------------------------------------------------
//  Routine weightedMacroscopicCrossSection calculates the number-density-weighted
//  macroscopic cross section of the collection of isotopes in a cell.
//dfr Weighted is a bit of a misnomer here, since there is no weighting
//applied by this routine.  In Mercury we would weight for multiple
//materials in a cell.
//----------------------------------------------------------------------------------------------------------------------
HOST_DEVICE
qs_real weightedMacroscopicCrossSection(MonteCarlo* monteCarlo, int taskIndex, int domainIndex,
                                       int cellIndex, int energyGroup)
{
   qs_real* precomputedCrossSection =
      &monteCarlo->domain[domainIndex].cell_state[cellIndex]._total[energyGroup];
   qs_assert (precomputedCrossSection != NULL);
   if (*precomputedCrossSection > qs_real(0.0))
      return *precomputedCrossSection;
   
   int globalMatIndex = monteCarlo->domain[domainIndex].cell_state[cellIndex]._material;
   int nIsotopes = (int)monteCarlo->_materialDatabase->_mat[globalMatIndex]._iso.size();
   qs_real sum = qs_real(0.0);
   for (int isoIndex = 0; isoIndex < nIsotopes; isoIndex++)
   {
      sum += macroscopicCrossSection(monteCarlo, -1, domainIndex, cellIndex,
                                     isoIndex, energyGroup);
   }

   QS::atomicWrite( *precomputedCrossSection, sum );

   return sum;
}
HOST_DEVICE_END
