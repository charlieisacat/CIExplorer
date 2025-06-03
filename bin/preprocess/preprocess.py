import sys
import os
import shutil

from dfg import process_dfgs
from vecnt import vecnt
from relabel import relabel

if __name__ == "__main__":
    dfgdir = sys.argv[1]
    graphdatafile = sys.argv[2]
    cntfile = sys.argv[3]
    relabelfile = sys.argv[4]
    outdir = sys.argv[5]

    if not os.path.exists(outdir):
        os.makedirs(outdir)
    # else:
    #     shutil.rmtree(path=outdir)
    #     os.makedirs(outdir)
    
    # print(dfgdir)
    
    graphdatafile = os.path.join(outdir, graphdatafile)
    cntfile = os.path.join(outdir, cntfile)
    relabelfile = os.path.join(outdir, relabelfile)

    process_dfgs(dfgdir, graphdatafile)
    vecnt(graphdatafile, cntfile)
    relabel(graphdatafile, cntfile, relabelfile)

# python preprocess.py /root/git/dse_project/scripts/graph_output graphdata vecnt relabeldata
