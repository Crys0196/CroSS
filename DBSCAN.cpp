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
string data_file = "./dataset/TRoute1.txt"; // 数据集地址
string new_file = "./dataset/TRoute2.txt";

string result_file = "./resultset/T12.txt";

int init_times = 3;	 // 初始化次数
double sigma = 1;	 // initial sigma = 1 for GKF
double lambda = 3;	 // 归一化参数lambda initial = 2
double delta = 0.05; // 半径delta
int minpts = 6;		 // 最小点数
int strayInit = 5;	 // Initial limit to stray dots
// TODO   2.岔点 问题：岔点与重聚类有矛盾即如何在新路线重聚类时，形成新岔点 3. Unify the doc type.
// 众包统计：1.共轭核心点数 2. 路线包含点数——衡量车程 3. 路线次数——选择程度
class point
{
public:
	double x = 0;		// longitude
	double y = 0;		// latitude
	double heading = 0; // cos(angle)

	int cluster = 0;

	std::unordered_set<int> fork;

	int pointType = 1; // 1 noise 2 border 3 core
	int pts = 0;	   // points in MinPts
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

// 读取轨迹数据集
typedef vector<point> TrajectoryType;

TrajectoryType smoothTrajectoryWithKalman(const TrajectoryType &trajectory, double processNoise = 0.01, double measurementNoise = 0.1)
{
	TrajectoryType smoothedTrajectory;

	if (trajectory.empty())
	{
		return smoothedTrajectory;
	}

	// 定义状态和观测向量的维度
	const int stateDim = 4;		  // 状态向量维度（经度、纬度、经度速度、纬度速度）
	const int observationDim = 2; // 观测向量维度（经度、纬度）

	// 创建滤波器对象
	Eigen::VectorXd initialState(stateDim);						// 初始状态向量
	initialState << trajectory[0].x, trajectory[0].y, 0.0, 0.0; // 初始速度设为0

	Eigen::MatrixXd initialCovariance = Eigen::MatrixXd::Identity(stateDim, stateDim); // 初始状态协方差矩阵

	Eigen::MatrixXd stateTransitionMatrix(stateDim, stateDim); // 状态转移矩阵
	stateTransitionMatrix << 1, 0, 1, 0,
		0, 1, 0, 1,
		0, 0, 1, 0,
		0, 0, 0, 1;

	Eigen::MatrixXd observationMatrix(observationDim, stateDim); // 观测矩阵
	observationMatrix << 1, 0, 0, 0,
		0, 1, 0, 0;

	Eigen::MatrixXd processNoiseCovariance = Eigen::MatrixXd::Identity(stateDim, stateDim) * processNoise; // 过程噪声协方差矩阵

	Eigen::MatrixXd measurementNoiseCovariance = Eigen::MatrixXd::Identity(observationDim, observationDim) * measurementNoise; // 观测噪声协方差矩阵

	// 初始化卡尔曼滤波器
	Eigen::VectorXd currentState = initialState;
	Eigen::MatrixXd currentCovariance = initialCovariance;

	smoothedTrajectory.push_back({currentState(0), currentState(1)}); // 将初始状态加入平滑后的轨迹

	// 进行卡尔曼滤波处理
	for (size_t i = 1; i < trajectory.size(); ++i)
	{
		// 预测步骤
		currentState = stateTransitionMatrix * currentState;
		currentCovariance = stateTransitionMatrix * currentCovariance * stateTransitionMatrix.transpose() + processNoiseCovariance;

		// 更新步骤
		Eigen::VectorXd observation(observationDim);
		observation << trajectory[i].x, trajectory[i].y;

		Eigen::VectorXd observationResidual = observation - observationMatrix * currentState;
		Eigen::MatrixXd kalmanGain = currentCovariance * observationMatrix.transpose() * (observationMatrix * currentCovariance * observationMatrix.transpose() + measurementNoiseCovariance).inverse();
		currentState = currentState + kalmanGain * observationResidual;
		currentCovariance = (Eigen::MatrixXd::Identity(stateDim, stateDim) - kalmanGain * observationMatrix) * currentCovariance;

		smoothedTrajectory.push_back({currentState(0), currentState(1)}); // 将更新后的状态加入平滑后的轨迹
	}

	return smoothedTrajectory;
}

double cosAngle(point a, point b)
{
	double a0 = a.x;
	double a1 = b.x;
	double b0 = a.y;
	double b1 = b.y;

	double t;
	t = (a1 - a0) / sqrt((a1 - a0) * (a1 - a0) + (b1 - b0) * (b1 - b0)) / lambda;
	return t;
}
TrajectoryType ReadTrajectory(const string &path)
{
	fstream file;
	TrajectoryType trajectory;
	file.open(path, ios::in);
	if (!file)
	{
		cout << "Data file " << path << " not find." << endl;
		return trajectory;
	}
	int i = 1;
	double lat0;
	double lon0;
	double lat1;
	double lon1;

	string buf;
	// initial
	getline(file, buf);
	if (buf[0] == '%')
		getline(file, buf);
	for (int i = 0; i < init_times; i++)
		getline(file, buf);

	string tmp;
	istringstream iss(buf);
	/*
	for (int i = 0; i < 2; i++)
	{
		iss >> tmp;
	}
	iss >> lat0 >> lon0;

	while (!file.eof())
	{
		getline(file, buf);
		istringstream iss1(buf);
		for (int i = 0; i < 2; i++)
		{
			iss1 >> tmp;
		}
		iss1 >> lat1 >> lon1;
		point p(lon0, lat0, lon1, lat1, i++);
		trajectory.push_back(p);
		lat0 = lat1;
		lon0 = lon1;
	}
*/

	while (!file.eof())
	{
		getline(file, buf);
		istringstream iss(buf);
		for (int i = 0; i < 2; i++)
		{
			iss >> tmp;
		}
		iss >> lat0 >> lon0;
		point p(lon0, lat0);
		trajectory.push_back(p);
	}

	file.close();

	TrajectoryType sTrajectory = smoothTrajectoryWithKalman(trajectory);
	for (int i = 0; i < sTrajectory.size() - 1; i++)
	{
		sTrajectory[i].heading = cosAngle(sTrajectory[i], sTrajectory[i + 1]);
		sTrajectory[i].cluster = i;
	}
	sTrajectory.pop_back();

	cout << "Read trajectory successfully!" << endl;
	cout << "Trajectory has " << sTrajectory.size() << " points." << endl;
	return sTrajectory;

	/*
	cout << "Read trajectory successfully!" << endl;
	cout << "Trajectory has " << trajectory.size() << " points." << endl;
	return trajectory;
	*/
}

double squareDistance(point a, point b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

double GKDistance(point a, point b)
{
	double temp = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.heading - b.heading) * (a.heading - b.heading);
	double k = exp(-(temp) / (sigma * sigma));
	double GKD = sqrt(2 - 2 * k);
	return GKD;
}

// 聚类算法
TrajectoryType DBSCAN(TrajectoryType dataset, double Eps, int MinPts)
{
	int len = dataset.size();
	// calculate pts
	cout << "calculate pts" << endl;
	for (int i = 0; i < len; i++)
	{
		for (int j = i + 1; j < len; j++)
		{
			if (GKDistance(dataset[i], dataset[j]) < Eps)
				dataset[i].pts++;
			dataset[j].pts++;
		}
	}
	// core point
	cout << "core point " << endl;
	TrajectoryType corePoint;
	for (int i = 0; i < len; i++)
	{
		if (dataset[i].pts >= MinPts)
		{
			dataset[i].pointType = 3;
			corePoint.push_back(dataset[i]);
		}
	}
	cout << "joint core point" << endl;
	// joint core point

	for (int i = 0; i < corePoint.size(); i++)
	{
		for (int j = i + 1; j < corePoint.size(); j++)
		{
			if (GKDistance(corePoint[i], corePoint[j]) < Eps)
			{
				corePoint[i].corepts.push_back(j);
				corePoint[j].corepts.push_back(i);
			}
		}
	}

	for (int i = 0; i < corePoint.size(); i++)
	{
		stack<point *> ps;
		if (corePoint[i].visited == 1)
			continue;
		ps.push(&corePoint[i]);
		point *v;
		while (!ps.empty())
		{
			v = ps.top();
			v->visited = 1;
			ps.pop();
			for (int j = 0; j < v->corepts.size(); j++)
			{
				if (corePoint[v->corepts[j]].visited == 1)
					continue;
				corePoint[v->corepts[j]].cluster = corePoint[i].cluster;
				corePoint[v->corepts[j]].visited = 1;
				ps.push(&corePoint[v->corepts[j]]);
			}
		}
	}
	cout << "border point,joint border point to core point" << endl;
	// border point,joint border point to core point
	int num_border = 0;
	for (int i = 0; i < len; i++)
	{
		if (dataset[i].pointType == 3)
			continue;
		for (int j = 0; j < corePoint.size(); j++)
		{
			if (GKDistance(dataset[i], corePoint[j]) < Eps)
			{
				dataset[i].pointType = 2;
				dataset[i].cluster = corePoint[j].cluster;
				num_border++;
				break;
			}
		}
	}
	cout << "Border points number = " << num_border << endl;
	cout << "Core points number = " << corePoint.size() << endl;
	return corePoint;
}

vector<int> getClusterNum(TrajectoryType corePoint)
{
	// Put cluster number into all_cluster
	vector<int> all_cluster;
	for (int i = 0; i < corePoint.size(); i++)
	{
		int cluster = corePoint[i].cluster;
		if (all_cluster.empty())
		{
			all_cluster.push_back(cluster);
			continue;
		}
		int ncount = count(all_cluster.begin(), all_cluster.end(), cluster);
		if (ncount == 0)
			all_cluster.push_back(cluster);
	}
	return all_cluster;
}

void sortClusterNum(TrajectoryType &data, vector<int> &num)
{
	int dlen = data.size();
	int clen = num.size();
	for (int i = 0; i < dlen; i++)
	{
		for (int j = 0; j < clen; j++)
		{
			if (data[i].cluster == num[j])
			{
				data[i].cluster = j + 1;
			}
		}
	}
	for (int i = 0; i < clen; i++)
	{
		num[i] = i + 1;
	}
}

TrajectoryType reClustering(TrajectoryType oT, TrajectoryType nT, double Eps, int MinPts)
{
	TrajectoryType rT; // resultTrajectory
	// similar part
	int c = getClusterNum(oT).back();
	for (int i = 0; i < nT.size(); i++)
	{
		c++;
		nT[i].cluster = c;
		for (int j = 0; j < oT.size(); j++)
		{
			if (GKDistance(nT[i], oT[j]) < Eps)
			{
				nT[i].pts++;
				oT[j].pts++;
				// Here maybe exist bugs
			}
		}
		if (nT[i].pts >= MinPts)
		{
			// tnum++;

			nT.erase(nT.begin() + i);
			i--;
		}
	}
	// different part
	TrajectoryType temp = DBSCAN(nT, Eps, MinPts);
	rT = oT;
	rT.insert(rT.end(), temp.begin(), temp.end());
	return rT;
}

template <typename A, typename B>
void delElemFromVec(A &vec, B &criteria)
{
	for (int i = 0; i < vec.size(); i++)
	{
		if (vec[i] == criteria)
		{
			vec.erase(vec.begin() + i);
			i--;
		}
	}
}
void StraydotRemove(TrajectoryType &data, vector<int> &num, int init = 8)
{
	TrajectoryType symbol;

	// Get symbol points
	for (int i = 0; i < num.size(); i++)
	{
		point p;
		p.cluster = num[i];
		symbol.push_back(p);
	}
	for (int i = 0; i < symbol.size(); i++)
	{
		int times = 0;
		for (int j = 0; j < data.size(); j++)
		{
			if (data[j] == symbol[i])
			{
				times++;
			}
		}
		if (times > init)
		{
			symbol.erase(symbol.begin() + i);
			i--;
		}
	}
	// stray dots
	for (int i = 0; i < symbol.size(); i++)
	{
		/*for (int k = 0; k < num.size(); k++)
		{
			if (num[k] == symbol[i].cluster)
			{
				swap(num[k], num[num.size() - 1]);
				num.pop_back();
				k--;
			}
		}

		for (int j = 0; j < data.size(); j++)
		{
			if (data[j] == symbol[i])
			{
				swap(data[j], data[data.size() - 1]);
				data.pop_back();
				j--;
			}
		}*/

		delElemFromVec(num, symbol[i].cluster);
		delElemFromVec(data, symbol[i]);
	}
	sortClusterNum(data, num);
}

// 找出拐点的函数
TrajectoryType findforkPoints(TrajectoryType &trajectory, double epsilon = 0.00001, int changeThreshold = 2)
{
	TrajectoryType forkPoints;

	if (trajectory.size() < changeThreshold)
	{
		// 轨迹点数量过小，无法判断拐点
		return forkPoints;
	}

	for (int i = 1; i < trajectory.size(); ++i)
	{
		point &p = trajectory[i];
		unordered_set<int> types;

		bool alreadyhave = 0;
		for (int k = 0; k < forkPoints.size(); k++)
		{
			double distance = squareDistance(p, forkPoints[k]);
			if (distance <= epsilon)
			{
				alreadyhave = 1;
				break;
			}
		}
		if (alreadyhave)
			continue;

		for (int j = 0; j < trajectory.size(); ++j)
		{
			const point &q = trajectory[j];
			double distance = squareDistance(p, q);
			if (distance <= epsilon)
			{
				types.insert(q.cluster);
			}
		}
		// cout << types.size() << endl;
		if (types.size() >= changeThreshold)
		{
			p.fork = types;
			forkPoints.push_back(p);
		}
	}

	return forkPoints;
}

void OutPut(const TrajectoryType corePoint, string filename)
{
	cout << "output" << endl;
	// output
	fstream clustering;
	clustering.open(filename, ios::out);
	/*for(int i=0;i<len;i++){
		if(dataset[i].pointType == 2)
			clustering<< fixed << setprecision(9) << dataset[i].x<<","<<dataset[i].y<<","<<dataset[i].cluster<<"\n";
	}*/
	// 此为边缘点
	for (int i = 0; i < corePoint.size(); i++)
	{
		clustering << fixed << setprecision(9) << corePoint[i].x << "," << corePoint[i].y << "," << corePoint[i].cluster << "\n";
	} // 输出核心点
	clustering.close();
}

void OutPutOriginData(const TrajectoryType data)
{
	cout << "output" << endl;
	// output
	fstream clustering;
	clustering.open("originData.txt", ios::out);
	/*for(int i=0;i<len;i++){
		if(dataset[i].pointType == 2)
			clustering<< fixed << setprecision(9) << dataset[i].x<<","<<dataset[i].y<<","<<dataset[i].cluster<<"\n";
	}*/
	// 此为边缘点
	for (int i = 0; i < data.size(); i++)
	{
		clustering << fixed << setprecision(9) << data[i].x << "," << data[i].y << "," << data[i].heading << "," << data[i].cluster << "\n";
	} // 输出核心点
	clustering.close();
}
int main(int argc, char **argv)
{
	TrajectoryType dataset0 = ReadTrajectory(data_file);
	TrajectoryType dataset1 = ReadTrajectory(new_file);

	// OutPutOriginData(dataset0);

	TrajectoryType trajectory1 = DBSCAN(dataset0, delta, minpts);
	// TrajectoryType trajectory2 = DBSCAN(dataset1, delta, minpts);
	vector<int> Cnum = getClusterNum(trajectory1); // Get the NumVector
	StraydotRemove(trajectory1, Cnum);
	sortClusterNum(trajectory1, Cnum);

	TrajectoryType reclusteringT = reClustering(trajectory1, dataset1, delta, minpts);
	vector<int> Cnum1 = getClusterNum(reclusteringT);
	StraydotRemove(reclusteringT, Cnum1);
	sortClusterNum(reclusteringT, Cnum1);

	TrajectoryType forkpoint = findforkPoints(trajectory1);
	// OutPutOriginData(dataset0);
	OutPut(forkpoint, "fork1.txt");
	OutPut(reclusteringT, result_file);
	//   OutPut(trajectory2, "trajectory2");
	//    OutPut(reclusteringT);
	return 0;
}