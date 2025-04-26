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
import plotly.graph_objects as go

# set target 
# look at possible ways we could go
# generate new pose and link
#

def main():
  tform_box = tf.create3DTransformFromOffsets(radians(35), radians(30), radians(35), -0.5, -0.5, 1)
  test_box = geom.BoundingBox(np.array((-1, -1, -1)), np.array((1, 1, 1)), tform_box)

  test_line_start = np.array((0, 0 ,0))
  test_line_end = np.array((1,0,0))
  tform_line = tf.create3DTransformFromOffsets(radians(0), radians(0), radians(0), 0, 0, 2.1)
  test_line_start = tf.transformVector(test_line_start, tform_line)
  test_line_end = tf.transformVector(test_line_end, tform_line)
  test_line_start_box = tf.transformVector(test_line_start, test_box._transform_inv)
  test_line_end_box = tf.transformVector(test_line_end, test_box._transform_inv)
  intersection = test_box.rayBoxIntersect(test_line_start_box, test_line_end_box - test_line_start_box)

  line_color = "rgb(255,0,0)"
  if (intersection):
     line_color="rgb(0,255,0)"

  line_data=np.vstack([test_line_start, test_line_end])

  test_box_points = test_box.getRenderingPoints()

  box_1 = go.Scatter3d(x=test_box_points[:,0], y=test_box_points[:,1], z=test_box_points[:,2], mode='lines',line=dict(color="rgb(0,0,255)"))
  line_1 = go.Scatter3d(x=line_data[:,0], y=line_data[:,1], z=line_data[:,2], mode='lines',line=dict(color=line_color))
  fig = go.Figure()
  fig.add_trace(box_1)
  fig.add_trace(line_1)
  makeAxesEqual(fig)
  fig.show()

# make the axes of a plotly figure equal
def makeAxesEqual(fig:go.Figure):
  axis_lims = [float('inf'), -float('inf')]
  # for each plot on the axes
  for plot in fig.data:
     axis_lims[0] = min(axis_lims[0], min(plot.x))
     axis_lims[0] = min(axis_lims[0], min(plot.y))
     axis_lims[0] = min(axis_lims[0], min(plot.z))
     axis_lims[1] = max(axis_lims[1], max(plot.x))
     axis_lims[1] = max(axis_lims[1], max(plot.y))
     axis_lims[1] = max(axis_lims[1], max(plot.z))

  fig.update_layout(scene=dict(
                    aspectmode='cube',
                    xaxis=dict(range=[axis_lims[0]*1.05, axis_lims[1]*1.05]),
                    yaxis=dict(range=[axis_lims[0]*1.05, axis_lims[1]*1.05]),
                    zaxis=dict(range=[axis_lims[0]*1.05, axis_lims[1]*1.05])))

if __name__ == '__main__':
    main()