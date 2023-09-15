#include "DBSCAN.h"

string data_file = "./dataset/TRoute1.txt"; // 数据集地址
string new_file = "./dataset/TRoute2.txt";
string result_file = "./resultset/Cluster.txt";

int main(int argc, char **argv)
{
    TrajectoryType dataset0 = ReadTrajectory(data_file);
    // TrajectoryType dataset1 = ReadTrajectory(new_file);

    // OutPutOriginData(dataset0);

    TrajectoryType trajectory1 = DBSCAN(dataset0, delta, minpts);
    // TrajectoryType trajectory2 = DBSCAN(dataset1, delta, minpts);
    vector<int> Cnum = getClusterNum(trajectory1); // Get the NumVector
    StraydotRemove(trajectory1, Cnum, 8);
    sortClusterNum(trajectory1, Cnum);

    // TrajectoryType reclusteringT = reClustering(trajectory1, dataset1, delta, minpts);
    // vector<int> Cnum1 = getClusterNum(reclusteringT);
    // StraydotRemove(reclusteringT, Cnum1);
    // sortClusterNum(reclusteringT, Cnum1);

    // TrajectoryType forkpoint = findforkPoints(trajectory1);
    //  OutPutOriginData(dataset0);
    // OutPut(forkpoint, "fork1.txt");
    OutPut(trajectory1, result_file);
    cout << "Successfully cluster! You can check route file in " << result_file << endl;
    // OutPut(trajectory2, "trajectory2");
    // OutPut(reclusteringT);
    return 0;
}