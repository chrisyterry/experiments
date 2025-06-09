# this file is for testing quickly, if whatever it is turns out to be useful, it should be removed to a concrete file
import sys
import os
import pathlib
sys.path.insert(0, os.path.join(pathlib.Path(__file__).parents[0], 'common'))

print(os.path.join(pathlib.Path(__file__).parents[0], 'common'))

import numpy as np
import math

import randomization as rnd

x = rnd.UniformGenerator(0, 2, 0.5, False)
print(x.generate(10))
y = rnd.BetaGenerator(5,5,0,2,0.5,False)
print(y.generate(10))

# min distance between line segments
#