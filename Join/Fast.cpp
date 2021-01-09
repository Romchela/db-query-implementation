#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>
#include <chrono>
using namespace std;

const int LIMIT = 1000'000;
const bool DELETE_TEMP_FILES = true;
const string DONATIONS_FILE = "Donations.csv";
const string DONORS_FILE = "Donors.csv";

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

// Create files 0.csv, 1.csv .. so on
// Each file contains pair (Donor Id, Donation amount)
// Donors can be repeated in different files, but in each file it's unique
// Function returns nummber of files
int createDonorAndAmountFiles() {
    ifstream fileDonations(DONATIONS_FILE); // 10^10
    int donationsDonorIdIndex = -1;
    int amountIndex = -1;
    int iter = 0;
    bool first = true;

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


    while (true) {
        vector<vector<string>> data;
        parseData(fileDonations, data, LIMIT);
        if (data.size() == 0) {
            break;
        }

        unordered_map<string, int> amountById;
        for (int i = 0; i < data.size(); i++) {
            string id = data[i][donationsDonorIdIndex];
            string amountStr = data[i][amountIndex];
            int amount = to_int(amountStr);
            amountById[id] += amount;
        }

        ofstream fileView(to_string(iter) + ".csv");
        for (auto it : amountById) {
            fileView << it.first << "," << to_string(it.second) << '\n';
        }
        fileView.close();

        iter++;
    }

    fileDonations.close();
    return iter;
}

// Increment amount from f2 for each donor from f1 
void leftCompress(string f1, string f2, string out) {
    ifstream file1(f1);
    ofstream output(out);
    while (true) {
        vector<vector<string>> data1;
        parseData(file1, data1, LIMIT);
        if (data1.size() == 0) {
            break;
        }

        unordered_map<string, int> amountById;
        for (int i = 0; i < data1.size(); i++) {
            amountById[data1[i][0]] = to_int(data1[i][1]);
        }

        ifstream file2(f2);
        while (true) {
            vector<vector<string>> data2;
            parseData(file2, data2, LIMIT);
            if (data2.size() == 0) {
                break;
            }
            for (int i = 0; i < data2.size(); i++) {
                string& donorId = data2[i][0];
                if (amountById.find(donorId) != amountById.end()) {
                    amountById[donorId] += to_int(data2[i][1]);
                }
            }
        }
        file2.close();

        for (auto it : amountById) {
            output << it.first << "," << it.second << '\n';
        }
    }
    file1.close();
    output.close();
}

// Put donors from f1 and donors which not exist in f1 from f2
void rightCompress(string f1, string f2, string out) {
    ofstream output(out);

    ifstream file1(f1);
    while (true) {
        vector<vector<string>> data;
        parseData(file1, data, LIMIT);
        if (data.size() == 0) {
            break;
        }
        for (int i = 0; i < data.size(); i++) {
            string& donorId = data[i][0];
            string& amount = data[i][1];
            output << donorId << ',' << amount << '\n';
        }
    }
    file1.close();

    ifstream file2(f2);
    while (true) {
        vector<vector<string>> data2;
        parseData(file2, data2, LIMIT);
        if (data2.size() == 0) {
            break;
        }

        unordered_map<string, string> ids;
        for (int i = 0; i < data2.size(); i++) {
            string& donorId = data2[i][0];
            string& amount = data2[i][1];
            ids[donorId] = amount;
        }

        ifstream file1(f1);
        while (true) {
            vector<vector<string>> data1;
            parseData(file1, data1, LIMIT);
            if (data1.size() == 0) {
                break;
            }
            for (int i = 0; i < data1.size(); i++) {
                string& donorId = data1[i][0];
                if (ids.find(donorId) != ids.end()) {
                    ids.erase(donorId);
                }
            }
        }
        file1.close();

        for (auto it : ids) {
            output << it.first << "," << it.second << '\n';
        }
    }
    file2.close();
}

// Get 2 files and combine them
void compressTwoFiles(string f1, string f2, string out) {
    leftCompress(f1, f2, out + ".left");
    rightCompress(out + ".left", f2, out);

    if (DELETE_TEMP_FILES)
        remove((out + ".left").c_str());
}

// Doing like a merge sort, combine pairs and get 1 file containing unique pairs (Donor ID, amount)
string compressDonorAndAmountFiles(int filesCount) {
    vector<string> files;
    for (int i = 0; i < filesCount; i++) {
        files.push_back(to_string(i));
    }
    
    // TOOD: can be parallel like merge sort
    while (files.size() > 1) {
        vector<string> newFiles;
        for (int i = 0; i + 1 < files.size(); i += 2) {
            string newFile = files[i] + files[i + 1];
            compressTwoFiles(files[i] + ".csv", files[i + 1] + ".csv", newFile + ".csv");
            if (DELETE_TEMP_FILES) {
                remove((files[i] + ".csv").c_str());
                remove((files[i + 1] + ".csv").c_str());
            }
            newFiles.push_back(newFile);
        }
        if (files.size() % 2) {
            newFiles.push_back(files[files.size() - 1]);
        }
        files = newFiles;
    }

    return files[0] + ".csv";
}

// Collect result file
void calculateAmountForState(string donationsFile, string donorsFile, string outputFile) {
    unordered_map<string, int> amountByState;

    int donorIdIndex = -1;
    int stateIndex = -1;


    ifstream donationsStream(donationsFile);
    while (true) {
        vector<vector<string>> donationsData;
        parseData(donationsStream, donationsData, LIMIT);
        if (donationsData.size() == 0) {
            break;
        }
        unordered_map<string, int> amountById;
        unordered_map<string, string> stateById;
        for (int i = 0; i < donationsData.size(); i++) {
            string& donorId = donationsData[i][0];
            string& amount = donationsData[i][1];
            amountById[donorId] = to_int(amount);
        }

        ifstream donorsStream(donorsFile);

        vector<string> columns;
        readColumns(donorsStream, columns);
        if (stateIndex == -1) {
            for (int i = 0; i < columns.size(); i++) {
                if (columns[i] == "Donor State") {
                    stateIndex = i;
                }
                else if (columns[i] == "Donor ID") {
                    donorIdIndex = i;
                }
                if (donorIdIndex != -1 && stateIndex != -1) {
                    break;
                }
            }
        }

        while (true) {
            vector<vector<string>> donorsData;
            parseData(donorsStream, donorsData, LIMIT);
            if (donorsData.size() == 0) {
                break;
            }
            
            for (int i = 0; i < donorsData.size(); i++) {
                string& donorId = donorsData[i][donorIdIndex];
                string& state = donorsData[i][stateIndex];
                if (amountById.find(donorId) != amountById.end()) {
                    stateById[donorId] = state;
                }
            }
        }
        donorsStream.close();

        for (auto it : stateById) {
            amountByState[it.second] += amountById[it.first];
        }
    }
    donationsStream.close();

    ofstream output(outputFile);
    for (auto it : amountByState) {
        output << it.first << "," << it.second << '\n';
    }
    output.close();

    if (DELETE_TEMP_FILES)
        remove(donationsFile.c_str());
}


int main() {

    int filesCount;
    {
        Timer timer("createDonorAndAmountFiles ");
        filesCount = createDonorAndAmountFiles();
    }

    string fileName;
    {
        Timer timer("compressDonorAndAmountFiles ");
        fileName = compressDonorAndAmountFiles(filesCount);
    }
    
    {
        Timer timer("calculateAmountForState ");
        calculateAmountForState(fileName, DONORS_FILE, "result.csv");
    }
    

    return 0;
}