from config import mining_root, basepath

bench_type = ""
benchmarks = [
   "aes/aes",
   "bfs/bulk",
    "bfs/queue",
    "fft/strided",
  "fft/transpose",
   "gemm/ncubed",
   "gemm/blocked",
   "kmp/kmp",
   "md/grid",
   "md/knn",
   "nw/nw",
   "sort/merge",
   "sort/radix",
    "spmv/crs",
    "spmv/ellpack",
   "stencil/stencil2d",
   "stencil/stencil3d",
   "viterbi/viterbi",
]

gpp_type="ooo2"
config_path="/root/nio/patt_mining/src/cost_model/"+gpp_type+".yml"
print(config_path)

conf_str = ""

import os
import shutil
from common import run_command


def dump_mining_log(destfile, log):
    with open(destfile, "w") as wf:
        wf.write(log)


def chdir(dir):
    os.chdir(dir)


def set_os_env(nin, nout, nonconst, BENCH):
    os.environ["Nin"] = str(nin)
    os.environ["Nout"] = str(nout)
    os.environ["NOCONST"] = str(nonconst)
    os.environ["BENCH"] = BENCH


def set_exe_arg_env(bench, alg):
    os.environ["MINING_ROOT"] = mining_root
    benchpath = os.path.join(basepath, bench, alg)
    os.environ["EXECUTABLE"] = os.path.join(benchpath, bench + "_run.bc")
    os.environ["EXECARGS"] = os.path.join(benchpath, "input.data")
    os.environ["EXCL_FILE"] = os.path.join(mining_root, "bin/run_shells", "exclfile")
    print(os.environ["EXCL_FILE"])


def clean_mining_path(hw_path, mining_path):
    os.chdir(hw_path)
    shutil.rmtree(path=mining_path)


settings = [
    [1024, 1024, 0],
]


def cp_result(run_path, bench, mining_path, out_dir):
    run_command(["cp", "-r", os.path.join(mining_path, "to_debug_pass"), out_dir])
    run_command(["cp", "-r", os.path.join(mining_path, "cp_ops_data"), out_dir])
    run_command(["cp", "-r", os.path.join(mining_path, "fu_ops_data"), out_dir])
    # run_command(["cp", "-r", os.path.join(mining_path, "graphdata_step1"), out_dir])
    # run_command(["cp", "-r", os.path.join(mining_path, "gspan_output_step1"), out_dir])
    # run_command(["cp", "-r", os.path.join(mining_path, "../data"), out_dir])
    run_command(
        ["cp", "-r", os.path.join(run_path, "patt_mining/data/bblist.txt"), out_dir]
    )

    run_command(
        [
            "cp",
            os.path.join(
                run_path,
                "patt_mining/bitcode",
                bench + "_run_rn.bc",
            ),
            out_dir,
        ]
    )

    run_command(["cp", "-r", os.path.join(bench_dir, "example.txt"), out_dir])

def clean(run_path, mining_path):
    os.chdir(run_path)
    shutil.rmtree(path=mining_path)
    os.makedirs(mining_path)
    os.chdir(mining_path)

current_path = os.getcwd()

for benchmark in benchmarks:
    print("==============", benchmark)

    bench_dir = os.path.join(basepath, benchmark)
    bench, alg = benchmark.split("/")

    os.chdir(bench_dir)
    run_command("ls")
    run_command(["make", "clean"])
    run_command("make")
    run_command(os.path.join(".", bench + "_run"))

    hw_path = os.path.join(bench_dir)

    base_out_dir = os.path.join(current_path, "results", benchmark)

    print("outdir", base_out_dir)

    if os.path.exists(base_out_dir):
        shutil.rmtree(path=base_out_dir)
    os.makedirs(base_out_dir)

    set_exe_arg_env(bench, alg)

    # go to mining path
    run_path = os.path.join(mining_root, "bin/run_shells")
    os.chdir(run_path)
    run_command(["rm", "-rf", "patt_mining"])

    print("run prepare")
    log = run_command(os.path.join(run_path, "prepare.sh"))
    print(log)

    mining_path = os.path.join(run_path, "patt_mining/temp")
    os.chdir(mining_path)
    
    for setting in settings:
        clean(run_path, mining_path)

        print("setting", setting)
        set_os_env(setting[0], setting[1], setting[2], bench)

        out_dir = os.path.join(base_out_dir, str(setting[0]) + "_" + str(setting[1]))
        os.makedirs(out_dir)
        
        # for IO GPP
        # log = run_command(["../../mining_ino.sh", config_path, "3"])
        log = run_command(["../../mining.sh", config_path, "3"])

        print(log)
        cp_result(run_path, bench, mining_path, out_dir)
