#ifndef _DBSCAN_H_
#define _DBSCAN_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <cmath>
#include <stack>
#include <iomanip>
#include <algorithm>
#include <unordered_set>
#include <eigen3/Eigen/Dense>

using namespace std;

// 众包统计：1.共轭核心点数 2. 路线包含点数——衡量车程 3. 路线次数——选择程度
extern int init_times; // 初始化次数
extern double sigma;   // initial sigma = 1 for GKF
extern double lambda;  // 归一化参数lambda initial = 2
extern double delta;   // 半径delta
extern int minpts;     // 最小点数
extern int strayInit;  // Initial limit to stray dots

class point
{
public:
    double x = 0;       // longitude
    double y = 0;       // latitude
    double heading = 0; // cos(angle)

    int cluster = 0;

    std::unordered_set<int> fork;

    int pointType = 1; // 1 noise 2 border 3 core
    int pts = 0;       // points in MinPts
    vector<int> corepts;
    int visited = 0;

    bool operator==(const point &p)
    {
        if (p.cluster == cluster)
            return true;
        return false;
    }
    point() {}
    point(double a0, double b0, double a1, double b1, int c)
    {
        x = a1;
        y = b1;
        heading = (a1 - a0) / sqrt((a1 - a0) * (a1 - a0) + (b1 - b0) * (b1 - b0)) / lambda;
        cluster = c;
    }
    point(point p, int c)
    {
        x = p.x;
        y = p.y;
        heading = p.heading;
        cluster = c;
    }
    point(double a, double b)
    {
        x = a;
        y = b;
    }
    point(double a, double b, double c)
    {
        x = a;
        y = b;
        heading = c;
    }
};

typedef vector<point> TrajectoryType;

TrajectoryType smoothTrajectoryWithKalman(const TrajectoryType &trajectory, double processNoise = 0.01, double measurementNoise = 0.1);

double cosAngle(point a, point b);

TrajectoryType ReadTrajectory(const string &path);

double squareDistance(point a, point b);

double GKDistance(point a, point b);

TrajectoryType DBSCAN(TrajectoryType dataset, double Eps, int MinPts);

vector<int> getClusterNum(TrajectoryType corePoint);

void sortClusterNum(TrajectoryType &data, vector<int> &num);

TrajectoryType reClustering(TrajectoryType oT, TrajectoryType nT, double Eps, int MinPts);

template <typename A, typename B>
void delElemFromVec(A &vec, B &criteria);

void StraydotRemove(TrajectoryType &data, vector<int> &num, int init = 8);

TrajectoryType findforkPoints(TrajectoryType &trajectory, double epsilon = 0.00001, int changeThreshold = 2);

void OutPut(const TrajectoryType corePoint, string filename);

void OutPutOriginData(const TrajectoryType data);

#endif // _DBSCAN_H_
