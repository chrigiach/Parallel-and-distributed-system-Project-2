/*
*Parallel and distributed systems | Project 2 | main file
*/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <sstream>
#include <mpi.h>
#include "distr_by_median.h"

using namespace std;

int main(int argc, char* argv[]){
	cout << "\n";
	cout << "Let's begin" << endl;
	cout << endl;
	
	//*****initialize useful variables*****
	int ierr; // the error "holder" for mpi functions
	int rank; // the rank of the mpi process
	int num_proc; // the number of mpi processes
	int count; // just a counter
	int pivot = 0; // the pivot that the leader will choose
	int dest; // the destination rank for mpi communications
	int source; // the source rank for mpi communications
	int tag; // the tag that helps the processes "chat" with each other
	int counter = 0; //just another counter
	
	
	MPI_Status stat;
	
	//*******initialize the mpi processes********
	ierr = MPI_Init (&argc, &argv);
	
	//check if mpi works fine
	if ( ierr != 0 ){
		cout << "\n";
		cout << "Project 2 - Fatal error!\n";
		cout << "  MPI_Init returned ierr = " << ierr << "\n";
		exit ( 1 );
	}
	
	int64_t p; // number of vectors
	int64_t dimension; // size of vectors
	int bin_content; // number of my vectors

	//get the number of mpi processes
	ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
	
	//get my rank
	ierr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	//*******Read the data********
	
	if(rank == 0){
		//open the file(change the name of the file so that you can test whatever binary file you want)
		ifstream myFile ("mnist.bin", ios::in | ios::binary);
		
		int64_t dimensions;
		int64_t elem_num;
		
		cout << "Let's save the binary info" << endl;
		
		//read the number of the vectors' dimensions
		myFile.read((char*)&dimensions, sizeof(int64_t));
		dimension = dimensions;
		cout << "Dimensions are " << dimension << endl;
		
		
		//read the number of vectors
		myFile.read((char*)&elem_num, sizeof(int64_t));
		p = elem_num;
		cout << "Number of vectors is " << p << endl;
		
		//count how many vectors will everyone contain
		bin_content = p/num_proc;
		//cout << "I rank 0 will hold " << bin_content << " vectors" << endl;
		
		//cout << "Let's read my data." << endl;
		//cout << endl;
		
		//read and store the data that belong to me
		vector<vector<double>> myElements; //where I store my part of the data
		double x;
		for(int i = 0 ; i < bin_content; i++){
			vector<double> temporary;
			for(int j = 0 ; j < dimension ; j++){
				myFile.read((char*)&x, sizeof(double));
				temporary.push_back(x);
			}
			myElements.push_back(temporary);
		}
		//cout << "I rank 0 have stored my part of the data." << endl;
		//cout << endl;
		
		//close the file, we do not need it anymore
		myFile.close();
		
		//********Find the pivot and send it all over the world***********
		srand(time(NULL));
		pivot = (rand()%bin_content) + 1;
		cout << "P:" << rank << " Pivot is "<< pivot <<"\n";
		cout << "\n";
		/*
		cout << "P:" << rank << " - setting up data to send the pivot to all other processes.\n";
		*/
		count = dimension;
		tag = 1;
		//send the vector-pivot
		vector<double> temp1;
		for (int i = 0 ; i < myElements[pivot].size() ; i++){
			temp1.push_back(myElements[pivot][i]);
		}
		//cout << "temp1 size just for checking " << temp1.size() << endl;
		for(int i = 1 ; i < num_proc ; i++){
			dest = i;
			ierr = MPI_Send(&temp1[0], count, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
		}
		//cout << "Pivot is sent to everybody" << endl;
		
		//call the distributeByMedian function
		team_size = num_proc;
		destributeByMedian(team_size, &myElements, num_proc, rank, bin_content, dimension);
		
	} else {
		
		//open the file(change the name of the file so that you can test whatever binary file you want)
		ifstream myFile ("mnist.bin", ios::in | ios::binary);
		
		int64_t dimensions;
		int64_t elem_num;
		
		//cout << "Let's save the binary info" << endl;
		
		//read the number of the vectors' dimensions
		myFile.read((char*)&dimensions, sizeof(int64_t));
		dimension = dimensions;
		//cout << "Dimensions are " << dimension << endl;
		
		//read the number of vectors
		myFile.read((char*)&elem_num, sizeof(int64_t));
		p = elem_num;
		//cout << "Number of vectors is " << p << endl;
		
		//count how many vectors will everyone contain
		bin_content = p/num_proc;
		//cout << "Bin_content size is " << bin_content << endl;
		
		//*********read my part of data**************
		//cout << "Let's read my data." << endl;
		//cout << endl;
		//read and store the data that belong to me
		vector<vector<double>> myElements; //where I store my part of the data
		double x;
		
		//ignore the part that belongs to other processes before me
		for(int i = 0 ; i < rank; i++){
			for(int j = 0 ; j < bin_content ; j++){
				myFile.read((char*)&x, sizeof(double));
			}
		}
		
		for(int i = 0 ; i < bin_content; i++){
			vector<double> temporary;
			for(int j = 0 ; j < dimension ; j++){
				myFile.read((char*)&x, sizeof(double));
				temporary.push_back(x);
			}
			myElements.push_back(temporary);
		}
		//cout << "I rank '"<< rank <<"' have stored my part of the data." << endl;
		//cout << endl;
		
		//close the file, we do not need it anymore
		myFile.close();
		/*
		cout << "\n";
		cout << "P:" << rank << " - setting up data to receive the pivot from P:0.\n";
		*/
		count = dimension;
		source = 0;
		tag = 1;
		vector<double> pivot;
		pivot.resize(dimension);
		//receive the vector-pivot
		MPI_Recv(&pivot[0], count, MPI_DOUBLE, source, tag, MPI_COMM_WORLD, &stat);
		//cout << "P:" << rank << " DONE!!! Got the pivot"<< endl;
		
		//call distributeByMedian function
		team_size = num_proc;
		destributeByMedian(team_size, &myElements, num_proc, rank, bin_content, dimension);
	}
	
	//terminate the mpi processes
	MPI_Finalize ( );
	
	cout << "I, rank "<<rank<<", can now rest in peace. Phew!!!" << endl;
	cout << "\n";
	return 0;
}