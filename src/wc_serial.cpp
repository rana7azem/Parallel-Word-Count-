#include <bits/stdc++.h>
#include <omp.h> 
using namespace std;

static inline bool is_word_char(unsigned char c){ return std::isalnum(c); }
static inline char lowerc(unsigned char c){ return (char)std::tolower(c); }

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc < 2){
        cerr << "Usage: " << argv[0] << " <input.txt>\n";
        return 1;
    }
    const string path = argv[1];
    ifstream fin(path);
    if(!fin){ cerr << "Error: cannot open " << path << "\n"; return 1; }

    unordered_map<string,long long> freq;
    freq.reserve(1<<15);

    string line, token;

    double t_total_start = omp_get_wtime(); 
    double t_comp_start = omp_get_wtime();  

    while(getline(fin, line)){
        token.clear();
        for(unsigned char c : line){
            if(is_word_char(c)) token.push_back(lowerc(c));
            else if(!token.empty()){ ++freq[token]; token.clear(); }
        }
        if(!token.empty()){ ++freq[token]; token.clear(); }
    }

    double t_comp_end = omp_get_wtime();   
    double t_total_end = omp_get_wtime();  

    double T_total = t_total_end - t_total_start; 
    double T_comp  = t_comp_end - t_comp_start;   
    double T_sync  = 0.0;                         
    double T_inter = T_total - (T_comp + T_sync);


    cout << "MODE=serial\n";;
    cout << "T_total(s)=" << T_total << "\n";
    cout << "T_comp(s)="  << T_comp  << "\n";
    cout << "T_sync(s)="  << T_sync  << "\n";
    cout << "T_inter(s)=" << T_inter << "\n";

   
    vector<pair<string,long long>> items(freq.begin(), freq.end());
    sort(items.begin(), items.end(),
         [](auto &a, auto &b){ return a.first < b.first; });

    int printed = 0;
    for(auto &kv : items){
        cout << kv.first << " -> " << kv.second << "\n";
        if(++printed >= 10) break;
    }
    return 0;
}