#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <thread>
#include <sstream>
using namespace std;

const int LIMIT = 1000'000;
const bool DELETE_TEMP_FILES = true;
const string DONATIONS_FILE = "Donations.csv";
const string DONORS_FILE = "Donors.csv";
const string RESULT_FILE = "Result.csv";
const string CSV_EXT = ".csv";
const string DELIM = ",";

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
    auto start = 0U;
    auto end = s.find(DELIM);
    while (end != std::string::npos) {
        result.push_back(s.substr(start, end - start));
        start = end + DELIM.length();
        end = s.find(DELIM, start);
    }

    result.push_back(s.substr(start, end));
}

void readColumns(ifstream& file, vector<string>& columns) {
    string s;
    getline(file, s);
    split(s, columns);
}

bool parseData(ifstream& file, vector<vector<string>>& data, int limit) {
    // TODO: get rid of slow ifstream
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
    return (data.size() != 0);
}

void findColumnIndexes(ifstream& stream, string columnName1, string columnName2, int& index1, int& index2) {
    vector<string> columns;
    readColumns(stream, columns);
    if (index1 == -1 && index2 == -1) {
        for (int i = 0; i < columns.size(); i++) {
            if (columns[i] == columnName1) {
                index1 = i;
            } else if (columns[i] == columnName2) {
                index2 = i;
            }
            if (index1 != -1 && index2 != -1) {
                break;
            }
        }
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

    findColumnIndexes(fileDonations, "Donation Amount", "Donor ID", amountIndex, donationsDonorIdIndex);

    while (true) {
        vector<vector<string>> data;
        if (!parseData(fileDonations, data, LIMIT)) {
            break;
        }

        unordered_map<string, int> amountById;
        for (int i = 0; i < data.size(); i++) {
            string& id = data[i][donationsDonorIdIndex];
            string& amountStr = data[i][amountIndex];
            amountById[id] += to_int(amountStr);
        }

        ofstream out(to_string(iter) + CSV_EXT);
        for (auto it : amountById) {
            out << it.first << "," << to_string(it.second) << '\n';
        }
        out.close();

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
        if (!parseData(file1, data1, LIMIT)) {
            break;
        }

        unordered_map<string, int> amountById;
        for (int i = 0; i < data1.size(); i++) {
            amountById[data1[i][0]] = to_int(data1[i][1]);
        }

        ifstream file2(f2);
        while (true) {
            vector<vector<string>> data2;
            if (!parseData(file2, data2, LIMIT)) {
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

    // Write all pairs (donor id, amount) from f1 to f2
    // TODO: copy file instead of writing line by line
    ifstream file1(f1);
    while (true) {
        vector<vector<string>> data;
        if (!parseData(file1, data, LIMIT)) {
            break;
        }

        for (int i = 0; i < data.size(); i++) {
            string& donorId = data[i][0];
            string& amount = data[i][1];
            output << donorId << ',' << amount << '\n';
        }
    }
    file1.close();

    // Write all pairs (donor id, amount) from f2 which are not presented in f1
    ifstream file2(f2);
    while (true) {
        vector<vector<string>> data2;
        if (!parseData(file2, data2, LIMIT)) {
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
            if (!parseData(file1, data1, LIMIT)) {
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

    if (DELETE_TEMP_FILES) {
        remove((out + ".left").c_str());
    }
}

void compress(string file, string out) {
    int donationsDonorIdIndex = -1;
    int amountIndex = -1;
    int iter = 0;



    ifstream file1(file);
    ofstream output(out);
    findColumnIndexes(file1, "Donation Amount", "Donor ID", amountIndex, donationsDonorIdIndex);

    while (true) {
        vector<vector<string>> data1;
        if (!parseData(file1, data1, LIMIT)) {
            break;
        }

        unordered_map<string, int> amountById;
        for (int i = 0; i < data1.size(); i++) {
            amountById[data1[i][donationsDonorIdIndex]] = 0;
        }

        vector<string> columns;
        {
            Timer timer("    delete elems which already processed ");
            ifstream file2(file);
            readColumns(file2, columns);
            for (int i = 0; i < iter; i++) {
                vector<vector<string>> data2;
                parseData(file2, data2, LIMIT);
                
                for (int i = 0; i < data2.size(); i++) {
                    string& donorId = data2[i][donationsDonorIdIndex];
                    if (amountById.find(donorId) != amountById.end()) {
                        amountById.erase(donorId);
                    }
                }
            }
            file2.close();
        }

        
        {
            Timer timer("    calc amount ");
            ifstream file2(file);
            readColumns(file2, columns);

            while (true) {
                vector<vector<string>> data2;
                if (!parseData(file2, data2, LIMIT)) {
                    break;
                }

                for (int i = 0; i < data2.size(); i++) {
                    string& donorId = data2[i][donationsDonorIdIndex];
                    if (amountById.find(donorId) != amountById.end()) {
                        amountById[donorId] += to_int(data2[i][amountIndex]);
                    }
                }
            }
            file2.close();
        }

        for (auto it : amountById) {
            output << it.first << "," << it.second << '\n';
        }
        iter++;
    }
    file1.close();
    output.close();

    if (DELETE_TEMP_FILES) {
        remove((out + ".left").c_str());
    }
}

// Doing like a merge sort, combine pairs and get 1 file containing unique pairs (Donor ID, amount)
string compressDonorAndAmountFiles(int filesCount) {
    vector<string> files;
    for (int i = 0; i < filesCount; i++) {
        files.push_back(to_string(i));
    }

    while (files.size() > 1) {
        vector<string> newFiles;
        vector<thread> jobs;
        for (int i = 0; i + 1 < files.size(); i += 2) {
            string newFile = files[i] + files[i + 1];
            jobs.push_back(thread(&::compressTwoFiles, files[i] + CSV_EXT, files[i + 1] + CSV_EXT, newFile + CSV_EXT));
            newFiles.push_back(newFile);
        }

        for (int i = 0; i < jobs.size(); i++) {
            jobs[i].join();
        }

        if (DELETE_TEMP_FILES) {
            for (int i = 0; i + 1 < files.size(); i += 2) {
                remove((files[i] + CSV_EXT).c_str());
                remove((files[i + 1] + CSV_EXT).c_str());
            }
        }

        if (files.size() % 2) {
            newFiles.push_back(files[files.size() - 1]);
        }
        files = newFiles;
    }

    return files[0] + CSV_EXT;
}

// Collect result file
void calculateAmountForState(string donationsFile, string donorsFile, string outputFile) {
    unordered_map<string, int> amountByState;

    int donorIdIndex = -1;
    int stateIndex = -1;


    ifstream donationsStream(donationsFile);
    while (true) {
        vector<vector<string>> donationsData;
        if (!parseData(donationsStream, donationsData, LIMIT)) {
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
        findColumnIndexes(donorsStream, "Donor State", "Donor ID", stateIndex, donorIdIndex);
        
        while (true) {
            vector<vector<string>> donorsData;
            if (!parseData(donorsStream, donorsData, LIMIT)) {
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

    // Sort result by state name
    vector<pair<string, int>> result;
    for (auto it : amountByState) {
        result.push_back({ it.first, it.second });
    }
    sort(result.begin(), result.end());

    // Write result into file
    ofstream output(outputFile);
    for (auto it : result) {
        output << it.first << "," << it.second << '\n';
    }
    output.close();

    if (DELETE_TEMP_FILES) {
        remove(donationsFile.c_str());
    }
}


int main() {

    Timer t("TOTAL ELAPSED TIME ");

    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    /*int filesCount;
    {
        Timer timer("createDonorAndAmountFiles ");
        filesCount = createDonorAndAmountFiles();
    }

    string fileName;
    {
        Timer timer("compressDonorAndAmountFiles ");
        fileName = compressDonorAndAmountFiles(filesCount);
    }*/

    string fileName = "compressed.csv";
    {
        Timer timer("compress ");
        compress(DONATIONS_FILE, fileName);
    }

    {
        Timer timer("calculateAmountForState ");
        calculateAmountForState(fileName, DONORS_FILE, RESULT_FILE);
    }


    return 0;
}