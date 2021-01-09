#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>
#include <chrono>
#include <algorithm>
using namespace std;

const int LIMIT = 1000'000;
const bool DELETE_TEMP_FILES = true;
const string DONATIONS_FILE = "Donations.csv";
const string DONORS_FILE = "Donors.csv";
const string RESULT_FILE = "Result.csv";

// Class which calculate execution time. 
// When is's created it saves now().
// When class will be desctructed it prints time difference.
class Timer {
private:
    typedef std::chrono::high_resolution_clock clock_;
    std::chrono::time_point<clock_> start;
    std::string message;
    int count;

public:
    explicit Timer(std::string message) : start(clock_::now()), message(std::move(message)), count(1) {}
    explicit Timer(std::string message, int count) : start(clock_::now()), message(std::move(message)), count(count) {}
    ~Timer() {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock_::now() - start).count() / count;
        std::cout << message << elapsed << " ms" << std::endl;
    }
};

inline int to_int(string& s) {
    return s == "" ? 0 : stoi(s);
}

void split(string& s, vector<string>& result) {
    string cur = "";
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == ',') {
            result.push_back(cur);
            cur = "";
        }
        else {
            cur += s[i];
        }
    }
    if (cur.size() > 0) {
        result.push_back(cur);
    }
}

void readColumns(ifstream& file, vector<string>& columns) {
    string s;
    getline(file, s);
    split(s, columns);
}

void parseData(ifstream& file, vector<vector<string>>& data, int limit) {
    string s;
    while (!file.eof() && data.size() < limit) {
        getline(file, s);
        if (s.size() == 0) {
            continue;
        }
        vector<string> v;
        split(s, v);
        data.push_back(v);
    }
}


int main() {
    Timer timer("elapsed time: ");
    int donationsDonorIdIndex = -1;
    int amountIndex = -1;
    int donorIdIndex = -1;
    int stateIndex = -1;
    

    ifstream fileDonations(DONATIONS_FILE);

    // Read columns information from donation file
    vector<string> columns;
    readColumns(fileDonations, columns);
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i] == "Donation Amount") {
            amountIndex = i;
        }
        else if (columns[i] == "Donor ID") {
            donationsDonorIdIndex = i;
        }
        if (amountIndex != -1 && donationsDonorIdIndex != -1) {
            break;
        }
    }

    unordered_map<string, int> amountByState;
    unordered_map<string, string> stateByDonorId;
    int iter = 0;
    while (true) {
        vector<vector<string>> data;
        parseData(fileDonations, data, LIMIT);
        if (data.size() == 0) {
            break;
        }
        
        std::cout << "Iter = " << iter++ << std::endl;

        for (int i = 0; i < data.size(); i++) {
            string donationDonorId = data[i][donationsDonorIdIndex];
            string amountStr = data[i][amountIndex];
            int amount = to_int(amountStr);
            
            // Check if we know state of the donor (contains in cache), if not go through donator file and find the state
            if (stateByDonorId.find(donationDonorId) == stateByDonorId.end()) {
                ifstream donorsStream(DONORS_FILE);

                vector<string> donorColumns;
                readColumns(donorsStream, donorColumns);
                if (stateIndex == -1) {
                    for (int i = 0; i < donorColumns.size(); i++) {
                        if (donorColumns[i] == "Donor State") {
                            stateIndex = i;
                        }
                        else if (donorColumns[i] == "Donor ID") {
                            donorIdIndex = i;
                        }
                        if (donorIdIndex != -1 && stateIndex != -1) {
                            break;
                        }
                    }
                }

                bool found = false;
                while (true) {
                    vector<vector<string>> donorsData;
                    parseData(donorsStream, donorsData, LIMIT);
                    if (donorsData.size() == 0) {
                        break;
                    }

                    for (int i = 0; i < donorsData.size(); i++) {
                        string& donorId = donorsData[i][donorIdIndex];
                        string& state = donorsData[i][stateIndex];
                        if (donorId == donationDonorId) {
                            if (stateByDonorId.size() >= LIMIT) {
                                stateByDonorId.erase(stateByDonorId.begin());
                            }
                            stateByDonorId[donorId] = state;
                            found = true;
                        }

                        if (stateByDonorId.size() < LIMIT) {
                            stateByDonorId[donorId] = state;
                        }

                        if (stateByDonorId.size() >= LIMIT && found) {
                            break;
                        }
                    }
                }
                donorsStream.close();

            }

            amountByState[stateByDonorId[donationDonorId]] += amount;
        }
    }

    fileDonations.close();

    vector<pair<string, int>> result;
    for (auto it : amountByState) {
        result.push_back(make_pair(it.first, it.second));
    }
    sort(result.begin(), result.end());

    ofstream output(RESULT_FILE);
    for (auto it : result) {
        output << it.first << "," << it.second << '\n';
    }
    output.close();

    return 0;
}