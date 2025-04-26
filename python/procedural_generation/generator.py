import numpy as np
import plotly
from math import radians

# add common utils directory
import sys
import os
import pathlib
sys.path.insert(0, os.path.join(pathlib.Path(__file__).parents[1], 'common'))
import transforms as tf
import geometry as geom

# we might want the option to have the target be anywhere within a certain region on a bounding box
# we want to be able to recursively plan sections by specifying a bounding region, start and end goal

# set target 
# look at possible ways we could go
# generate new pose and link
#

def main():
  pass

if __name__ == '__main__':
    main()