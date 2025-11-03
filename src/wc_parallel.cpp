#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

static inline bool is_word_char(unsigned char c) { return std::isalnum(c); }
static inline char to_lower_char(unsigned char c) { return (char)std::tolower(c); }

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.txt> [topN]\n";
        return 1;
    }

    string path = argv[1];
    int topN = (argc >= 3 ? max(1, stoi(argv[2])) : 10);

    ifstream fin(path);
    if (!fin) {
        cerr << "Error: cannot open file: " << path << "\n";
        return 1;
    }
    vector<string> lines;
    string line;
    while (getline(fin, line)) lines.push_back(move(line));

    double t_total_start = omp_get_wtime();   

    int n = lines.size();
    int max_threads = omp_get_max_threads();
    vector<unordered_map<string, long long>> local_maps(max_threads);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        auto &mp = local_maps[tid];
        mp.reserve(1 << 14);

        #pragma omp for schedule(static)
        for (int i = 0; i < n; ++i) {
            const string &ln = lines[i];
            string token; token.reserve(32);
            for (unsigned char c : ln) {
                if (is_word_char(c)) token.push_back(to_lower_char(c));
                else if (!token.empty()) { ++mp[token]; token.clear(); }
            }
            if (!token.empty()) { ++mp[token]; token.clear(); }
        }
    }

    unordered_map<string, long long> freq;
    freq.reserve(1 << 16);
    long long total_tokens = 0;

    for (auto &mp : local_maps) {
        for (auto &kv : mp) {
            freq[kv.first] += kv.second;
            total_tokens += kv.second;
        }
    }

    double t_total_end = omp_get_wtime();  
    double T_total = t_total_end - t_total_start;

    vector<pair<string, long long>> items(freq.begin(), freq.end());
    if (topN > (int)items.size()) topN = (int)items.size();
    nth_element(items.begin(), items.begin() + topN, items.end(),
                [](auto &a, auto &b){ return a.second > b.second; });
    sort(items.begin(), items.begin() + topN,
         [](auto &a, auto &b){ return a.second > b.second; });

    cout << "MODE=parallel\n";
    cout << "THREADS=" << omp_get_max_threads() << "\n";
    cout << "TOKENS=" << total_tokens << "\n";
    cout << "UNIQUE=" << freq.size() << "\n";
    cout << "T_total(s)=" << T_total << "\n";   
    cout << "TOP" << topN << ":\n";
    for (int i = 0; i < topN; ++i) {
        cout << items[i].first << " -> " << items[i].second << "\n";
    }

    return 0;
}