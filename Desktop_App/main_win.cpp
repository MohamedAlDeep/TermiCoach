#include <iostream>
#include <string>
#include <random> // For random number generation instead of API call
#include <ctime>  // For time-based random seed
#include <algorithm> // For transform
#include "json.hpp"
#include "win-deps/sqlite-amalgamation-3430000/sqlite3.h"
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using json = nlohmann::json;

string CaloriesBurnt;

// Random calorie calculator for Windows version (since we're not using curl)
string calculateCalories(const string& workout, int reps, int sets) {
    // Simple calorie calculator based on workout type, reps and sets
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Add time-based seed for better randomness
    gen.seed(static_cast<unsigned int>(time(nullptr)));
    
    // Base calories per rep based on workout type
    int baseCalories = 5;  // default base
    
    // Increase base calories for known workout types
    string workoutLower = workout;
    transform(workoutLower.begin(), workoutLower.end(), workoutLower.begin(), ::tolower);
    
    if (workoutLower.find("pushup") != string::npos || workoutLower.find("push up") != string::npos) {
        baseCalories = 8;
    } else if (workoutLower.find("squat") != string::npos) {
        baseCalories = 12;
    } else if (workoutLower.find("burpee") != string::npos) {
        baseCalories = 15;
    } else if (workoutLower.find("plank") != string::npos) {
        baseCalories = 10;
    } else if (workoutLower.find("situp") != string::npos || workoutLower.find("sit up") != string::npos) {
        baseCalories = 7;
    }
    
    // Calculate total calories with some randomness
    std::uniform_int_distribution<> distr(baseCalories - 2, baseCalories + 2);
    int caloriesPerRep = distr(gen);
    
    int totalCalories = caloriesPerRep * reps * sets;
    
    return to_string(totalCalories);
}

// Callback function for SQL queries
static int callback(void* data, int argc, char** argv, char** colName) {
    for (int i = 0; i < argc; i++) {
        std::cout << colName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
    return 0;
}


void calculateWorkoutCalories(const string& training) {
    // Parse the training string - assuming format like "pushups 10 3" (exercise, reps, sets)
    string exercise = training;
    string reps = "10";
    string sets = "3";
    
    // Try to parse reps and sets from the training string
    size_t lastSpace = training.find_last_of(' ');
    if (lastSpace != string::npos) {
        sets = training.substr(lastSpace + 1);
        string remaining = training.substr(0, lastSpace);
        lastSpace = remaining.find_last_of(' ');
        if (lastSpace != string::npos) {
            reps = remaining.substr(lastSpace + 1);
            exercise = remaining.substr(0, lastSpace);
        }
    }
    
    // Calculate calories
    try {
        int repsInt = stoi(reps);
        int setsInt = stoi(sets);
        CaloriesBurnt = calculateCalories(exercise, repsInt, setsInt);
        cout << CaloriesBurnt << " Calories" << endl;
    } catch (const exception& e) {
        cerr << "Error calculating calories: " << e.what() << endl;
        CaloriesBurnt = "100"; // Default fallback
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

void welcomeMessage(){
    #ifdef _WIN32
    // For Windows, set console code page to UTF-8 so console knows how to interpret string data
    SetConsoleOutputCP(CP_UTF8);
    #endif
    
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

// Function to manually download and open the dashboard HTML file
void DownloadAndOpenDashboard() {
    const std::string filename = "dashboard.html";
    const std::string dashboardContent = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TermiCoach Dashboard</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        h1 { color: #e74c3c; }
        .workout-list { margin-top: 20px; }
        .workout-item { border-bottom: 1px solid #eee; padding: 10px 0; }
        .workout-item:last-child { border-bottom: none; }
        .stats { display: flex; margin-top: 20px; }
        .stat-box { flex: 1; padding: 15px; background-color: #f9f9f9; margin-right: 10px; border-radius: 5px; text-align: center; }
        .stat-box:last-child { margin-right: 0; }
        .stat-value { font-size: 24px; font-weight: bold; color: #e74c3c; }
        .stat-label { font-size: 14px; color: #777; }
    </style>
</head>
<body>
    <div class="container">
        <h1>TermiCoach Dashboard</h1>
        <p>This is a simple dashboard for TermiCoach. For a more detailed view, please load your workout data.</p>
        
        <div class="stats">
            <div class="stat-box">
                <div class="stat-value">--</div>
                <div class="stat-label">Total Workouts</div>
            </div>
            <div class="stat-box">
                <div class="stat-value">--</div>
                <div class="stat-label">Total Calories</div>
            </div>
            <div class="stat-box">
                <div class="stat-value">--</div>
                <div class="stat-label">Avg. Calories/Workout</div>
            </div>
        </div>
        
        <div class="workout-list">
            <h2>Recent Workouts</h2>
            <p>Export your data and reload this page to see your workout history.</p>
        </div>
    </div>
    
    <script>
        document.addEventListener('DOMContentLoaded', function() {
            console.log('Dashboard loaded');
            // This is a placeholder for future dashboard functionality
            // In a future version, this could load the CSV data and display it
        });
    </script>
</body>
</html>)";

    // Create the dashboard HTML file
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // Write the content to the file
    fwrite(dashboardContent.c_str(), sizeof(char), dashboardContent.length(), file);
    fclose(file);

    // Open the file using the default browser
    #ifdef _WIN32
    ShellExecuteA(NULL, "open", filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
    #else
    std::string command = "xdg-open " + filename;
    system(command.c_str());
    #endif

    std::cout << "Dashboard opened in your default browser." << std::endl;
}

int main() {
    welcomeMessage();
   
    char* errMsg = 0;

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
            "5. Exit" << endl;

        cin >> choice;
        if(choice < 1 || choice > 5) {
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
            calculateWorkoutCalories(name + " " + reps + " " + sets);
            InsertWorkout(db, name, reps, sets, CaloriesBurnt);
        } else if (choice == 2) {
            DownloadAndOpenDashboard();
        } else if (choice == 3) {
            ShowLastWorkout(db);
        } else if (choice == 4) {
            cout << "Edit user data feature not implemented yet." << endl;
        } else if (choice == 5) {
            cout << "Exiting TermiCoach. Stay fit!" << endl;
            break; // Exit the loop
        }
       

    }
    // Close database
    sqlite3_close(db);
    return 0;
}
