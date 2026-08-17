#include "CoralBenchmark.hh"
#include "QS_Precision.hh"
#include "MonteCarlo.hh"
#include "Parameters.hh"
#include "Tallies.hh"
#include "utilsMpi.hh"
#include "MC_Processor_Info.hh"
#include <cmath>

void BalanceRatioTest( MonteCarlo* monteCarlo, Parameters &params );
void BalanceEventTest( MonteCarlo *monteCarlo );
void MissingParticleTest( MonteCarlo* monteCarlo );
void FluenceTest( MonteCarlo* monteCarlo );

//
//  Coral Benchmark tests are only relavent/tested for the  coral benchmark input deck
//
void coralBenchmarkCorrectness( MonteCarlo* monteCarlo, Parameters &params )
{
    if( !params.simulationParams.coralBenchmark )
        return;

    if( monteCarlo->processor_info->rank == 0 )
    {
        //Test Balance Tallies for relative correctness
        //  Expected ratios of absorbs,fisisons, scatters are maintained
        //  withing some tolerance, based on input expectation
        BalanceRatioTest( monteCarlo, params );

        //Test Balance Tallies for equality in number of Facet Crossing 
        //and Collision events 
        BalanceEventTest( monteCarlo );
        
        //Test for lost particles during the simulation
        //  This test should always succeed unless test for 
        //  done was broken, or we are running with 1 MPI rank
        //  and so never preform this test duing test_for_done
        MissingParticleTest( monteCarlo );
    }

    //Test that the scalar flux is homogenous across cells for the problem
    //  This test really required alot of particles or cycles or both
    //  This solution should converge to a homogenous solution
    FluenceTest( monteCarlo );
}

void BalanceRatioTest( MonteCarlo *monteCarlo, Parameters &params )
{
    fprintf(stdout,"\n");
    fprintf(stdout, "Testing Ratios for Absorbtion, Fission, and Scattering are maintained\n");

    Balance &balTally = monteCarlo->_tallies->_balanceCumulative;

    uint64_t absorb     = balTally._absorb;
    uint64_t fission    = balTally._fission;
    uint64_t scatter    = balTally._scatter;
    qs_real absorbRatio, fissionRatio, scatterRatio;

    qs_real percent_tolerance = qs_real(1.0);

    //Hardcoding Ratios for each specific test
    if( params.simulationParams.coralBenchmark == 1 )
    {
        fissionRatio = qs_real(0.05); 
        scatterRatio = 1;
        absorbRatio  = qs_real(0.04);
    }
    else if( params.simulationParams.coralBenchmark == 2 )
    {
        fissionRatio = qs_real(0.075); 
        scatterRatio = qs_real(0.830);
        absorbRatio  = qs_real(0.094);
        percent_tolerance = qs_real(1.1);
    }
    else
    {
        //The input provided for which coral problem is incorrect
        qs_assert(false);
    }

    qs_real tolerance = percent_tolerance / qs_real(100.0);

    qs_real Absorb2Scatter  = qsm::abs_( ( absorb /  absorbRatio  ) * (scatterRatio / scatter) - 1);
    qs_real Absorb2Fission  = qsm::abs_( ( absorb /  absorbRatio  ) * (fissionRatio / fission) - 1);
    qs_real Scatter2Absorb  = qsm::abs_( ( scatter / scatterRatio ) * (absorbRatio  / absorb ) - 1);
    qs_real Scatter2Fission = qsm::abs_( ( scatter / scatterRatio ) * (fissionRatio / fission) - 1);
    qs_real Fission2Absorb  = qsm::abs_( ( fission / fissionRatio ) * (absorbRatio  / absorb ) - 1);
    qs_real Fission2Scatter = qsm::abs_( ( fission / fissionRatio ) * (scatterRatio / scatter) - 1);


    bool pass = true;

    if( Absorb2Scatter  > tolerance ) pass = false;
    if( Absorb2Fission  > tolerance ) pass = false;
    if( Scatter2Absorb  > tolerance ) pass = false;
    if( Scatter2Fission > tolerance ) pass = false;
    if( Fission2Absorb  > tolerance ) pass = false;
    if( Fission2Scatter > tolerance ) pass = false;

    if( pass )
    {
        fprintf(stdout, "PASS:: Absorption / Fission / Scatter Ratios maintained with %g%% tolerance\n", qsm::pr(tolerance*qs_real(100.0)));
    }
    else
    {
        fprintf(stdout, "FAIL:: Absorption / Fission / Scatter Ratios NOT maintained with %g%% tolerance\n", qsm::pr(tolerance*qs_real(100.0)));
        fprintf(stdout, "absorb:  %12" PRIu64 "\t%g\n", absorb, qsm::pr(absorbRatio));
        fprintf(stdout, "scatter: %12" PRIu64 "\t%g\n", scatter, qsm::pr(scatterRatio));
        fprintf(stdout, "fission: %12" PRIu64 "\t%g\n", fission, qsm::pr(fissionRatio));
        fprintf(stdout, "Relative Absorb to Scatter:  %g < %g\n", qsm::pr(Absorb2Scatter), qsm::pr(tolerance));
        fprintf(stdout, "Relative Absorb to Fission:  %g < %g\n", qsm::pr(Absorb2Fission), qsm::pr(tolerance));
        fprintf(stdout, "Relative Scatter to Absorb:  %g < %g\n", qsm::pr(Scatter2Absorb), qsm::pr(tolerance));
        fprintf(stdout, "Relative Scatter to Fission: %g < %g\n", qsm::pr(Scatter2Fission), qsm::pr(tolerance));
        fprintf(stdout, "Relative Fission to Absorb:  %g < %g\n", qsm::pr(Fission2Absorb), qsm::pr(tolerance));
        fprintf(stdout, "Relative Fission to Scatter: %g < %g\n", qsm::pr(Fission2Scatter), qsm::pr(tolerance));
    }

}

