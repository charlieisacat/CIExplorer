import os
import subprocess

def run_command(args):
    process = subprocess.Popen(args, stdout=subprocess.PIPE)
    output, error = process.communicate()
    return output.decode()

def is_dir_empty(dir_path):
    return not any(os.scandir(dir_path))

def run_shell_command(args):
    process = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    output, error = process.communicate()
    return output.decode()
