import numpy as np
import plotly

class BoundingBox:
  _world_origin=True # if true, bounding box is axis-aligned and all values are in world coordinates
  _origin # the origin of the bounding box in world coordinates
  _max_point # the max point of the bounding box in local coordiantes
  _min_point # the min point of the bounding box in local coordiantes

