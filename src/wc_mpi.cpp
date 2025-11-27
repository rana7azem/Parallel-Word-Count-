#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
using namespace std;

/* -----------------------------------------------------------
   Helper function: merge two hash maps (for rank 0 final result)
-------------------------------------------------------------*/
void merge_maps(unordered_map<string,int>& A,
                const unordered_map<string,int>& B)
{
    for (auto &p : B)
        A[p.first] += p.second;
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0)
            cout << "Usage: mpiexec -n 4 ./wc_mpi.exe data.txt\n";
        MPI_Finalize();
        return 0;
    }

    string filename = argv[1];

    // Step 1: Rank 0 reads full file
    string full_text = "";
    if (rank == 0)
    {
        ifstream file(filename);
        stringstream buffer;
        buffer << file.rdbuf();
        full_text = buffer.str();
    }

    // Step 2: Broadcast the file length
    int N = full_text.size();
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Step 3: Rank 0 computes variable chunk sizes (don't cut words)
    vector<int> sendcounts(size, 0), displs(size, 0);

    if (rank == 0)
    {
        int base = N / size;
        int start = 0;

        for (int r = 0; r < size; r++) {

            int end = (r == size - 1 ? N : start + base);

            // Expand forward until a whitespace is reached
            while (end < N && full_text[end] != ' ' && full_text[end] != '\n')
                end++;

            sendcounts[r] = end - start;
            displs[r] = start;

            start = end;
        }
    }

    // Step 4: Scatter counts so each rank knows its size
    int local_size = 0;
    MPI_Scatter(sendcounts.data(), 1, MPI_INT,
                &local_size, 1, MPI_INT,
                0, MPI_COMM_WORLD);

    // Step 5: Receive the chunk
    string local_chunk(local_size, ' ');
    MPI_Scatterv(full_text.data(), sendcounts.data(), displs.data(), MPI_CHAR,
                 local_chunk.data(), local_size, MPI_CHAR,
                 0, MPI_COMM_WORLD);

    // Step 6: Local word count
    unordered_map<string,int> local_map;
    stringstream ss(local_chunk);
    string word;
    while (ss >> word)
        local_map[word]++;

    // --------------------------------------------------------------
    // Step 7: Send results back using MPI_Gatherv
    //         (convert map → string, gather strings)
    // --------------------------------------------------------------

    // Convert local map to serialized string: "word count\n"
    string local_serial = "";
    for (auto &p : local_map)
        local_serial += p.first + " " + to_string(p.second) + "\n";

    int my_len = local_serial.size();

    // Gather sizes
    vector<int> recvcounts(size, 0), displs2(size, 0);
    MPI_Gather(&my_len, 1, MPI_INT,
               recvcounts.data(), 1, MPI_INT,
               0, MPI_COMM_WORLD);

    // Rank 0 prepares final buffer
    string final_buffer = "";
    if (rank == 0) {
        int offset = 0;
        for (int r = 0; r < size; r++)
        {
            displs2[r] = offset;
            offset += recvcounts[r];
        }
        final_buffer.resize(offset);
    }

    // Gather serialized maps
    MPI_Gatherv(local_serial.data(), my_len, MPI_CHAR,
                final_buffer.data(), recvcounts.data(), displs2.data(), MPI_CHAR,
                0, MPI_COMM_WORLD);

    // --------------------------------------------------------------
    // Step 8: Rank 0 merges all partial maps
    // --------------------------------------------------------------

    if (rank == 0)
    {
        unordered_map<string,int> final_map;

        stringstream all(final_buffer);
        string w;
        int c;

        while (all >> w >> c)
            final_map[w] += c;

        cout << "===== FINAL WORD COUNT (MPI) =====\n";
        for (auto &p : final_map)
            cout << p.first << " -> " << p.second << "\n";
    }

    MPI_Finalize();
    return 0;
}
