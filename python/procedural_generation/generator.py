import numpy as np
import plotly
from math import radians

# add common utils directory
import sys
import os
import pathlib
sys.path.insert(0, os.path.join(pathlib.Path(__file__).parents[1], 'common'))
import transforms
import geometry

# set target 
# look at possible ways we could go
# generate new pose and link
#

def main():

  tform = transforms.create3DTransformFromOffsets(radians(0), radians(0), radians(0), 0, 0, 1)
  test_box = geometry.BoundingBox(np.array((0, 0, 0)), np.array((1, 1, 1)))

  start_pos = np.zeros(3)

  test_line_start = np.array((0.9,1,1))
  test_line_end = np.array((2,2,2))
  intersection = test_box.rayBoxIntersect(test_line_start, test_line_end - test_line_start)

  line_color = "rgb(255,0,0)"
  if (intersection):
     line_color="rgb(0,0,255)"

  line_data=np.vstack([test_line_start, test_line_end])

  all_points = start_pos
  for i in range(0,9):
      if len(all_points.shape) == 1:
        all_points = np.vstack((all_points, transforms.transformVector(all_points, tform)))
      else:
        all_points = np.vstack((all_points, transforms.transformVector(all_points[-1,:], tform)))

  goal_pos = np.zeros(3)

  test_box_points = test_box.getRenderingPoints()

  import plotly.graph_objects as go
  box_1 = go.Scatter3d(x=test_box_points[:,0], y=test_box_points[:,1], z=test_box_points[:,2], mode='lines',line=dict(color="rgb(0,0,255)"))
  line_1 = go.Scatter3d(x=line_data[:,0], y=line_data[:,1], z=line_data[:,2], mode='lines',line=dict(color=line_color))
  fig = go.Figure()
  fig.add_trace(box_1)
  fig.add_trace(line_1)
  fig.show()

if __name__ == '__main__':
    main()