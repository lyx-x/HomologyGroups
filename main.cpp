#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <cassert>

#ifndef NORMAL
#include <glog/logging.h>
#else
#define LOG(INFO) cout
#endif

using namespace std;

typedef set<int> vertices;
typedef vector<vector<int>> matrix;

struct interval {
	float start, end;
	int dim;

	bool operator<(const interval &rhs) const {
		if (start < rhs.start)
			return true;
		else if (start == rhs.start) {
			if (dim < rhs.dim)
				return true;
			else if (dim == rhs.dim)
				return end < rhs.end;
		}
		return false;
	}
};

ostream& operator<< (ostream &o, const interval &i) {
	o << i.dim << ' ' << i.start << ' ';
	if (i.end == numeric_limits<float>::max())
		o << "inf";
	else
		o << i.end;
	o << endl;
	return o;
}

struct simplex {
	int dim; // dimension of the simplex
	float val; // time
	vertices vert; // the sorted set of the vertex IDs of the simplex

	simplex(): dim(-1), val(0) {}
	simplex(int dim, float val): dim(dim), val(val) {}

	bool operator<(const simplex &rhs) const {
		if (val < rhs.val)
			return true;
		else if (val == rhs.val) {
			if (dim < rhs.dim)
				return true;
			else if (dim == rhs.dim)
				return vert < rhs.vert;
		}
		return false;
	}
};

ostream& operator<< (ostream &o, const simplex &s) {
	o << "{val=" << s.val << "; dim=" << s.dim << "; [";
	for (auto it = s.vert.begin(); it != s.vert.end(); ){
		o << *it++;
		if (it != s.vert.end())
			o << ", ";
	}
	o << "]}" << endl;
	return o;
}

ostream& operator<< (ostream &o, const matrix &m) {
	for (int i = 0; i < m.size(); i++) {
		for(int j = 0; j < m.size(); j++)
			o << (find(m[j].begin(), m[j].end(), i) != m[j].end()) << ' ';
		o << endl;
	}
	return o;
}

/*
 * Build boundary matrix from a list of simplexes
 */
matrix make_matrix(const vector<simplex>& F) {

	// initiate a sparse matrix with F.size() columns and rows
	matrix m(F.size());

	// associate each simplex with its order in the F set
	map<set<int>, int> indices;

	int non_zero = 0;

	// build the matrix
	vertices tmp;
	int col = 0;
	for (auto s: F) {
		tmp = s.vert;
		indices[s.vert] = col;
		// remove one vertex to form a face, loop over all vertices to find all faces
		for (auto v: s.vert) {
			tmp.erase(tmp.find(v));
			int index;
			if (indices.count(tmp) > 0) {
				index = indices[tmp]; // get the row number of each face
				m[col].push_back(index);
				non_zero++;
			}
			tmp.insert(v);
		}
		sort(m[col].begin(), m[col].end()); // put rows in order
		col++; // go to next column
	}

	LOG(INFO) << "Non zeros: " << non_zero << endl;

	return m;
}

/*
 * Column operation on binary matrix: self = self + other
 *
 * The result is the disjoint part of two sets, the complexity is linear to the size of set
 *
 * Params:
 * 		self: column to be updated (sorted)
 * 		other: other column (remains the same) (sorted)
 */
void add(vector<int> &self, const vector<int> &other) {
	vector<int> tmp;
	auto a = self.begin();
	auto b = other.begin();
	while (a != self.end() && b != other.end()) {
		if (*a == *b) { // same number
			a++;
			b++;
		} else if (*a < *b) { // a is not in other set
			tmp.push_back(*a);
			a++;
		} else {
			tmp.push_back(*b); // b is not in self set
			b++;
		}
	}
	self = tmp; // overwrite the column
}

/*
 * Reduce the matrix so the low function is injection
 */
