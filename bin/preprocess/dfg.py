# -*-coding:utf-8 -*-
import sys
import glob
import os
import random


def merge_str_list(items):
    return " ".join(items)

def process_line(items, graph_id):
    def _process_edge(items):
        return merge_str_list(items)

    def _process_vertex(items):
        return merge_str_list(items)

    def _process_graph_label(items, graph_id):
        items[2] = str(graph_id)
        return merge_str_list(items)

    new_line = ""

    if items[0] == "t":
        new_line = _process_graph_label(items, graph_id)
        graph_id += 1
    elif items[0] == "v":
        new_line = _process_vertex(items)
    elif items[0] == "e":
        new_line = _process_edge(items)
    elif items[0] == "n":
        new_line = _process_edge(items)
    elif items[0] == "s":
        new_line = _process_edge(items)
    else:
        print("Error occurs when processing line")

    return graph_id, new_line.strip()

def build_inst_opcode_mapping(items, mapping):
    if items[0] == 'v':
        opcode = items[2]
        inst = items[3]

        if opcode not in mapping.keys():
            mapping[opcode] = inst

def process_dfgs(dfgdir, graphdatafile):

    inst_opcode_mapping = {}

    dfg_files = glob.glob(os.path.join(dfgdir, "*.data"))

    wf = open(graphdatafile, "w+")

    # 每个dfg_file对应一个图
    # metadata保存了dfg_file和graph_id之间的映射关系
    graph_id = 0
    for dfg_file in dfg_files:
        # print("Processing on file", dfg_file)
        rf = open(dfg_file, "r")
        for line in rf:
            items = [t.strip() for t in line.strip().split(" ")]
            graph_id, new_line = process_line(items, graph_id)
            build_inst_opcode_mapping(items, inst_opcode_mapping)

            if new_line != "": 
                wf.write(new_line + "\n")

        rf.close()
    
    wf.close() 

