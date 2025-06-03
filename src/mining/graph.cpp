#include "graph.h"

void Graph::add_vertex(int uid)
{
    uid_vid_map.insert(make_pair(uid, vertices.size()));
    vertices.push_back(uid);
}