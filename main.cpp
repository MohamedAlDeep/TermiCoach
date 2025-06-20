#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <sqlite3.h>


using namespace std;
using json = nlohmann::json;

int CaloriesBurnt;

// Callback function to handle curl response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch(std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
}

// Callback function for SQL queries
static int callback(void* data, int argc, char** argv, char** colName) {
    for (int i = 0; i < argc; i++) {
        std::cout << colName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
    return 0;
}


void curlRunUp(CURL* curl){
    
    if (curl) {
        // Get user input
        cout << "Enter Your training: ";
        string training;
        getline(cin, training);
        
        // Set up the request URL
        string url = "https://ai.hackclub.com/chat/completions";
        
        // Create JSON request body
        json requestData = {
            {"messages", json::array({
                {{"role", "user"}, {"content", "I have done: " + training + " Give me the number of calories burnt (Only a single number, dont add any word)"}}
            })}
        };
        string requestBody = requestData.dump();
        
        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        
        // Set headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // Response handling
        string responseString;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        } else {
            // Parse JSON response
            try {
                json responseJson = json::parse(responseString);
                string calories = responseJson["choices"][0]["message"]["content"];
                
                // Print result
                CaloriesBurnt = stoi(calories);
                cout << CaloriesBurnt << " Calories" << endl;
            } catch (const exception& e) {
                cerr << "Error parsing response: " << e.what() << endl;
                cerr << "Response received: " << responseString << endl;
            }
        }
        
        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void dataBase(sqlite3* db, char* &errMsg) {
    // Open/Create database
    int rc = sqlite3_open("app.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        return;
    }

    // Create table
    const char* sql = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INT);";
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

    // Initialize curl
int main() {
    sqlite3* db;
    char* errMsg = 0;
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    curlRunUp(curl);
    curl_global_cleanup();

    // Initialize SQLite database
    dataBase(db, errMsg);

    

    // Close database
    sqlite3_close(db);
    return 0;
}