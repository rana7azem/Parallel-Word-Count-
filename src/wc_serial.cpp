#include <bits/stdc++.h>
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
    while(getline(fin, line)){
        token.clear();
        for(unsigned char c : line){
            if(is_word_char(c)) token.push_back(lowerc(c));
            else if(!token.empty()){ ++freq[token]; token.clear(); }
        }
        if(!token.empty()){ ++freq[token]; token.clear(); }
    }

    vector<pair<string,long long>> items(freq.begin(), freq.end());
    sort(items.begin(), items.end(),
         [](auto &a, auto &b){ return a.first < b.first; });

    for(auto &kv : items){
        cout << kv.first << " -> " << kv.second << "\n";
    }
    return 0;
}
