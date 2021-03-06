/*
#############################################################################
# If you use BioFVM in your project, please cite BioFVM and the version     #
# number, such as below:                                                    #
#                                                                           #
# We solved the diffusion equations using BioFVM (Version 1.1.5) [1]        #
#                                                                           #
# [1] A. Ghaffarizadeh, S.H. Friedman, and P. Macklin, BioFVM: an efficient #
#    parallelized diffusive transport solver for 3-D biological simulations,#
#    Bioinformatics 32(8): 1256-8, 2016. DOI: 10.1093/bioinformatics/btv730 #
#                                                                           #
#############################################################################
#                                                                           #
# BSD 3-Clause License (see https://opensource.org/licenses/BSD-3-Clause)   #
#                                                                           #
# Copyright (c) 2015-2017, Paul Macklin and the BioFVM Project              #
# All rights reserved.                                                      #
#                                                                           #
# Redistribution and use in source and binary forms, with or without        #
# modification, are permitted provided that the following conditions are    #
# met:                                                                      #
#                                                                           #
# 1. Redistributions of source code must retain the above copyright notice, #
# this list of conditions and the following disclaimer.                     #
#                                                                           #
# 2. Redistributions in binary form must reproduce the above copyright      #
# notice, this list of conditions and the following disclaimer in the       #
# documentation and/or other materials provided with the distribution.      #
#                                                                           #
# 3. Neither the name of the copyright holder nor the names of its          #
# contributors may be used to endorse or promote products derived from this #
# software without specific prior written permission.                       #
#                                                                           #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       #
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED #
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           #
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER #
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  #
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       #
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        #
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    #
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      #
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        #
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              #
#                                                                           #
#############################################################################
*/

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <random>

#include "../BioFVM.h" 

using namespace BioFVM;

int omp_num_threads = 8; // set number of threads for parallel computing
// set this to # of CPU cores x 2 (for hyperthreading)
double substrate1_supply_rate=10;
double substrate1_saturation_density=1;
double substrate1_uptake_rate=10;

void supply_function( Microenvironment* microenvironment, int voxel_index , std::vector<double>* write_here )
{
	double min_x=microenvironment->mesh.bounding_box[0];
	double max_x=microenvironment->mesh.bounding_box[3];
	double min_y=microenvironment->mesh.bounding_box[1];
	double max_y=microenvironment->mesh.bounding_box[4];
	double min_z=microenvironment->mesh.bounding_box[2];
	double max_z=microenvironment->mesh.bounding_box[5];
	double strip_width=40;
	(*write_here)[0] = 0; 

	if( abs(max_x-microenvironment->voxels(voxel_index).center[0]) < strip_width || abs(microenvironment->voxels(voxel_index).center[0]- min_x)< strip_width  
			|| abs(max_y-microenvironment->voxels(voxel_index).center[1]) < strip_width || abs(microenvironment->voxels(voxel_index).center[1]- min_y)< strip_width  
				|| abs(max_z-microenvironment->voxels(voxel_index).center[2]) < strip_width || abs(microenvironment->voxels(voxel_index).center[2]- min_z)< strip_width )
				{
					(*write_here)[0] = substrate1_supply_rate;
				}	
				
	return ;
}

void supply_target_function( Microenvironment* microenvironment, int voxel_index , std::vector<double>* write_here )
{
	(*write_here)[0] = substrate1_saturation_density; 	
	return ;
}

void uptake_function( Microenvironment* microenvironment, int voxel_index , std::vector<double>* write_here )
{
	double spheroid_radius=100;
	std::vector<double> center(3);
	center[0] = (microenvironment->mesh.bounding_box[0]+microenvironment->mesh.bounding_box[3])/2;
	center[1] = (microenvironment->mesh.bounding_box[1]+microenvironment->mesh.bounding_box[4])/2;
	center[2] = (microenvironment->mesh.bounding_box[2]+microenvironment->mesh.bounding_box[5])/2;
	if(sqrt( norm_squared(microenvironment->voxels(voxel_index).center - center))<spheroid_radius)
		(*write_here)[0] = substrate1_uptake_rate; 	
	return ;
}

int main( int argc, char* argv[] )
{	
	// openmp setup
	omp_set_num_threads(omp_num_threads);
	
	// create a microenvironment; 
	Microenvironment microenvironment;
	microenvironment.name="substrate scale";

	microenvironment.set_density(0, "substrate1" , "dimensionless" );
	microenvironment.spatial_units = "microns";
	microenvironment.mesh.units = "microns";
	microenvironment.time_units = "minutes";
	
	
	double minX=0, minY=0, minZ=0, maxX=1000, maxY=1000, maxZ=1000, mesh_resolution=10;
	microenvironment.resize_space_uniform( minX,maxX,minY,maxY,minZ,maxZ, mesh_resolution );
	microenvironment.display_information( std::cout );
	
	#pragma omp parallel for 
	for( int i=0; i < microenvironment.number_of_voxels() ; i++ )
	{
		microenvironment.density_vector(i)[0]= 1.0; 
	}
	microenvironment.write_to_matlab( "initial_concentration.mat" );

	// register the diffusion solver 	
	microenvironment.diffusion_decay_solver = diffusion_decay_solver__constant_coefficients_LOD_3D; 
	
	// register substrates properties 
	microenvironment.diffusion_coefficients[0] = 1000; // microns^2 / min 
	microenvironment.decay_rates[0] = 0.01;
				
	// display information 	
	microenvironment.display_information( std::cout );
	
	// register defined supply/uptake functions with the solver
	microenvironment.bulk_supply_rate_function =  supply_function;
	microenvironment.bulk_supply_target_densities_function = supply_target_function;
	microenvironment.bulk_uptake_rate_function = uptake_function;
	
	double dt = 0.01; 	
	double t = 0.0; 
	double t_max=5;

	while( t < t_max )
	{
		microenvironment.simulate_bulk_sources_and_sinks( dt );
		microenvironment.simulate_diffusion_decay( dt );
		t += dt; 
	}
	microenvironment.write_to_matlab( "final.mat" );
	std::cout<<"done!"<<std::endl;
	return 0; 
}