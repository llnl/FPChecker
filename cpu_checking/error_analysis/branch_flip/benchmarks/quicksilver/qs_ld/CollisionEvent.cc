#include "CollisionEvent.hh"
#include "QS_Precision.hh"
#include "MC_Particle.hh"
#include "NuclearData.hh"
#include "DirectionCosine.hh"
#include "MonteCarlo.hh"
#include "MC_Cell_State.hh"
#include "MaterialDatabase.hh"
#include "MacroscopicCrossSection.hh"
#include "MC_Base_Particle.hh"
#include "ParticleVaultContainer.hh"
#include "PhysicalConstants.hh"
#include "DeclareMacro.hh"
#include "QS_atomics.hh"

#define MAX_PRODUCTION_SIZE 4

//----------------------------------------------------------------------------------------------------------------------
//  Routine MC_Collision_Event determines the isotope, reaction and secondary (projectile)
//  particle characteristics for a collision event.
//
//  Return true if the particle will continue.
//----------------------------------------------------------------------------------------------------------------------

HOST_DEVICE
void updateTrajectory( qs_real energy, qs_real angle, MC_Particle& particle )
{
    particle.kinetic_energy = energy;
    qs_real cosTheta = angle;
    qs_real randomNumber = rngSample(&particle.random_number_seed);
    qs_real phi = 2 * qs_real(3.14159265) * randomNumber;
    qs_real sinPhi = qsm::sin_(phi);
    qs_real cosPhi = qsm::cos_(phi);
    qs_real sinTheta = qsm::sqrt_((qs_real(1.0) - (cosTheta*cosTheta)));
    particle.direction_cosine.Rotate3DVector(sinTheta, cosTheta, sinPhi, cosPhi);
    qs_real speed = (PhysicalConstants::_speedOfLight *
            qsm::sqrt_((qs_real(1.0) - ((PhysicalConstants::_neutronRestMassEnergy *
            PhysicalConstants::_neutronRestMassEnergy) /
            ((energy + PhysicalConstants::_neutronRestMassEnergy) *
            (energy + PhysicalConstants::_neutronRestMassEnergy))))));
    particle.velocity.x = speed * particle.direction_cosine.alpha;
    particle.velocity.y = speed * particle.direction_cosine.beta;
    particle.velocity.z = speed * particle.direction_cosine.gamma;
    randomNumber = rngSample(&particle.random_number_seed);
    particle.num_mean_free_paths = -qs_real(1.0)*qsm::log_(randomNumber);
}
HOST_DEVICE_END

HOST_DEVICE

bool CollisionEvent(MonteCarlo* monteCarlo, MC_Particle &mc_particle, unsigned int tally_index)
{
   const MC_Cell_State &cell = monteCarlo->domain[mc_particle.domain].cell_state[mc_particle.cell];

   int globalMatIndex = cell._material;

   //------------------------------------------------------------------------------------------------------------------
   //    Pick the isotope and reaction.
   //------------------------------------------------------------------------------------------------------------------
   qs_real randomNumber = rngSample(&mc_particle.random_number_seed);
   qs_real totalCrossSection = mc_particle.totalCrossSection;
   qs_real currentCrossSection = totalCrossSection * randomNumber;
   int selectedIso = -1;
   int selectedUniqueNumber = -1;
   int selectedReact = -1;
   int numIsos = (int)monteCarlo->_materialDatabase->_mat[globalMatIndex]._iso.size();
   
   for (int isoIndex = 0; isoIndex < numIsos && currentCrossSection >= 0; isoIndex++)
   {
      int uniqueNumber = monteCarlo->_materialDatabase->_mat[globalMatIndex]._iso[isoIndex]._gid;
      int numReacts = monteCarlo->_nuclearData->getNumberReactions(uniqueNumber);
      for (int reactIndex = 0; reactIndex < numReacts; reactIndex++)
      {
         currentCrossSection -= macroscopicCrossSection(monteCarlo, reactIndex, mc_particle.domain, mc_particle.cell,
                   isoIndex, mc_particle.energy_group);
         if (currentCrossSection < 0)
         {
            selectedIso = isoIndex;
            selectedUniqueNumber = uniqueNumber;
            selectedReact = reactIndex;
            break;
         }
      }
   }
   qs_assert(selectedIso != -1);

   //------------------------------------------------------------------------------------------------------------------
   //    Do the collision.
   //------------------------------------------------------------------------------------------------------------------
   qs_real energyOut[MAX_PRODUCTION_SIZE];
   qs_real angleOut[MAX_PRODUCTION_SIZE];
   int nOut = 0;
   qs_real mat_mass = monteCarlo->_materialDatabase->_mat[globalMatIndex]._mass;

   monteCarlo->_nuclearData->_isotopes[selectedUniqueNumber]._species[0]._reactions[selectedReact].sampleCollision(
      mc_particle.kinetic_energy, mat_mass, &energyOut[0], &angleOut[0], nOut, &(mc_particle.random_number_seed), MAX_PRODUCTION_SIZE );

   //--------------------------------------------------------------------------------------------------------------
   //  Post-Collision Phase 1:
   //    Tally the collision
   //--------------------------------------------------------------------------------------------------------------

   // Set the reaction for this particle.
   QS::atomicIncrement( monteCarlo->_tallies->_balanceTask[tally_index]._collision );
   NuclearDataReaction::Enum reactionType = monteCarlo->_nuclearData->_isotopes[selectedUniqueNumber]._species[0].\
           _reactions[selectedReact]._reactionType;
   switch (reactionType)
   {
      case NuclearDataReaction::Scatter:
         QS::atomicIncrement( monteCarlo->_tallies->_balanceTask[tally_index]._scatter);
         break;
      case NuclearDataReaction::Absorption:
         QS::atomicIncrement( monteCarlo->_tallies->_balanceTask[tally_index]._absorb);
         break;
      case NuclearDataReaction::Fission:
         QS::atomicIncrement( monteCarlo->_tallies->_balanceTask[tally_index]._fission);
         QS::atomicAdd( monteCarlo->_tallies->_balanceTask[tally_index]._produce, (uint64_t) nOut);
         break;
      case NuclearDataReaction::Undefined:
         printf("reactionType invalid\n");
         qs_assert(false);
   }

   if( nOut == 0 ) return false;

   for (int secondaryIndex = 1; secondaryIndex < nOut; secondaryIndex++)
   {
        // Newly created particles start as copies of their parent
        MC_Particle secondaryParticle = mc_particle;
        secondaryParticle.random_number_seed = rngSpawn_Random_Number_Seed(&mc_particle.random_number_seed);
        secondaryParticle.identifier = secondaryParticle.random_number_seed;
        updateTrajectory( energyOut[secondaryIndex], angleOut[secondaryIndex], secondaryParticle );
        monteCarlo->_particleVaultContainer->addExtraParticle(secondaryParticle);
   }

   updateTrajectory( energyOut[0], angleOut[0], mc_particle);

   // If a fission reaction produces secondary particles we also add the original
   // particle to the "extras" that we will handle later.  This avoids the 
   // possibility of a particle doing multiple fission reactions in a single
   // kernel invocation and overflowing the extra storage with secondary particles.
   if ( nOut > 1 ) 
       monteCarlo->_particleVaultContainer->addExtraParticle(mc_particle);

   //If we are still tracking this particle the update its energy group
   mc_particle.energy_group = monteCarlo->_nuclearData->getEnergyGroup(mc_particle.kinetic_energy);

   return nOut == 1;
}

HOST_DEVICE_END

