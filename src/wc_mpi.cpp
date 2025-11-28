#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
using namespace std;
//THE NON BLOCKING  IMPLEMENTATION

void merge_maps(unordered_map<string,int>& A,
                const unordered_map<string,int>& B)
{
    for (auto &p : B)
        A[p.first] += p.second;
}


   

void word_count_nonblocking(int rank, int size, const string& filename) {
    
    string full_text = "";
    if (rank == 0)
    {
        ifstream file(filename);
        stringstream buffer;
        buffer << file.rdbuf();
        full_text = buffer.str();
    }

    int N = full_text.size();
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> sendcounts(size, 0), displs(size, 0);

    if (rank == 0)
    {
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

    int local_size = 0;
    vector<MPI_Request> send_requests_counts, recv_requests_counts;
    
    if (rank == 0) {
        send_requests_counts.resize(size - 1);
        for (int r = 1; r < size; r++) {
            MPI_Isend(&sendcounts[r], 1, MPI_INT, r, 0, MPI_COMM_WORLD, 
                     &send_requests_counts[r - 1]);
        }
        local_size = sendcounts[0];
    } else {
        recv_requests_counts.resize(1);
        MPI_Irecv(&local_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, 
                 &recv_requests_counts[0]);
    }

    if (rank == 0) {
        for (int i = 0; i < size - 1; i++) {
            MPI_Wait(&send_requests_counts[i], MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Wait(&recv_requests_counts[0], MPI_STATUS_IGNORE);
    }

    string local_chunk(local_size, ' ');
    vector<MPI_Request> send_requests_data, recv_requests_data;
    
    if (rank == 0) {
        send_requests_data.resize(size - 1);
        for (int r = 1; r < size; r++) {
            MPI_Isend(&full_text[0] + displs[r], sendcounts[r], MPI_CHAR, r, 1, 
                     MPI_COMM_WORLD, &send_requests_data[r - 1]);
        }
        memcpy(&local_chunk[0], &full_text[0] + displs[0], sendcounts[0]);
        
        int completed = 0;
        vector<int> done_flags(size - 1, 0);
        while (completed < size - 1) {
            for (int i = 0; i < size - 1; i++) {
                if (!done_flags[i]) {
                    int flag = 0;
                    MPI_Test(&send_requests_data[i], &flag, MPI_STATUS_IGNORE);
                    if (flag) {
                        done_flags[i] = 1;
                        completed++;
                    }
                }
            }
        }
    } else {
        recv_requests_data.resize(1);
        MPI_Irecv(&local_chunk[0], local_size, MPI_CHAR, 0, 1, 
                 MPI_COMM_WORLD, &recv_requests_data[0]);
        
        int flag = 0;
        while (!flag) {
            MPI_Test(&recv_requests_data[0], &flag, MPI_STATUS_IGNORE);
        }
    }
    
    if (rank == 0) {
        for (int i = 0; i < size - 1; i++) {
            MPI_Wait(&send_requests_data[i], MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Wait(&recv_requests_data[0], MPI_STATUS_IGNORE);
    }

    unordered_map<string,int> local_map;
    stringstream ss(local_chunk);
    string word;
    while (ss >> word)
        local_map[word]++;

    string local_serial = "";
    for (auto &p : local_map)
        local_serial += p.first + " " + to_string(p.second) + "\n";

    int my_len = local_serial.size();

    vector<int> recvcounts(size, 0), displs2(size, 0);
    vector<MPI_Request> send_requests_size, recv_requests_size;
    
    if (rank == 0) {
        recvcounts[0] = my_len;
        recv_requests_size.resize(size - 1);
        for (int r = 1; r < size; r++) {
            MPI_Irecv(&recvcounts[r], 1, MPI_INT, r, 2, MPI_COMM_WORLD, 
                     &recv_requests_size[r - 1]);
        }
    } else {
        send_requests_size.resize(1);
        MPI_Isend(&my_len, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, 
                 &send_requests_size[0]);
    }

    if (rank == 0) {
        for (int i = 0; i < size - 1; i++) {
            MPI_Wait(&recv_requests_size[i], MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Wait(&send_requests_size[0], MPI_STATUS_IGNORE);
    }

    string final_buffer = "";
    if (rank == 0) {
        int offset = 0;
        for (int r = 0; r < size; r++)
        {
            displs2[r] = offset;
            offset += recvcounts[r];
        }
        final_buffer.resize(offset);
        memcpy(&final_buffer[0] + displs2[0], &local_serial[0], my_len);
    }

    vector<MPI_Request> send_requests_final, recv_requests_final;
    
    if (rank == 0) {
        recv_requests_final.resize(size - 1);
        for (int r = 1; r < size; r++) {
            MPI_Irecv(&final_buffer[0] + displs2[r], recvcounts[r], MPI_CHAR, r, 3, 
                     MPI_COMM_WORLD, &recv_requests_final[r - 1]);
        }
        
        int completed = 0;
        vector<int> done_flags(size - 1, 0);
        while (completed < size - 1) {
            for (int i = 0; i < size - 1; i++) {
                if (!done_flags[i]) {
                    int flag = 0;
                    MPI_Test(&recv_requests_final[i], &flag, MPI_STATUS_IGNORE);
                    if (flag) {
                        done_flags[i] = 1;
                        completed++;
                    }
                }
            }
        }
        
        for (int i = 0; i < size - 1; i++) {
            MPI_Wait(&recv_requests_final[i], MPI_STATUS_IGNORE);
        }
    } else {
        send_requests_final.resize(1);
        MPI_Isend(&local_serial[0], my_len, MPI_CHAR, 0, 3, 
                 MPI_COMM_WORLD, &send_requests_final[0]);
        
        int flag = 0;
        while (!flag) {
            MPI_Test(&send_requests_final[0], &flag, MPI_STATUS_IGNORE);
        }
        
        MPI_Wait(&send_requests_final[0], MPI_STATUS_IGNORE);
    }

    if (rank == 0)
    {
        unordered_map<string,int> final_map;

        stringstream all(final_buffer);
        string w;
        int c;

        while (all >> w >> c)
            final_map[w] += c;

        cout << "===== FINAL WORD COUNT (MPI Non-Blocking) =====\n";
        for (auto &p : final_map)
            cout << p.first << " -> " << p.second << "\n";
    }
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0)
            cout << "Usage: mpiexec -n 4 ./wc_mpi.exe data.txt [--nonblocking]\n";
        MPI_Finalize();
        return 0;
    }

    string filename = argv[1];
    bool use_nonblocking = false;
    
    // Check for non-blocking flag
    if (argc >= 3 && string(argv[2]) == "--nonblocking") {
        use_nonblocking = true;
    }

    // Use non-blocking implementation if requested
    if (use_nonblocking) {
        word_count_nonblocking(rank, size, filename);
        MPI_Finalize();
        return 0;
    }





    
//the blocking implementation

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
    MPI_Scatterv(&full_text[0], sendcounts.data(), displs.data(), MPI_CHAR,
                 &local_chunk[0], local_size, MPI_CHAR,
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
    MPI_Gatherv(&local_serial[0], my_len, MPI_CHAR,
                &final_buffer[0], recvcounts.data(), displs2.data(), MPI_CHAR,
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
