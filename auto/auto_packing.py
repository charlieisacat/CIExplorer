import os
from common import run_command, is_dir_empty, run_shell_command
from config import custom_adaptor, base_dir

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

threshold = 3
for benchmark in benchmarks:
    print(benchmark)
    bench, alg = benchmark.split("/")

    input_dir = os.path.join(base_dir, bench, alg)

    for setting in os.listdir(input_dir):
        print("setting", setting)
        setting_path = os.path.join(input_dir, setting)

        if not os.path.isdir(setting_path):
            continue

        if is_dir_empty(setting_path):
            continue

        pattern_path = os.path.join(setting_path, "to_debug_pass")
        cp_ops_path = os.path.join(setting_path, "cp_ops_data")
        fu_ops_path = os.path.join(setting_path, "fu_ops_data")
        bb_list_path = os.path.join(setting_path, "bblist.txt")

        ir_path = os.path.join(setting_path, bench + "_run_rn.bc")
        threshold_str = str(threshold)
        if setting == "novia" or setting == "flix":
            threshold_str = "-1"

        log = run_command(
            [
                custom_adaptor,
                ir_path,
                pattern_path,
                cp_ops_path,
                fu_ops_path,
                "3",
                bb_list_path,
                setting,
                threshold_str,
                "1.",
                "-1"
            ]
        )
        print(log)

        run_command(["mv", setting + ".ll", setting_path])
        run_command(["mv", setting + ".yaml", setting_path])

        break
