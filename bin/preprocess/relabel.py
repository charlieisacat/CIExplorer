import sys
import glob
import os
import random

def read_cntfile(filename):
    rf = open(filename, "r")
    vlbs = {}
    elbs = {}

    vlb = 2 
    elb = 2 
    for line in rf:
        items = [t.strip() for t in line.strip().split(" ")]
        if items[0] == "v":
            vlbs[items[1]] = str(vlb)
            vlb += 1
        elif items[0] == "e":
            elbs[(items[1], items[2])] = str(elb)
            elb += 1
        
    rf.close()
    return vlbs, elbs

def relabel(graphdatafile, cntfile, relabelfile):

    rf = open(graphdatafile, "r")
    wf = open(relabelfile, "w+")

    vlbs, elbs = read_cntfile(cntfile)

    graph_id = 0
    for line in rf:
        items = [t.strip() for t in line.strip().split(" ")]

        if items[0] == "t":
            edges = []
            vertices = []
        elif items[0] == "v":
            vlb = items[2]
            vertices.append(vlb)
            items.append(vlbs[vlb])
        elif items[0] == "e":
            frm_vlb = vertices[int(items[1])]
            to_vlb = vertices[int(items[2])]
            items[3] = elbs[(frm_vlb, to_vlb)]
        else:
            pass

        new_line = " ".join(items)
        graph_id += 1
        wf.write(new_line + "\n")
    
    rf.close() 
    wf.close() 
