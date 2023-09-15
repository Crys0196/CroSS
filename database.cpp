#include <iostream>
#include <sqlite3.h> // SQLite的C/C++接口头文件
#include <string>

#include <algorithm>
#include <sstream>
#include <fstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <cmath>
#include <stack>
#include <iomanip>
#include "stdio.h"
#include <math.h>

using namespace std;

// 关于数据库的相关变量
sqlite3 *db;
// int j = 100;//这个数字需要储存到数据库中，以应对变化
std::string combinedString; // 定义的全局变量，用于完成新一次的这个表格名字的确立。
std::string table = "Route";
double threshold = 0.000001; // dtw距离的阈值

// 关于文件的相关变量
string filename = "./resultset/T1.txt"; // 数据集地址

std::string combineStrings(const std::string &table, int j)
{
    return table + std::to_string(j);
}

// 关于比较的相关变量

// 定义数据类型(DTW)
typedef std::vector<std::pair<double, double>> Path;

// 计算两个点之间的距离(DTW)
double distanceBetweenPoints(const std::pair<double, double> &p1, const std::pair<double, double> &p2)
{
    double dx = p2.first - p1.first;
    double dy = p2.second - p1.second;
    return std::sqrt(dx * dx + dy * dy);
}

// 创建新表格
void createNewTable(const char *tableName, sqlite3 *db)
{

    char *errMsg = 0;

    int rc = sqlite3_open("database.db", &db);

    // 创建一个SQL语句来创建新表格
    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS " + std::string(tableName) + " ("
                                                                                          "id INTEGER PRIMARY KEY,"
                                                                                          "feature1 TEXT,"
                                                                                          "feature2 TEXT,"
                                                                                          "feature3 TEXT,"
                                                                                          "feature4 TEXT,"
                                                                                          "score REAL,"
                                                                                          "valid REAL," // 用于判断是否为真实道路
                                                                                          "repetition INTEGER);";
    // 执行创建新表格的SQL语句

    rc = sqlite3_exec(db, createTableSQL.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL错误for createnewtable: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else
    {
        std::cout << "新表格 " << tableName << " 创建完成。" << std::endl;
    }

    // 其表格对应的相关的点的表格
    std::string createTableSQLpoints = "CREATE TABLE IF NOT EXISTS " + std::string(tableName) + std::string("points") + " ("
                                                                                                                        "point_id INTEGER PRIMARY KEY,"
                                                                                                                        "class_a_id INTEGER,"
                                                                                                                        "longitude REAL,"
                                                                                                                        "latitude REAL);";

    //"FOREIGN KEY (class_a_id) REFERENCES ClassA (id));";提供外键，但我可以用名字上的关系去实现这个程序？
    //
    int rc1 = sqlite3_exec(db, createTableSQLpoints.c_str(), 0, 0, &errMsg);
    if (rc1 != SQLITE_OK)
    {
        std::cerr << "SQL错误for classpointsnewtable: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else
    {
        std::cout << "新点基表格 " << tableName << "points"
                  << " 创建完成。" << std::endl;
    }

    sqlite3_close(db);
}

// 获取当前表格顺序
int getorder(sqlite3 *db)
{ // 这个函数执行一次那个整数就会增加1,所以必须和增加次数的绑定才能完成。

    int rc = sqlite3_open("database.db", &db); // "database.db"是数据库文件名
    int a;

    if (rc)
    {
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    // 创建表（如果不存在）
    const char *createTableQuery = "CREATE TABLE IF NOT EXISTS IntegerTable (value INTEGER);";
    rc = sqlite3_exec(db, createTableQuery, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Error creating table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return -1;
    }

    // 存储整数1（如果表中还没有数据）
    const char *insertQuery = "INSERT OR IGNORE INTO IntegerTable (value) VALUES (1);";
    rc = sqlite3_exec(db, insertQuery, 0, 0, 0);

    // 从数据库中读取整数值
    const char *selectQuery = "SELECT value FROM IntegerTable;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, selectQuery, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Error preparing select statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        int retrievedValue = sqlite3_column_int(stmt, 0);
        a = retrievedValue;
    }
    else
    {
        std::cerr << "Error retrieving value: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return a;
} // 获取当前的顺序，以便于组合

// 更新表格的顺序
void updateorder(sqlite3 *db)
{

    int rc = sqlite3_open("database.db", &db); // "database.db"是数据库文件名

    if (rc)
    {
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // 从数据库中读取整数值
    const char *selectQuery = "SELECT value FROM IntegerTable;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, selectQuery, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Error preparing select statement: " << sqlite3_errmsg(db) << std::endl;
        // sqlite3_close(db);
        std::cout << "Retrieved Integer Value:是我 " << std::endl;
        return;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        int retrievedValue = sqlite3_column_int(stmt, 0);
        std::cout << "Retrieved Integer Value: " << retrievedValue << std::endl;

        // 增加整数值
        int newValue = retrievedValue + 1;

        // 更新数据库中的整数值
        const char *updateQuery = "UPDATE IntegerTable SET value = ?;";
        rc = sqlite3_prepare_v2(db, updateQuery, -1, &stmt, 0);

        if (rc != SQLITE_OK)
        {
            std::cerr << "Error preparing update statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        sqlite3_bind_int(stmt, 1, newValue);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "Error executing update statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "Error retrieving value: " << sqlite3_errmsg(db) << std::endl;
    }
    sqlite3_close(db);
} // 每次运行一次后需要跑一次数据，更新创建新表格的名字

void IncreaseIntegers(sqlite3 *db)
{

    int rc = sqlite3_open("database.db", &db);

    if (rc)
    {
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    const char *updateQuery = "UPDATE IntegerTable SET value = value + 1;";
    char *errorMsg = nullptr;

    rc = sqlite3_exec(db, updateQuery, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
    }
    else
    {
        std::cout << "Integer incremented and database updated successfully." << std::endl;
    }

    sqlite3_close(db);
}

// 读取文本行数
int CountLines(string filename)
{
    ifstream ReadFile;
    int n = 0;
    string tmp;
    ReadFile.open(filename.c_str()); // ios::in 表示以只读的方式读取文件
    if (ReadFile.fail())             // 文件打开失败:返回0
    {
        return 0;
    }
    else // 文件存在
    {
        while (getline(ReadFile, tmp, '\n'))
        {
            n++;
        }
        ReadFile.close();
        return n;
    }
}

// 决定是插入还是更新；
int checkidExists(const char *tableName)
{

    int rc = sqlite3_open("database.db", &db);

    int idToCheck = 1;
    if (rc)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 0;
    }

    bool recordExists = false;

    std::string selectSQL = "SELECT COUNT(*) FROM " + std::string(tableName) + " WHERE id = ?;";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, idToCheck);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        int rowCount = sqlite3_column_int(stmt, 0);
        if (rowCount > 0)
        {
            recordExists = 1;
        }
    }
    else
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return recordExists;
}

// 更新class的特征信息，这里分成几个部分进行更新？
void updateclassscore(const char *tableName, double scorevalue, sqlite3 *db)
{

    // 打开或创建数据库文件
    int rc = sqlite3_open("database.db", &db);

    if (rc)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        std::cout << "Error for insertclassscore!" << std::endl;
    }

    int idToInsert = 1; // 设置要插入的ID
    int idexist = checkidExists(tableName);

    if (idexist == 1)
    {
        int idToUpdate = 1; // 要更新的记录的Id
        std::string updateSQL = "UPDATE " + std::string(tableName) + " SET "
                                                                     "score = ? WHERE id = ?;";

        // 编译SQL语句
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, updateSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        // 绑定参数
        sqlite3_bind_int(stmt, 1, scorevalue); // 绑定新的 valid 值
        sqlite3_bind_int(stmt, 2, idToUpdate); // 绑定要更新的记录的ID

        // 执行更新语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }

        std::cout << "classscore Data updated successfully!" << std::endl;

        // 清理并关闭
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else
    {
        std::string insertSQL = "INSERT INTO " + std::string(tableName) + " (id, score) VALUES (?, ?);";

        // 编译SQL语句
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        // 绑定参数
        sqlite3_bind_int(stmt, 1, idToInsert); // 绑定第一个参数（ID）
        sqlite3_bind_int(stmt, 2, scorevalue); // 绑定第二个参数（valid值）

        // 执行插入语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }

        std::cout << "classscore Data inserted successfully!" << std::endl;

        // 清理并关闭
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
}

// 更新class的valid信息。1表示这段数据是有效的，0表示这段数据是无效的。
void updateclassvalid(const char *tableName, int validvalue, sqlite3 *db)
{
    // 打开或创建数据库文件
    int rc = sqlite3_open("database.db", &db);

    if (rc)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        std::cout << "Error for insertclassscore!" << std::endl;
    }

    int idToInsert = 1; // 设置要插入的ID

    int idexist = checkidExists(tableName);

    if (idexist == 1)
    {
        int idToUpdate = 1; // 要更新的记录的Id
        std::string updateSQL = "UPDATE " + std::string(tableName) + " SET "
                                                                     "valid = ? WHERE id = ?;";

        // 编译SQL语句
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, updateSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        // 绑定参数
        sqlite3_bind_int(stmt, 1, validvalue); // 绑定新的 valid 值
        sqlite3_bind_int(stmt, 2, idToUpdate); // 绑定要更新的记录的ID

        // 执行更新语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }

        std::cout << "classvalid Data updated successfully!" << std::endl;

        // 清理并关闭
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else
    {
        std::string insertSQL = "INSERT INTO " + std::string(tableName) + " (id, valid) VALUES (?, ?);";

        // 编译SQL语句
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        // 绑定参数
        sqlite3_bind_int(stmt, 1, idToInsert); // 绑定第一个参数（ID）
        sqlite3_bind_int(stmt, 2, validvalue); // 绑定第二个参数（valid值）

        // 执行插入语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }

        std::cout << "classvalid Data inserted successfully!" << std::endl;

        // 清理并关闭
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
}

// 这个函数只适用于初始需要这个设置为1的情况
void updateclassrepete(const char *tableName, int repetevalue, sqlite3 *db)
{
    // 打开或创建数据库文件
    int rc = sqlite3_open("database.db", &db);

    if (rc)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        std::cout << "Error for insertclassrepete!" << std::endl;
    }

    int idToInsert = 1; // 设置要插入的ID

    int idexist = checkidExists(tableName);

    if (idexist == 1)
    {
        int idToUpdate = 1; // 要更新的记录的Id
        std::string updateSQL = "UPDATE " + std::string(tableName) + " SET "
                                                                     "repetition= ? WHERE id = ?;";

        // 编译SQL语句
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, updateSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        // 绑定参数
        sqlite3_bind_int(stmt, 1, repetevalue); // 绑定新的 valid 值
        sqlite3_bind_int(stmt, 2, idToUpdate);  // 绑定要更新的记录的ID

        // 执行更新语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }

        std::cout << "classrepete Data updated successfully!" << std::endl;

        // 清理并关闭
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else
    {
        std::string insertSQL = "INSERT INTO " + std::string(tableName) + " (id, repetition) VALUES (?, ?);";

        // 编译SQL语句
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return;
        }

        // 绑定参数
        sqlite3_bind_int(stmt, 1, idToInsert);  // 绑定第一个参数（ID）
        sqlite3_bind_int(stmt, 2, repetevalue); // 绑定第二个参数（valid值）

        // 执行插入语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }

        std::cout << "classrepete Data inserted successfully!" << std::endl;

        // 清理并关闭
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
}

// 这个函数适用于为repetition增加1的情况
void incrementrepetition(const char *tableName, sqlite3 *db)
{
    // 打开或创建数据库文件
    int rc = sqlite3_open("database.db", &db);

    if (rc)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        std::cout << "Error for updateRepetition!" << std::endl;
    }

    int idToUpdate = 1; // 要更新的记录的ID

    std::string updateSQL = "UPDATE " + std::string(tableName) + " SET "
                                                                 "repetition = repetition + 1 WHERE id = ?;"; // 将 repetition 增加 1

    // 编译SQL语句
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, updateSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    // 绑定参数
    sqlite3_bind_int(stmt, 1, idToUpdate); // 绑定要更新的记录的ID

    // 执行更新语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    std::cout << "Repetition updated successfully!" << std::endl;

    // 清理并关闭
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// 获取表格id中总数
int getTotalPointCount(const char *tableName, sqlite3 *db)
{

    int pointCount = -1;
    int rc = sqlite3_open("database.db", &db);
    if (rc)
    {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return pointCount;
    }

    std::string query = "SELECT COUNT(*) FROM " + std::string(tableName);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "查询准备失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return pointCount;
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        pointCount = sqlite3_column_int(stmt, 0); // 获取统计结果
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return pointCount;
}

// 读取tablename的特定id的longtitude
long double getLongitude(const char *tableName, int pointId, sqlite3 *db)
{
    long double longitude = 0.0;
    int rc = sqlite3_open("database.db", &db);

    std::string selectSQL = "SELECT longitude FROM " + std::string(tableName) + " "
                                                                                "WHERE point_id = " +
                            std::to_string(pointId) + ";";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "查询准备失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return longitude;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        longitude = sqlite3_column_double(stmt, 0);
    }
    else
    {
        std::cerr << "未找到指定的数据行." << std::endl;
    }

    sqlite3_finalize(stmt);
    return longitude;
}

// 读取tablename的特定id的latitude
long double getLatitude(const char *tableName, int pointId, sqlite3 *db)
{
    long double Latitude = 0.0;
    int rc = sqlite3_open("database.db", &db);

    std::string selectSQL = "SELECT Latitude FROM " + std::string(tableName) + " "
                                                                               "WHERE point_id = " +
                            std::to_string(pointId) + ";";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "查询准备失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return Latitude;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        Latitude = sqlite3_column_double(stmt, 0);
    }
    else
    {
        std::cerr << "未找到指定的数据行." << std::endl;
    }

    sqlite3_finalize(stmt);
    return Latitude;
}

// 获取特定路径
Path getpath(const char *tableName, sqlite3 *db)
{
    int rc = sqlite3_open("database.db", &db);
    if (rc)
    {
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db) << std::endl;
    }

    int totalid = getTotalPointCount(tableName, db);

    Path path1;

    for (int i = 1; i <= totalid; i++)
    {
        long double lon1 = getLongitude(tableName, i, db);
        long double lat1 = getLatitude(tableName, i, db);
        path1.push_back({lon1, lat1});
    }

    return path1;
    sqlite3_close(db);
}

// 获取两条路段之间的DTW距离数值
double calculateDWB(const Path &data1, const Path &data2)
{
    // 创建一个矩阵用于存储DTW距离
    std::vector<std::vector<double>> dtwMatrix(data1.size(), std::vector<double>(data2.size(), 0.0));

    // 计算DTW矩阵
    for (size_t i = 0; i < data1.size(); ++i)
    {
        for (size_t j = 0; j < data2.size(); ++j)
        {
            double cost = distanceBetweenPoints(data1[i], data2[j]);

            // 计算DTW距离，可以根据需要选择使用其他距离度量
            if (i == 0 && j == 0)
            {
                dtwMatrix[i][j] = cost;
            }
            else if (i == 0)
            {
                dtwMatrix[i][j] = cost + dtwMatrix[i][j - 1];
            }
            else if (j == 0)
            {
                dtwMatrix[i][j] = cost + dtwMatrix[i - 1][j];
            }
            else
            {
                dtwMatrix[i][j] = cost + std::min({dtwMatrix[i - 1][j], dtwMatrix[i][j - 1], dtwMatrix[i - 1][j - 1]});
            }
        }
    }

    // 返回DTW距离
    return dtwMatrix[data1.size() - 1][data2.size() - 1];
}
// 路径比较算法，这里的threshold需要调试，而且那个路径对齐也需要调试
void DTWcomparison(int totestnum, sqlite3 *db, double threshold)
{
    std::string DTWcombinedString;
    const char *DTWclasspointposition = nullptr;
    const char *DTWcreatename = nullptr;
    ;
    std::string DTWpoints = "points";
    std::string DTWposition;
    Path path1, path2;

    int rc = sqlite3_open("database.db", &db);
    if (rc)
    {
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db) << std::endl;
    }

    DTWcombinedString = combineStrings(table, totestnum - 1);
    DTWcreatename = DTWcombinedString.c_str(); // 这个是上一个刚刚完成的class的名字
    DTWposition = DTWcreatename + DTWpoints;
    DTWclasspointposition = DTWposition.c_str(); // 这个是上一个刚刚完成的classpoints的名字
    path1 = getpath(DTWclasspointposition, db);

    for (int i = 1; i <= totestnum - 2; i++)
    {
        std::string newcombinedString;
        const char *newclasspointposition = nullptr;
        const char *newcreatename = nullptr;
        ;
        std::string newpoints = "points";
        std::string newposition;

        newcombinedString = combineStrings(table, i);
        newcreatename = newcombinedString.c_str(); // 这个是i的class的名字
        newposition = newcreatename + newpoints;
        newclasspointposition = newposition.c_str(); // 这个是i的classpoints的名字

        path2 = getpath(newclasspointposition, db);
        double c = calculateDWB(path1, path2); // 判断这两条路径的距离
        if (c < threshold)
        {
            updateclassvalid(DTWcreatename, 0, db);
            incrementrepetition(newcreatename, db);
            break; // 直接跳出循环不检测了；
        }
    }
}

// 单点负责读入信息
void loadpoints(const char *tableName, int pointId, int newClassAId, double newLongitude, double newLatitude, sqlite3 *db)
{

    int rc = sqlite3_open("database.db", &db);

    std::string insertSQL = "INSERT INTO " + std::string(tableName) + "(point_id, class_a_id, longitude, latitude) "
                                                                      "VALUES (" +
                            std::to_string(pointId) + ", " + std::to_string(newClassAId) + ", " + std::to_string(newLongitude) + ", " + std::to_string(newLatitude) + ");";

    rc = sqlite3_exec(db, insertSQL.c_str(), nullptr, 0, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL 错误: " << sqlite3_errmsg(db) << std::endl;
    }
    else
    {
        std::cout << "新点数据插入成功!" << std::endl;
    }
}

// std::string updateSQL = "UPDATE " + std::string(tableName) + " SET "
//     "class_a_id = " + std::to_string(newClassAId) + ", "
//    "longitude = " + std::to_string(newLongitude) + ", "
//    "latitude = " + std::to_string(newLatitude) + " "
//     "WHERE point_id = " + std::to_string(pointId) + ";";

// 执行UPDATE语句
//  rc = sqlite3_exec(db, updateSQL.c_str(), nullptr, 0, nullptr);
//
// if (rc != SQLITE_OK) {
//      std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
//     std::cout << "Point data updated unsuccessfully!" << std::endl;
// }

//  std::cout << "Point data updated successfully!" << std::endl;
//  sqlite3_close(db); .//这个是更新数据，不是插入数据，需要这个区别

// 读取文本行的数据
string ReadLine(string filename, int line)
{
    int lines, i = 0;
    string temp;
    fstream file;
    file.open(filename.c_str());
    lines = CountLines(filename);

    if (line <= 0)
    {
        return "Error 1: 行数错误，不能为0或负数。";
    }
    if (file.fail())
    {
        return "Error 2: 文件不存在。";
    }
    if (line > lines)
    {
        return "Error 3: 行数超出文件长度。";
    }
    while (getline(file, temp) && i < line - 1)
    {
        i++;
    }
    file.close();
    return temp;
}

// 存储一个文件的信息
bool loadfile(sqlite3 *db)
{
    int line = CountLines(filename);
    int clusteringtemp[10] = {0};
    int z = 1;
    int m = 0;

    const char *classpointposition = nullptr;
    const char *createname = nullptr;
    ;
    std::string points = "points";
    std::string position;

    for (int i = 0; i <= line - 1; i++)
    {
        string temp;
        temp = ReadLine(filename, i + 1);
        string temp1, temp2, temp3;
        temp1 = temp.substr(0, 13);
        temp2 = temp.substr(14, 12);
        temp3 = temp.substr(27);

        long double a;
        a = std::stold(temp1);

        long double b;
        b = std::stold(temp2);

        int c;
        c = std::stoi(temp3);
        // a是经度 b是维度 c是类别数目

        // cout << c;

        int j = getorder(db);

        int bi = 99999999;
        for (int k = 0; k < 10; k++)
        {
            if (c == clusteringtemp[k])
                bi = 1;
        }

        if (bi != 1)
        {

            clusteringtemp[m] = c;
            m++;

            // 防止在一个文件里面出错，所以需要变动一下；
            // 这也就意味着上一个类的数据全部组装完毕，此时检测一遍重复性
            DTWcomparison(j, db, threshold);
            // 更新名字和创立
            combinedString = combineStrings(table, j);
            // std::cout << "Combined String: " << combinedString << std::endl;
            IncreaseIntegers(db); // 增加j的数值，这个j已经是下一次的j了，也就是下面执行的loadpoints其实是用的"j"的数值。而上一个已经完成的表格是j-1

            cout << "我执行了" << j << "次";
            z = 1;
            createname = combinedString.c_str(); // 把string字符串转换成相关的char字符串，因为后面需要。
            createNewTable(createname, db);

            position = createname + points;
            classpointposition = position.c_str();
            updateclassvalid(createname, 1, db);
            updateclassscore(createname, 100, db); // 更新得分
            updateclassrepete(createname, 1, db);  // 更新repetition
        }

        loadpoints(classpointposition, z, j - 1, a, b, db);
        z++;
    }
    return true;
}

// 调试文件，存储信息
bool loadfile1(sqlite3 *db)
{
    int line = CountLines(filename);
    int clusteringtemp[10] = {0};
    int z = 1;
    int m = 0;
    int j = getorder(db);
    const char *classpointposition = nullptr;
    const char *createname = nullptr;
    ;
    std::string points = "points";
    std::string position;

    for (int i = 0; i <= line - 1; i++)
    {
        string temp;
        temp = ReadLine(filename, i + 1);
        string temp1, temp2, temp3;
        temp1 = temp.substr(0, 13);
        temp2 = temp.substr(14, 12);
        temp3 = temp.substr(27);

        long double a;
        a = std::stold(temp1);

        long double b;
        b = std::stold(temp2);

        int c;
        c = std::stoi(temp3);
        // a是经度 b是维度 c是类别数目

        // 防止在一个文件里面出错，所以需要变动一下；

        // 更新名字和创立
        combinedString = combineStrings(table, j);
        // std::cout << "Combined String: " << combinedString << std::endl;

        z = 1;
        createname = combinedString.c_str(); // 把string字符串转换成相关的char字符串，因为后面需要。
        createNewTable(createname, db);
        position = createname + points;
        classpointposition = position.c_str();

        updateclassscore(createname, 100, db); // 更新得分

        loadpoints(classpointposition, z, j - 1, a, b, db);
        z++;
    }
    return true;
} // 调试文件

// 调试用代码，可删除
bool isTableEmpty(const char *tableName)
{
    bool isEmpty = true;

    int rc = sqlite3_open("database.db", &db);
    if (rc)
    {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return isEmpty;
    }

    std::string query = "SELECT 1 FROM " + std::string(tableName) + " LIMIT 1";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "查询准备失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return isEmpty;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        isEmpty = false; // 表格不为空
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return isEmpty;
}

int main()
{

    char *errMsg = 0; // 用于储存错误信息

    // 创建数据库（如果数据库已经存在，则会打开现有数据库）
    int rc = sqlite3_open("database.db", &db);
    if (rc)
    {
        std::cerr << "无法打开/创建数据库: " << sqlite3_errmsg(db) << std::endl;
        return 0;
    }
    sqlite3_close(db);

    // 更新字符串
    // std::string combinedString = combineStrings(table, j);
    // std::cout << "Combined String: " << combinedString << std::endl
    //  const char* createname = combinedString.c_str();//把string字符串转换成相关的char字符串，因为后面需要。
    // createNewTable(createname);

    // bool a = loadfile(db);
    bool b = loadfile(db);
    // IncreaseIntegers(db);

    // int j = getorder();
    // cout << j;
    // combinedString = combineStrings(table, j);
    // const char*  createname = combinedString.c_str();
    // createNewTable(createname);
    // std::string points = "points";
    // std::string  position = createname + points;
    // const char*  classpointposition = position.c_str();
    // cout << classpointposition;
    // 关闭数据库

    // bool tableIsEmpty = isTableEmpty("table130points");
    //

    // cout << tableIsEmpty<<endl;

    // double a = getLatitude("table130points", 1, db);
    // cout << a;

    // updateclassscore("table166",100, db);
    std::cout << "数据库和表格创建成功。" << std::endl;
    //
    //  Path path1, path2;

    //  path1 = getpath("table130points", db);
    // path2 = getpath("table130points", db);
    // double c = calculateDWB(path1, path2);
    // cout << c;
    // std::cout << "对齐后的路径：" << std::endl;
    // for (const auto& point : path1) {
    //   std::cout << "(" << point.first << ", " << point.second << ")" << std::endl;
    //}

    // updateclassrepete("table130", 1, db);
    // incrementrepetition("table130", db);

    return 0;
}
