import sys
import glob
import os
import random

def vecnt(graphdatafile, cntfile):
    rf = open(graphdatafile, "r")
    wf = open(cntfile, "w+")

    v_cnt = {}
    e_cnt = {}

    graph_id = 0
    for line in rf:
        items = [t.strip() for t in line.strip().split(" ")]

        if items[0] == "t":
            edges = []
            vertices = []
        elif items[0] == "v":
            vlb = items[2]
            vertices.append(vlb)
            if vlb in v_cnt.keys():
                v_cnt[vlb] += 1
            else:
                v_cnt[vlb] = 1
        elif items[0] == "e":
            frm_vlb = vertices[int(items[1])]
            to_vlb = vertices[int(items[2])]
            if (frm_vlb, to_vlb) in e_cnt.keys():
                e_cnt[(frm_vlb, to_vlb)] += 1
            else:
                e_cnt[(frm_vlb, to_vlb)] = 1
        else:
            pass

        graph_id += 1
    
    v_cnt = dict(sorted(v_cnt.items(), key = lambda kv:(kv[1]), reverse=True))
    e_cnt = dict(sorted(e_cnt.items(), key = lambda kv:(kv[1]), reverse=True))

    for key, value in v_cnt.items():
        wf.write("v " + key + " " + str(value) + "\n")
    
    for key, value in e_cnt.items():
        wf.write("e " + key[0] + " " + key[1] + " " + str(value) + "\n")
    
    rf.close() 
    wf.close() 