void reduction(matrix &m) {
	int time = 0;
	map<int, int> inverse_low;
	for (int i = 0; i < m.size(); i++) {
		if (m[i].size() == 0)
			continue;
		int low;
		do {
			low = *(m[i].rbegin());
			if (inverse_low.count(low) <= 0)
				break;
			add(m[i], m[inverse_low[low]]);
			time++;
		} while (m[i].size() > 0);
		inverse_low[low] = i;
	}
	LOG(INFO) << "Average reduction times: " << time / float(m.size()) << endl;
}

/*
 * Find intervals
 */
multiset<interval> get_intervals(const matrix &reduced_m, const vector<simplex> &F) {

	// prepare for lots of intervals
	float *start = new float[F.size()];
	float *end = new float[F.size()];
	int *dim = new int[F.size()];
	fill(end, end + F.size(), numeric_limits<float>::max());
	fill(dim, dim + F.size(), -1);

	// loop over all columns to find interval bound
	assert(F.size() == reduced_m.size());
	for (int c = 0; c < F.size(); c++) {
		if (reduced_m[c].size() == 0) { // start an interval
			start[c] = F[c].val;
			dim[c] = F[c].dim;
		}
		else {
			end[*reduced_m[c].rbegin()] = F[c].val;
		}
	}

	multiset<interval> res;
	for (int c = 0; c < F.size(); c++) {
		if (dim[c] >= 0) {
			interval i;
			i.start = start[c];
			i.end = end[c];
			i.dim = dim[c];
			res.insert(i);
		}
	}

	delete[] start;
	delete[] end;
	delete[] dim;
	return res;
}

/*
 * Help function to read the filtration file within a certain format
 */
vector<simplex> read_filtration(const string name){

	vector<simplex> F;

	ifstream input(name);
	ios::sync_with_stdio(false);

	map<int, int> vertices_count;
	if (input) {
		float val;
		int dim, tmp;
		while (input.good()) {
			input >> val >> dim;
			if (input.eof()) // avoid ghost line
				break;
			simplex s(dim, val);
			for (int i = 0; i <= dim; i++) {
				input >> tmp;
				s.vert.insert(tmp);
			}
			if (vertices_count.count(dim) == 0)
				vertices_count[dim] = 1;
			else
				vertices_count[dim]++;
			F.push_back(s);
		}
		input.close();
	}
	else
		cout << "Failed to read file " << name << endl;

	for (auto count: vertices_count)
		LOG(INFO) << "Vertices of dim " << count.first << ": " << count.second << endl;
	LOG(INFO) << "Simplexes: " << F.size() << endl;
	return F;
};

void save_intervals(const string name, const multiset<interval>& intervals) {
	ofstream output(name);
	for (auto i: intervals)
		output << i;
	output.close();
}

/*
 * Usage: filtration [file_name] [output_name] [log_prefix]
 * Ex: filtration filtrations/filtration_B.txt intervals/B.txt log/B_
 */
int main(int argc, char** argv) {

	if (argc < 2) {
		cout << "Syntax: " << argv[0] << " <filtration_file>"
				<< " <output_file>" << " <log_file>" << endl;
		return 0;
	}

	string input_file = argv[1];
	string output_file = (argc >= 3) ? argv[2] : "intervals/interval.txt";
	string log_file = (argc >= 4) ? argv[3] : "log/";

#ifndef NORMAL
	// Initialize Google's logging library.
	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::INFO, log_file.c_str());
#endif

	LOG(INFO) << "Reading filtration \"" << input_file << '"' << endl;
	auto F = read_filtration(input_file);
	LOG(INFO) << "Done." << endl;

	// sort all simplexes by insertion time (val)
	sort(F.begin(), F.end());

	LOG(INFO) << "Building boundary matrix..." << endl;
	auto m = make_matrix(F);
	LOG(INFO) << "Matrix dimension: " << m.size() << "x" << m.size() << endl;

	LOG(INFO) << "Reducing matrix..." << endl;
	reduction(m);

	LOG(INFO) << "Calculating intervals..." << endl;
	auto res = get_intervals(m, F);
	LOG(INFO) << "Done. " << res.size() << " intervals." << endl;

	save_intervals(output_file, res);
	LOG(INFO) << "Intervals saved." << endl;

	return EXIT_SUCCESS;
}
