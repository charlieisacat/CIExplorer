#ifndef UTILS_H
#define UTILS_H
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

#include "llvm.h"
#include "graph.h"

using namespace std;
using namespace llvm;

// Define a data structure to hold the data dependency graph
using DependencyGraph = map<Instruction *, vector<Instruction *>>;
using Pattern = vector<Graph *>;

vector<Pattern *> read_pattern_data(string filename);
vector<Graph *> read_graph_data(string filename);
vector<Graph *> read_graph_data_v2(string filename);
void remove_illegal_patts(vector<Pattern *> &patts);
void read_bblist(string filename, vector<string>& bblist);

string split_str(string filename, char split, int idx);
void read_dyn_info(string filename, map<string, long> *iter_map);
void bind_dyn_info(map<string, long> *iter_map, vector<Graph *> graphs);

string bitwise_or(string s1, string s2);
string bitwise_reverse(string s);

string split_str(string str, char split, int idx);

string insn_to_string(Value *v);

#endif