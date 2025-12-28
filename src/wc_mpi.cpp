#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
using namespace std;


void merge_maps(unordered_map<string,int>& A,
                const unordered_map<string,int>& B)
{
    for (auto &p : B)
        A[p.first] += p.second;
}


// NON-BLOCKING MPI IMPLEMENTATION



int run_nonblocking_mpi(int argc, char** argv, int rank, int size, const string& filename) {
    
    // Step 1: Rank 0 reads full file
    string full_text = "";
    if (rank == 0) {
        ifstream file(filename);
        stringstream buffer;
        buffer << file.rdbuf();
        full_text = buffer.str();
    }

    // Broadcast the file length
    int N = full_text.size();
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Rank 0 computes variable chunk sizes
    vector<int> sendcounts(size, 0), displs(size, 0);
    if (rank == 0) {
        int base = N / size;
        int start = 0;
        for (int r = 0; r < size; r++) {
            int end = (r == size - 1 ? N : start + base);
            while (end < N && full_text[end] != ' ' && full_text[end] != '\n')
                end++;
            sendcounts[r] = end - start;
            displs[r] = start;
            start = end;
        }
    }

    // Broadcast chunk info
    MPI_Bcast(sendcounts.data(), size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs.data(), size, MPI_INT, 0, MPI_COMM_WORLD);

    int local_size = sendcounts[rank];
    string local_chunk(local_size, ' ');
    
    // === NON-BLOCKING SEND/RECV ===
    MPI_Request recv_request = MPI_REQUEST_NULL;
    vector<MPI_Request> send_requests;
    
    if (rank == 0) {
        send_requests.resize(size, MPI_REQUEST_NULL);
        // Rank 0 copies its own data
        local_chunk = full_text.substr(displs[0], sendcounts[0]);
        // Non-blocking sends to other ranks
        for (int r = 1; r < size; r++) {
            MPI_Isend(full_text.data() + displs[r], sendcounts[r], MPI_CHAR,
                     r, 0, MPI_COMM_WORLD, &send_requests[r]);
        }
    } else {
        // Non-blocking receive
        MPI_Irecv(&local_chunk[0], local_size, MPI_CHAR,
                 0, 0, MPI_COMM_WORLD, &recv_request);
    }

    // Wait for receive to complete
    if (rank != 0) {
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    }

    // Local word count (computation can overlap with other ranks' communication)
    unordered_map<string,int> local_map;
    stringstream ss(local_chunk);
    string word;
    while (ss >> word)
        local_map[word]++;

    // Ensure all sends complete (avoid memory leaks)
    if (rank == 0) {
        for (int r = 1; r < size; r++) {
            MPI_Wait(&send_requests[r], MPI_STATUS_IGNORE);
        }
    }

    // Serialize local map
    string local_serial = "";
    for (auto &p : local_map)
        local_serial += p.first + " " + to_string(p.second) + "\n";
    int my_len = local_serial.size();

    // === NON-BLOCKING GATHER SIZES ===
    vector<int> recvcounts(size, 0);
    if (rank == 0) {
        recvcounts[0] = my_len;
        vector<MPI_Request> recv_size_reqs(size - 1);
        for (int r = 1; r < size; r++) {
            MPI_Irecv(&recvcounts[r], 1, MPI_INT, r, 1, MPI_COMM_WORLD, &recv_size_reqs[r-1]);
        }
        MPI_Waitall(size - 1, recv_size_reqs.data(), MPI_STATUSES_IGNORE);
    } else {
        MPI_Request send_size_req;
        MPI_Isend(&my_len, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &send_size_req);
        MPI_Wait(&send_size_req, MPI_STATUS_IGNORE);
    }

    // Prepare final buffer
    string final_buffer = "";
    vector<int> displs2(size, 0);
    if (rank == 0) {
        int offset = 0;
        for (int r = 0; r < size; r++) {
            displs2[r] = offset;
            offset += recvcounts[r];
        }
        final_buffer.resize(offset);
        copy(local_serial.begin(), local_serial.end(), final_buffer.begin());
    }

    // === NON-BLOCKING GATHER DATA ===
    if (rank == 0) {
        vector<MPI_Request> recv_data_reqs(size - 1);
        for (int r = 1; r < size; r++) {
            MPI_Irecv(&final_buffer[displs2[r]], recvcounts[r], MPI_CHAR,
                     r, 2, MPI_COMM_WORLD, &recv_data_reqs[r-1]);
        }
        // Communication and computation can overlap here
        MPI_Waitall(size - 1, recv_data_reqs.data(), MPI_STATUSES_IGNORE);
    } else {
        MPI_Request send_data_req;
        MPI_Isend(local_serial.data(), my_len, MPI_CHAR,
                 0, 2, MPI_COMM_WORLD, &send_data_req);
        MPI_Wait(&send_data_req, MPI_STATUS_IGNORE);
    }

    // Final merge and output
    if (rank == 0) {
        unordered_map<string,int> final_map;
        stringstream all(final_buffer);
        string w;
        int c;
        while (all >> w >> c)
            final_map[w] += c;

        cout << "===== NON-BLOCKING MPI WORD COUNT =====\n";
        int count = 0;
        for (auto &p : final_map) {
            cout << p.first << " -> " << p.second << "\n";
            if (++count >= 10) break;  // Show top 10
        }
    }

    return 0;
}


// ORIGINAL BLOCKING MPI IMPLEMENTATION 


int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) {
            cout << "Usage: mpiexec -n 4 ./wc_mpi.exe data.txt [--nonblocking]\n";
            cout << "  --nonblocking : Use non-blocking MPI )\n";
        }
        MPI_Finalize();
        return 0;
    }

    string filename = argv[1];
    
    // Check if non-blocking mode is requested
    bool use_nonblocking = false;
    if (argc >= 3 && string(argv[2]) == "--nonblocking") {
        use_nonblocking = true;
    }
    
    // Run non-blocking version if requested
    if (use_nonblocking) {
        int result = run_nonblocking_mpi(argc, argv, rank, size, filename);
        MPI_Finalize();
        return result;
    }
    
    // Otherwise continue with original blocking implementation below

    // Step 1: Rank 0 reads full file
    string full_text = "";
    if (rank == 0)
    {
        ifstream file(filename);
        stringstream buffer;
        buffer << file.rdbuf();
        full_text = buffer.str();
    }

    // Broadcast the file length
    int N = full_text.size();
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Rank 0 computes variable chunk sizes (don't cut words)
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

    // Scatter counts so each rank knows its size
    int local_size = 0;
    MPI_Scatter(sendcounts.data(), 1, MPI_INT,
                &local_size, 1, MPI_INT,
                0, MPI_COMM_WORLD);

    //  Receive the chunk
    string local_chunk(local_size, ' ');
    MPI_Scatterv(full_text.data(), sendcounts.data(), displs.data(), MPI_CHAR,
                 &local_chunk[0], local_size, MPI_CHAR,
                 0, MPI_COMM_WORLD);

    // Local word count
    unordered_map<string,int> local_map;
    stringstream ss(local_chunk);
    string word;
    while (ss >> word)
        local_map[word]++;


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
                &final_buffer[0], recvcounts.data(), displs2.data(), MPI_CHAR,
                0, MPI_COMM_WORLD);


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
