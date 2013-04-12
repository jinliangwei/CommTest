import os
import sys

# CAVEAT: (-m)machine option should be smaller than np option by 1. Hidden one is server machine 
os.system("mpirun -np 3 -machinefile machine3.vm ./dynamic -d./tmpdata/human_X.txt.463.4000 -o./tmpdata/human_Y.txt -m2 -t7");
