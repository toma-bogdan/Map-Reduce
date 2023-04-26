#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <list>
#include <unistd.h>
#include <math.h>
#include <set>
#include <string.h>
using namespace std;

pthread_mutex_t mutex;
pthread_barrier_t barrier;

struct mapper_str {
	long id;
  int reducer;
	vector<string> *files;
  vector<vector<int>> perfectPows;

};

struct reducer_str {
  int id;
  vector<mapper_str> *mappers;
};


void *reducer_function(void *arg) {
  // Wait for the mappers to finish
  pthread_barrier_wait(&barrier);

  reducer_str *reducers = (reducer_str*)arg;
  int id = (*reducers).id;

  string name = "out";
  name.append(std::to_string(id + 2));
  name.append(".txt");

  ofstream out(name);

  // Extract the powers at id position for           
  std::set<int> powers_set;
  for (int i = 0; i < (*reducers).mappers->size(); i++) {
    mapper_str mapper = (*reducers).mappers->at(i);
    vector<vector<int>> perfectPowers = mapper.perfectPows;
    for (int p : perfectPowers.at(id)) {
      powers_set.insert(p);
    }
  }

  out << powers_set.size();
  out.close();
  pthread_exit(NULL);
}

/* Returns if the number is power perfect else -1 
   (pow in this function is not used with subunitary base) */
int binary_search(int nr, int left, int right, int exp) {
  if (right >= left) {
    // Calculates mid
    int mid = left + (right - left) / 2;
    if (pow(mid, exp) == nr) {
      // Calculates if the number is power perfect with mid and exponent and returns it
      return mid;
    }

    if (pow(mid, exp) > nr) // if it is bigger, search in the left side
      return binary_search(nr, left, mid - 1, exp);

    // if it is smaller, search in the right side
    return binary_search(nr, mid + 1, right, exp);
  }

  return -1;
}

/* Returns a vector of arrays with nr at position i, where i nr = pow(n,i) */
vector<vector<int>> find_powers(int nr, int nr_reducers) {

  vector<vector<int>> powers;
  for (int i = 0; i < nr_reducers; i++) {
    vector<int> v;
    powers.push_back(v);
  }

  for (int i = 2; i < nr_reducers + 2; i++) {
    // For each exponent we see if: nr = (any_number)^i
    int left = 1;
    int right = sqrt(nr);

    int res = binary_search(nr, left, right, i);

    // Add in mapper, at i position the number if it is power perfect
    if (res != -1)
      powers[i - 2].push_back(nr);
  }


  return powers;
}
void *mapper_function(void *arg) {
  mapper_str *mapper = (mapper_str*)arg;

  int i = 0;
  string aux;
  while (!(*mapper).files->empty()) {
      // Lock a file so only a mapper procces only a file
      pthread_mutex_lock(&mutex);

      // Takes the first file from the vector and removes it 
      aux = (*mapper).files->front();
      (*mapper).files->erase((*mapper).files->begin());

      pthread_mutex_unlock(&mutex);

      ifstream file(aux);
      int n;
      file >> n;

      for (int i = 0; i < n; i++) {
        int nr;
        file >> nr;

        // Calculates if the number has any perfect powers
        vector<vector<int>> res = find_powers(nr, (*mapper).reducer);

        // Copy res into mapper's perfect powers vector of arrays
        for (int i = 0; i < res.size(); i++) {
          for (int j = 0; j < res[i].size(); j++) {
            (*mapper).perfectPows[i].push_back(res[i][j]);
          }
        }
      }
      
      file.close();
  }
  // Waits for all mappers to finish  
  pthread_barrier_wait(&barrier);
  pthread_exit(NULL);
}



int main(int argc, char *argv[]) {

  int N_Map = atoi(argv[1]); // number of mappers
  int N_Reducers = atoi(argv[2]);  // number of reducers
  int N_FILES; // number of input files
  vector<mapper_str> mapper;
  vector<reducer_str> reducer;
  vector<string> files;
  pthread_t threads[N_Map + N_Reducers];
  int r;
  long id;
  void *status;
  pthread_mutex_init(&mutex,NULL);
  pthread_barrier_init(&barrier,NULL,N_Map + N_Reducers);

  ifstream inputFile(argv[3]);
  inputFile >> N_FILES;

  // Reads the input files from test.txt
  for (int i = 0; i < N_FILES; i++) {
    string aux;
    inputFile >> aux;
    files.push_back(aux);
  }

  // Initialize mappers
  for (int i = 0; i < N_Map; i++) {
    mapper_str mapper_aux;
    mapper_aux.files = &files;
    mapper_aux.id = i;

    // Initialize the lists of the perfect power
    for (int i = 0; i < N_Reducers; i++) {
      vector<int> power_list;
      mapper_aux.perfectPows.push_back(power_list);
    }
    mapper_aux.reducer = N_Reducers;
    mapper.push_back(mapper_aux);
  }

  // Initialize the data structure for reducers
  for (int i = 0; i < N_Reducers; i++) {
    reducer_str r;
    r.id = i;
    r.mappers = &mapper;
    reducer.push_back(r);
  }

  for (id = 0; id < N_Map + N_Reducers; id++) {
    if (id < N_Map){ // start the mappers
      r = pthread_create(&threads[id], NULL, mapper_function, &(mapper[id]));
    } else { // start the reducers
      r = pthread_create(&threads[id], NULL, reducer_function, &(reducer[id - N_Map]));
    }
    if (r) {
        printf("Eroare la crearea thread-ului %ld\n", id);
        exit(-1);
    }
  }

  // Wait for the threads to finish
  for (id = 0; id < N_Map + N_Reducers; id++) {
    r = pthread_join(threads[id], &status);

    if (r) {
        printf("Eroare la asteptarea thread-ului %ld\n", id);
        exit(-1);
    }
  }
  pthread_mutex_destroy(&mutex);
  pthread_barrier_destroy(&barrier);

  pthread_exit(NULL);
}
