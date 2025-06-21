#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <sqlite3.h>


using namespace std;
using json = nlohmann::json;

string CaloriesBurnt;

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


void curlRunUp(CURL* curl, string training){
    
    if (curl) {
        // Get user input
        
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
                CaloriesBurnt = calories;
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

// Change the function signature to return the db pointer

sqlite3* dataBase(char* &errMsg) {
    sqlite3* db;
    
    // Open/Create database
    int rc = sqlite3_open("app.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        return nullptr;
    }

    // Create table
    const char* sql = "CREATE TABLE IF NOT EXISTS user (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INT, weight REAL, height REAL);";
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }

    const char* sql2 = "CREATE TABLE IF NOT EXISTS workouts ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                       "name TEXT, "
                       "reps INT, "
                       "sets INT,"
                       "calories REAL);";
    rc = sqlite3_exec(db, sql2, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
    return db;
}
void InsertUser(sqlite3* db, const string& name, int age, float weight, float height) {
    // Insert data
    int rc;
    char* errMsg = 0;
    string sql = "INSERT INTO user (name, age, weight, height) VALUES ('" + name + "', " + to_string(age) + ", " + to_string(weight) + ", " + to_string(height) + ");";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}
void InsertWorkout(sqlite3* db, const string& name, string reps, string sets, string calories) {
    int rc;
    char* errMsg = 0;
    int repsInt = stoi(reps);
    int setsInt = stoi(sets);
    float caloriesFloat = stof(calories);
    string sql = "INSERT INTO workouts (name, reps, sets, calories) VALUES ('" + name + "', " + to_string(repsInt) + ", " + to_string(setsInt) + ", " + to_string(caloriesFloat) + ");";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Workout inserted successfully!" << endl;
    }
}
void ShowLastWorkout(sqlite3* db) {
    const char* sql = "SELECT name, reps, sets, calories, timestamp FROM workouts ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cout << "Last Workout:\n";
            cout << "Name: " << (const char*)sqlite3_column_text(stmt, 0) << endl;
            cout << "Reps: " << sqlite3_column_int(stmt, 1) << endl;
            cout << "Sets: " << sqlite3_column_int(stmt, 2) << endl;
            cout << "Calories: " << sqlite3_column_double(stmt, 3) << endl;
            cout << "Timestamp: " << (const char*)sqlite3_column_text(stmt, 4) << endl << endl;
        } else {
            cout << "No workouts found.\n";
        }
        sqlite3_finalize(stmt);
    } else {
        cout << "Failed to query last workout.\n";
    }
}

void checkUser(sqlite3* db) {
    int userCount = 0;
    const char* checkUserSql = "SELECT COUNT(*) FROM user;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, checkUserSql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            userCount = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    if (userCount == 0) {
        string name;
        int age;
        float weight, height;
        cout << "Enter your name: ";
        cin.ignore(); // Ensure input buffer is clear before getline
        getline(cin, name);
        cout << "Enter your age: ";
        cin >> age;
        cout << "Enter your weight (kg): ";
        cin >> weight;
        cout << "Enter your height (cm): ";
        cin >> height;
        cout << "\n";
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear input buffer
        InsertUser(db, name, age, weight, height);
    }
}

void ExportToCSV(sqlite3* db, const string& filename) {
    const char* sql = "SELECT * FROM workouts;";
    sqlite3_stmt* stmt;
    FILE* file = fopen(filename.c_str(), "w");
    if (!file) {
        cerr << "Failed to open file for writing: " << filename << endl;
        return;
    }

    // Write CSV header
    fprintf(file, "id,timestamp,name,reps,sets,calories\n");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* timestamp = sqlite3_column_text(stmt, 1);
            const unsigned char* name = sqlite3_column_text(stmt, 2);
            int reps = sqlite3_column_int(stmt, 3);
            int sets = sqlite3_column_int(stmt, 4);
            double calories = sqlite3_column_double(stmt, 5);

            fprintf(file, "%d,\"%s\",\"%s\",%d,%d,%.2f\n",
                id,
                timestamp ? (const char*)timestamp : "",
                name ? (const char*)name : "",
                reps,
                sets,
                calories
            );
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Failed to query workouts for export.\n";
    }
    fclose(file);
    cout << "Exported workouts to " << filename << endl;
}

void welcomeMessage(){
    // Print "Welcome to TermiCoach" in bold red, slightly smaller ASCII art
    cout << "\033[1;31m"; // Bold red
    cout << R"(
    
    

  _______                  _  _____                 _     
 |__   __|                (_)/ ____|               | |    
    | | ___ _ __ _ __ ___  _| |     ___   __ _  ___| |__  
    | |/ _ \ '__| '_ ` _ \| | |    / _ \ / _` |/ __| '_ \ 
    | |  __/ |  | | | | | | | |___| (_) | (_| | (__| | | |
    |_|\___|_|  |_| |_| |_|_|\_____\___/ \__,_|\___|_| |_|
                                                          
                                                          

)" << endl;
    cout << "\033[0m"; // Reset color
    cout << "Your personal fitness assistant." << endl;
    cout << "Let's get started with your workout! \n\n" << endl;
}



int main() {
    welcomeMessage();
   
    char* errMsg = 0;

    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    
    // Initialize SQLite database
    sqlite3* db = dataBase(errMsg);
    if (db == nullptr) {
        return 1;  // Exit with error
    }
    checkUser(db);
    
    while(true){
        int choice;
        cout << "1. Insert a Workout" << endl <<  
            "2. View dashboard" << endl <<
            "3. View last workout" << endl <<
            "4. Edit user data" << endl <<
            "5. Export user data (CSV)" << endl <<
            "6. Exit" << endl;

        cin >> choice;
        if(choice < 1 || choice > 6) {
            cout << "Invalid choice. Please try again." << endl;
            continue; // Prompt user again
        } else if(choice == 1) {
            string name, reps, sets, calories;
            cout << "Enter the following data" << endl;
            cout << "Enter workout name: ";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear input buffer
            getline(cin, name);
            cout << endl;
            cout << "Enter number of reps: ";
            getline(cin, reps);
            cout << endl;
            cout << "Enter number of sets: ";
            getline(cin, sets);
            curlRunUp(curl, name + " " + reps + " " + sets);
            InsertWorkout(db, name, reps, sets, CaloriesBurnt); // Example workout
        } else if (choice == 2) {
            cout << "View dashboard feature not implemented yet." << endl;
        } else if (choice == 3) {
            ShowLastWorkout(db);
        } else if (choice == 4) {
            cout << "Edit user data feature not implemented yet." << endl;
        } else if (choice == 5) {
            string filename;
            cout << "Enter filename to export workouts (e.g., workouts.csv): ";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, filename);
            ExportToCSV(db, filename);
        } else if (choice == 6) {
            cout << "Exiting TermiCoach. Stay fit!" << endl;
            break; // Exit the loop
        }
       

    }
    // Check for user workout input
    


    // Cleanup Curl
    curl_global_cleanup();
    // Close database
    sqlite3_close(db);
    return 0;
}