void BalanceEventTest( MonteCarlo *monteCarlo )
{

    fprintf(stdout,"\n");
    fprintf(stdout, "Testing balance between number of facet crossings and reactions\n");

    Balance &balTally = monteCarlo->_tallies->_balanceCumulative;

    uint64_t num_segments = balTally._numSegments;
    uint64_t collisions   = balTally._collision;
    uint64_t census       = balTally._census;

    uint64_t facetCrossing = num_segments - census - collisions;

    qs_real ratio = qsm::abs_( (qs_real(facetCrossing) / qs_real(collisions)) - 1);
    
    qs_real tolerance = qs_real(1.0);    
    bool pass = true;
    if( ratio > (tolerance/qs_real(100.0)) ) pass = false; 
    
    if( pass )
    {
        fprintf( stdout, "PASS:: Collision to Facet Crossing Ratio maintained even balanced within %g%% tolerance\n", qsm::pr(tolerance));
    }
    else
    {
        fprintf(stdout, " FAIL:: Collision to Facet Crossing Ratio balanced NOT maintained within %g%% tolerance\n", qsm::pr(tolerance));
        fprintf(stdout, "\tFacet Crossing: %" PRIu64 "\tCollision: %" PRIu64 "\tRatio: %g\n", facetCrossing, collisions, qsm::pr(ratio));
    }


}

void MissingParticleTest( MonteCarlo *monteCarlo )
{
    fprintf(stdout,"\n");
    fprintf(stdout, "Test for lost / unaccounted for particles in this simulation\n");

    Balance &balTally = monteCarlo->_tallies->_balanceCumulative;

    uint64_t gains = 0, losses = 0;
    
    gains   = balTally._start  + balTally._source + balTally._produce + balTally._split;
    losses  = balTally._absorb + balTally._census + balTally._escape  + balTally._rr + balTally._fission;

    if( gains == losses )
    {
        fprintf( stdout, "PASS:: No Particles Lost During Run\n" );
    }
    else
    {
        fprintf( stdout, "FAIL:: Particles Were Lost During Run, test for done should have failed\n" );
    }


}


void FluenceTest( MonteCarlo* monteCarlo )
{
    if( monteCarlo->processor_info->rank == 0 )
    {
        fprintf(stdout,"\n");
        fprintf(stdout, "Test Fluence for homogeneity across cells\n");
    }

    qs_real max_diff = qs_real(0.0);

    int numDomains = monteCarlo->_tallies->_fluence._domain.size();
    for (int domainIndex = 0; domainIndex < numDomains; domainIndex++)
    {
        
        qs_real local_sum = qs_real(0.0);
        int numCells = monteCarlo->_tallies->_fluence._domain[domainIndex]->size(); 

        for (int cellIndex = 0; cellIndex < numCells; cellIndex++)
        {
            local_sum += monteCarlo->_tallies->_fluence._domain[domainIndex]->getCell( cellIndex );
        }

        qs_real average = local_sum / numCells;
        
        for (int cellIndex = 0; cellIndex < numCells; cellIndex++)
        {
            qs_real cellValue = monteCarlo->_tallies->_fluence._domain[domainIndex]->getCell( cellIndex );
            qs_real percent_diff = (((cellValue > average) ? cellValue - average : average - cellValue ) / (( cellValue + average)/qs_real(2.0)))*100;
            max_diff = ( (max_diff > percent_diff) ? max_diff : percent_diff );
        }
    }

    qs_real percent_tolerance = qs_real(6.0);

    qs_real max_diff_global = qs_real(0.0);

    mpiAllreduce(&max_diff, &max_diff_global, 1, MPI_DOUBLE, MPI_MAX, monteCarlo->processor_info->comm_mc_world);

    if( monteCarlo->processor_info->rank == 0 )
    {
        if( max_diff_global > percent_tolerance )
        {
            fprintf( stdout, "FAIL:: Fluence not homogenous across cells within %g%% tolerance\n", qsm::pr(percent_tolerance));
            fprintf( stdout, "\tTry running more particles or more cycles to see if Max Percent Difference goes down.\n");
            fprintf( stdout, "\tCurrent Max Percent Diff: %4.1f%%\n", qsm::pr(max_diff_global));
        }
        else
        {
            fprintf( stdout, "PASS:: Fluence is homogenous across cells with %g%% tolerance\n", qsm::pr(percent_tolerance));
        }
    }

}
