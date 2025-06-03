export MINING_ROOT="/root/git/patt_mining_llvm"

#export EXECUTABLE="/root/mach_ir/fft/transpose/fft_run.bc"
#export EXECARGS="/root/mach_ir/fft/transpose/input.data"
#export EXCL_FILE="root/mach_ir/fft/transpose/exclfile"

export EXECUTABLE="/root/mach_ir/stencil/stencil3d/stencil_run.bc"
export EXECARGS="/root/mach_ir/stencil/stencil3d/input.data"
export EXCL_FILE="/root/git/patt_mining/bin/run_shells/exclfile"

#export EXECUTABLE="/root/mach_ir/bfs/bulk/bfs_run.bc"
#export EXECARGS="/root/mach_ir/bfs/bulk/input.data"
#export EXCL_FILE="/root/git/patt_mining/bin/run_shells/exclfile"

#export EXECUTABLE="/root/mach_ir/md/grid/md_run.bc"
#export EXECARGS="/root/mach_ir/md/grid/input.data"
#export EXCL_FILE="/root/git/patt_mining/bin/run_shells/exclfile"

#export EXECUTABLE="/root/mach_ir/gemm/blocked/gemm_run.bc"
#export EXECARGS="/root/mach_ir/gemm/blocked/input.data"
#export EXCL_FILE="/root/git/patt_mining/bin/run_shells/exclfile"

#export EXECUTABLE="/root/mach_ir/fft/strided/fft_run.bc"
#export EXECARGS="/root/mach_ir/fft/strided/input.data"
#export EXCL_FILE="/root/git/patt_mining/bin/run_shells/exclfile"

export Nin=1024
export Nout=1024

export BENCH=stencil

export CONF_USESUM=0
export CONF_DOSPLIT2=0
export CONF_CONN=1
export CONF_NOCONST=1
export CONF_SEEDPATT=0
