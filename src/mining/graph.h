#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <string>
#include <map>

#include "llvm.h"

using namespace std;

class Graph
{
public:
    vector<int> vertices;
    string bbname = "";
    long itervalue = 0;
    double score = 0.;

    map<int, int> uid_vid_map;

    void add_vertex(int uid);

    // used for mia
    int pid = -1;
};

#endif