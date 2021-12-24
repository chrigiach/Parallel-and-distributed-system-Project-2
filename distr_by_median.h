/*
*Parallel and distributed systems | Project 2 | destributeByMedian
*/



void destributeByMedian(int team_size, vector<vector<int>> *myElements, int num_proc, int rank, int bin_content, int dimension){
	
	//find out if I am the leader or not
	bool isLeader = 1;
	for(int i = 0 ; i < num_proc ; i+=team_size){
		if(rank == i){
			isLeader = 0;
		}
	}
	
	// if I am the leader
	if(isLeader == 0){		
		
		//measure the distances of my vectors
		
		vector<double> distances;
		vector<double> mydistances;
		
		for(int i = 0 ; i < bin_content ; i++){
			double sum = 0;
			for(int j = 0 ; j < dimension ; j++){
				sum = sum + pow((myElements[i][j] - temp1[j]), 2);
			}
			double distance = sqrt(sum);
			mydistances.push_back(distance);
		}
		
		distances.insert(distances.end(), mydistances.begin(), mydistances.end());
		
		//get the distances from each rank of my team
		count = bin_content;
		tag = 2;
		for(int i = rank + 1 ; i < rank + team_size ; i++){
			source = i;
			vector<double> temp;
			temp.resize(bin_content);
			MPI_Recv(&temp[0], count, MPI_DOUBLE, source, tag, MPI_COMM_WORLD, &stat);
			distances.insert(distances.end(), temp.begin(), temp.end());
		}
		//cout << "I rank " << rank << " have received every set of distances " << endl;
		
		//*****Find the median and send it to the team members*****
		
		// find the median
		//cout << "I rank " << rank << " will find the median." << endl;
		sort(distances.begin(), distances.end());
		//cout << "Distances sorted. I rank " << rank << " will find now the median." << endl;
		double median = (distances[p/2 - 1] + distances[p/2])/2;
		cout << "Median is " << median << endl;
		
		//send the median to the team members
		count = 1;
		tag = 3;
		for(int i = rank + 1 ; i < rank + team_size ; i++){
			dest = i; //follower
			ierr = MPI_Send(&median, count, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
		}
		
		//*****now separate the vectors based on the median*****
		//separate to what I keep and what I give away(smaller or bigger depending the rank I am and the team I belong to)
		
		vector<vector<double>> smaller;
		vector<vector<double>> bigger;
		

		//separate the vectors to smaller and bigger
		
		vector<int> size_table;
		size_table.resize(num_proc);
		
		for(int i = 0 ; i < num_proc ; i++){
			size_table[i] = 0;
		}
		
		for(int i = 0 ; i < bin_content ; i++){
			if(mydistances[i] < median){
				smaller.push_back(myElements[i]);
			} else {
				bigger.push_back(myElements[i]);
			}
		}
			
			//save the small ones because I am the leader
			myElements.swap(smaller);
			smaller.clear();
			size_table[rank] = bigger.size();
			
		
		//receive the number of vectors that will be exchanged
		count = 1;
		tag = 4;
		for(int i = rank + 1 ; i < rank + team_size ; i++){
			int size;
			source = i;
			MPI_Recv(&size, count, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);
			size_table[source] = size;
		}
		
		/*
		cout << "Sizes for exchange:" << endl;
		for(int i = 0 ; i < size_table.size() ; i++){
			cout << size_table[i] << endl;
		}
		*/
		
		//create the exchange table
		vector<vector<int>> exchange_table;
		exchange_table.resize(num_proc);
		for(int i = 0 ; i < num_proc ; i++){
			exchange_table[i].resize(num_proc);
			for(int j = 0 ; j < num_proc ; j++){
				exchange_table[i][j] = 0;
			}
		}
		
		//make the exchange table
		int pointer_big;
		int exch_num;
		
		for(int i = 0 ; i < num_proc/2 - 1 ; i++){
			pointer_big = num_proc/2;
			while(size_table[i] != 0){
				if(size_table[i] < size_table[pointer_big]){
					exch_num = size_table[i];
					exchange_table[i][pointer_big] = exch_num;
					exchange_table[pointer_big][i] = exch_num;
					size_table[i] = size_table[i] - exch_num;
					size_table[pointer_big] = size_table[pointer_big] - exch_num;
				} else if((size_table[i] > size_table[pointer_big]) || (size_table[pointer_big] != 0)){
					exch_num = size_table[pointer_big];
					exchange_table[i][pointer_big] = exch_num;
					exchange_table[pointer_big][i] = exch_num;
					size_table[i] = size_table[i] - exch_num;
					size_table[pointer_big] = size_table[pointer_big] - exch_num;
				}
				if(size_table[pointer_big] == 0){
					pointer_big++;
				}
			}
		}
		
		/*
		for(int i = 0 ; i < num_proc ; i++){
			for(int j = 0 ; j < num_proc ; j++){
				cout << exchange_table[i][j] << " ";
			}
			cout << endl;
		}
		*/
		
		//send the exchange table
		count = num_proc;
		tag = 5;
		vector<int> exch_temp;
		exch_temp.resize(num_proc);
		for(int i = rank + 1 ; i < rank + team_size ; i++){
			dest = i;
			for(int j = 0 ; j < num_proc ; j++){
				exch_temp = exchange_table[j];
				ierr = MPI_Send(&exch_temp[0], count, MPI_INT, dest, tag, MPI_COMM_WORLD);
			}
		}
		
		
		
		//exchange vectors
		int sendtag = 6;
		int recvtag = 6;
		int starting_point = 0;
		for(int i = 0 ; i < num_proc ; i++){
			if(exchange_table[rank][i] != 0){
				cout << "I rank 0 will exchange " << exchange_table[rank][i] << " vectors with rank " << i << endl;
				count = dimension;
				dest = i;
				source = i;
				for(int j = starting_point ; j < starting_point + exchange_table[rank][i] ; j++){
					vector<double> new_vec;
					new_vec.resize(dimension);
					//build the new jth vector that will be sent
					for(int k = 0 ; k < dimension ; k++){
						new_vec[k] = bigger[j][k];
					}
					ierr = MPI_Sendrecv_replace(&new_vec[0], count, MPI_DOUBLE,  dest, sendtag, source, recvtag, MPI_COMM_WORLD, &stat);
					myElements.push_back(new_vec);
				}
				starting_point = starting_point + counter;
			}
		}
		
	} else {
		
		int leader;
		
		//find out who my leader is
		for(int i = 0 ; i < num_proc ; i+=team_size){
			if(rank > i || rank < (i + team_size)){
				leader = i;
			}
		}
		
		
		//*****measure the distances from the pivot and send them to the leader*****
		vector<double> distances;
		for(int i = 0 ; i < bin_content ; i++){
			double sum = 0;
			for(int j = 0 ; j < dimension ; j++){
				sum = sum + pow((myElements[i][j] - pivot[j]),2);
			}
			double distance = sqrt(sum);
			distances.push_back(distance);
		}
		//cout << "Distances measured from me rank " << rank << endl;
		
		//send the distances to leader
		count = bin_content;
		tag = 2;
		dest = leader;
		ierr = MPI_Send(&distances[0], count, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
		//cout << "I rank " << rank << " have sent my set of distances " << endl;
		
		//*****receive the median*****
		double median;
		count = 1;
		source = leader;
		tag = 3;
		MPI_Recv(&median, count, MPI_DOUBLE, source, tag, MPI_COMM_WORLD, &stat);
		//cout << "I rank " << rank << " know that the median is " << median << endl;
		
		//*****now separate the vectors based on the median*****
		//separate to what I keep and what I give away(smaller or bigger depending the rank I am)
		
		vector<vector<double>> smaller;
		vector<vector<double>> bigger;
		

		//separate the vectors to smaller and bigger
		for(int i = 0 ; i < bin_content ; i++){
			if(distances[i] < median){
				smaller.push_back(myElements[i]);
			} else {
				bigger.push_back(myElements[i]);
			}
		}
		
		int size_to_send;
		
		if(rank < leader + team_size/2){
			//save the small ones
			myElements.swap(smaller);
			smaller.clear();
			size_to_send = bigger.size();
		} else {
			//save the big ones
			myElements.swap(bigger);
			bigger.clear();
			size_to_send = smaller.size();
		}
		
		//send the number of vectors that you want to get rid of to the leader
		count = 1;
		dest = leader;
		tag = 4;
		ierr = MPI_Send(&size_to_send, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
		
		//receive the exchange_table
		vector<vector<int>> exchange_table;
		count = num_proc;
		source = leader;
		tag = 5;
		for(int i = 0 ; i < num_proc ; i++){
			vector<int> exch_temp;
			exch_temp.resize(num_proc);
			MPI_Recv(&exch_temp[0], count, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);
			exchange_table.push_back(exch_temp);
		}
		
		//exchange vectors
		int sendtag = 6;
		int recvtag = 6;
		int starting_point = 0;
		for(int i = 0 ; i < num_proc ; i++){
			if(exchange_table[rank][i] != 0){
				cout << "I rank 0 will exchange " << exchange_table[rank][i] << " vectors with rank " << i << endl;
				count = dimension;
				dest = i;
				source = i;
				for(int j = starting_point ; j < starting_point + exchange_table[rank][i] ; j++){
					vector<double> new_vec;
					new_vec.resize(dimension);
					//build the new jth vector that will be sent
					if(rank < leader + team_size/2){
						for(int k = 0 ; k < dimension ; k++){
							new_vec[k] = bigger[j][k];
						}
					} else {
						for(int k = 0 ; k < dimension ; k++){
							new_vec[k] = smaller[j][k];
						}
					}
					ierr = MPI_Sendrecv_replace(&new_vec[0], count, MPI_DOUBLE,  dest, sendtag, source, recvtag, MPI_COMM_WORLD, &stat);
					myElements.push_back(new_vec);
				}
				starting_point = starting_point + counter;
			}
		}
		
	}
	
	//*****Devide into smaller groups and call myself*****
	team_size = team_size / 2;
	if(team_size != 1){
		distributeByMedian(team_size, myElements, num_proc);
	}
}