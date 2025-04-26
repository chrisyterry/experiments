# lines
# triangles
# bounding cubes
# 2D polygons

import numpy as np
import transforms as tf

class Shape:
    _transform = None # transform of shape from refernce coordinate system
    _transform_inv = None # inverse transform of shape from reference coordinate system
    def __init__(self, dimensions:int, transform=None):
        if not transform is None:
            # checks and transform initialization
            assert (len(transform.shape) == 2) and (transform.shape[0] == transform.shape[1])
            assert (dimensions + 1) == transform.shape[0]
            self._transform = transform
            self._transform_inv = np.linalg.inv(self._transform)

    def _transformRenderPoints(self, render_points:np.array):
        # transform the points to world coordinates
        if self._transform is not None:
            transformed_points = np.zeros(render_points.shape)
            for row in range(0, render_points.shape[0]):
                transformed_points[row, :] = tf.transformVector(render_points[row, :], self._transform)
            render_points = transformed_points

        return render_points

"""
class defining a line segment and various useful methods for working with line segments
"""
class LineSegment(Shape):
    pass

class BoundingBox(Shape):
    _points = np.zeros((2,3)) # first row is min, second row is max

    # construct bounding box from pre-specified points
    def __init__(self, min_point:np.array, max_point:np.array, transform=None):
        # checks and shape initialization
        assert len(max_point.shape) == 1 and len (min_point.shape) == 1
        assert max_point.shape[0] == min_point.shape[0] and max_point.shape[0]
        Shape.__init__(self, max_point.shape[0], transform)

        self._points = np.vstack((min_point, max_point))

    # determine if point is within bounding box; point is assumed to be in box coordinates
    def pointInBox(self, point:np.array) -> bool:
        assert len(point.shape) == 1 and point.shape[0] == self._max_point.shape[0]

        for i in range(0, point.shape[0]):
            if self._max_point < point[i] or self._min_point > point[i]:
                return False
            
        return True
    
    # determine if the specified ray intersects the bounding box (based on https://tavianator.com/2022/ray_box_boundary.html)
    # ray is assumed to be in box coordinates
    def rayIntersect(self, ray_origin:np.array, direction:np.array) -> float:
        t_min = 0.0
        t_max = float('inf')
        # for each dimension
        for i in range(0,3):
            dir_inv = 1/direction[i]

            # determine which points to use as min and max
            sign_bit = dir_inv > 0
            box_min = self._points[int(not sign_bit),i]
            box_max = self._points[int(sign_bit), i]

            d_min = (box_min - ray_origin[i]) * dir_inv
            d_max = (box_max - ray_origin[i]) * dir_inv

            t_min = max(d_min, t_min)
            t_max = min(d_max, t_max)

        return t_min <= t_max
    
    def getRenderingPoints(self):

        min_point = self._points[0,:]
        max_point = self._points[1,:]
        render_points = np.array([[min_point[0], min_point[1], min_point[2]],
                                [max_point[0], min_point[1], min_point[2]], 
                                [max_point[0], max_point[1], min_point[2]],
                                [min_point[0], max_point[1], min_point[2]],
                                [min_point[0], min_point[1], min_point[2]], # end of first level
                                [min_point[0], min_point[1], max_point[2]],
                                [max_point[0], min_point[1], max_point[2]],
                                [max_point[0], min_point[1], min_point[2]],
                                [max_point[0], min_point[1], max_point[2]],
                                [max_point[0], max_point[1], max_point[2]],
                                [max_point[0], max_point[1], min_point[2]],
                                [max_point[0], max_point[1], max_point[2]],
                                [min_point[0], max_point[1], max_point[2]],
                                [min_point[0], max_point[1], min_point[2]],
                                [min_point[0], max_point[1], max_point[2]],
                                [min_point[0], min_point[1], max_point[2]]]) # end of second level)

        return self._transformRenderPoints(render_points)
    
class Triangle(Shape):
    _v0=np.zeros(3) # vertex 0
    _v1=np.zeros(3) # vertex 1
    _v2=np.zeros(3) # vertex 2
    _e01=np.zeros(3) # edge going from vertex 0 to vertex 1
    _e02=np.zeros(3) # edge going from vertex 0 to vertex 2
    _e02_dot_e02=0 # dot product between edge 2 and itself
    _e02_dot_e01=0 # dot product between edge 2 and edge 1
    _e01_dot_e01=0 # dot product between edge 1 and itself
    _barycentric_denom_inv=0 # inverse denominator for barycentric coordinate calculation
    _plane_normal=np.zeros(3) # the normal for the triangle plane
    _degenerate=False # whether the triangle is degenerate/all the points fall along a single line

    # construct the class from vertex positions; optionally flip the normal
    # ToDo, we need a way of associating quantities with vertices
    def __init__(self, v0:np.array, v1:np.array, v2:np.array, transform=None, invert_normal:bool=False):
        # checks and shape initalization
        assert len(v0.shape) == 1 and len(v1.shape) == 1 and len(v2.shape) == 1
        assert v0.shape[0] == v1.shape[0] and v0.shape[0] == v2.shape[0]
        Shape.__init__(self, v0.shape[0], transform)

        # store vertices
        self._v0 = v0
        self._v1 = v1
        self._v2 = v2
        # store relevant edges
        self._e01 = v2 - v1
        self._e02 = v1 - v0
        # store dot products
        self._e02_dot_e02 = np.dot(self._e02, self._e02)
        self._e02_dot_e01 = np.dot(self._e02, self._e01)
        self._e01_dot_e01 = np.dot(self._e01, self._e01)

        # if the barycentric denominator is zero, the points are colinear and the triangle is degenerate
        barycentric_denom = self._e02_dot_e02*self._e01_dot_e01 - self._e02_dot_e01*self._e02_dot_e01
        if (barycentric_denom == 0.0):
            self._degenerate = True
            return
        
        self._barycentric_denom_inv = 1/barycentric_denom
        self._plane_normal = np.cross(self._e01, self._e02)

    # find the intersect between the triangle and the ray (assumes ray is in triangle coordinates)
    def rayIntersect(self, ray_origin:np.array, ray_direction:np.array):
        assert len(ray_origin.shape) == 1 and len(ray_direction.shape) == 1
        assert ray_origin.shape[0] == ray_direction.shape[0] and ray_origin.shape[0] == self._v0.shape[0]

        # normalize the ray direction
        unit_direction = ray_direction / np.atleast_1d(np.linalg.norm(ray_direction, 2, -1))

        # if ray is pretty much parallel to the triangle plane
        denominator = np.dot(self._plane_normal, unit_direction)
        if abs(denominator) < 1e-8:
            return None
        
        # get the intersect between the ray and the triangle plane
        t = np.dot(self._plane_normal, (self._v0 - ray_origin))
        intersect = ray_origin + t * unit_direction

        # determine if the itnersect is within the bounds of the triangle
        barycentric_intersect = self.pointToBarycentric(intersect)
        if (barycentric_intersect[0] >= 0 and barycentric_intersect[0] <= 1) and \
           (barycentric_intersect[1] >= 0 and barycentric_intersect[1] <= 1) and\
           (barycentric_intersect[2] >= 0 and barycentric_intersect[2] <= 1):
            return intersect
        else:
            return None

    # get the specified point in barycentric coordinates
    def pointToBarycentric(self, pt:np.array):
        assert len(pt.shape) == 1
        assert pt.shape[0] == self._v0.shape[0]

        barycentric_pt = np.zeros(3)

        v0_to_pt = pt - self._v0
        e01_dot  = np.dot(self._e01, v0_to_pt)
        e02_dot  = np.dot(self._e02, v0_to_pt)

        barycentric_pt[0] = (self._e01_dot_e01 * e02_dot - self._e02_dot_e01 * e01_dot) * self._barycentric_denom_inv
        barycentric_pt[1] = (self._e02_dot_e02 * e01_dot - self._e02_dot_e01 * e02_dot) * self._barycentric_denom_inv 
        barycentric_pt[2] = 1 - barycentric_pt[0] - barycentric_pt[1]

        return barycentric_pt
    
    def getRenderingPoints(self):
        render_points = np.array([self._v0, self._v1, self._v2, self._v0])
        return self._transformRenderPoints(render_points)


class Polygon2D(Shape):

    # intersect test
    # convex hull
    # combining polygons
    # mean value coordinate interpolation
    pass

# test code
def main():
  from math import radians
  from plotting import makeAxesEqual
  import plotly.graph_objects as go

  # test bounding box
  tform_box = tf.create3DTransformFromOffsets(radians(35), radians(30), radians(35), -0.5, -0.5, 1)
  test_box = BoundingBox(np.array((-1, -1, -1)), np.array((1, 1, 1)), tform_box)

  print("box transform", test_box._transform)

  # test triangle
  tform_triangle = tf.create3DTransformFromOffsets(radians(45), radians(60), radians(45), 0, 0, 1.5)
  test_triangle = Triangle(np.array([0,0,0]), np.array([1,0,0]), np.array([1,1,0]), tform_triangle)

  print("triangle transform", test_triangle._transform)

  # test line
  test_line_start = np.array((0, 0 ,0))
  test_line_end = np.array((1,0,0))
  tform_line = tf.create3DTransformFromOffsets(radians(0), radians(0), radians(0), 0, 0, 2.1)
  test_line_start = tf.transformVector(test_line_start, tform_line)
  test_line_end = tf.transformVector(test_line_end, tform_line)

  # triangle intersect test, arbitrary placement
  test_line_start_triangle = tf.transformVector(test_line_start, test_triangle._transform_inv)
  test_line_end_triangle = tf.transformVector(test_line_end, test_triangle._transform_inv)
  triangle_intersect = not (test_triangle.rayIntersect(test_line_start_triangle, test_line_end_triangle - test_line_start_triangle) is None)

  # bounding box intersect test, arbitrary placement
  test_line_start_box = tf.transformVector(test_line_start, test_box._transform_inv)
  test_line_end_box = tf.transformVector(test_line_end, test_box._transform_inv)
  box_intersection = test_box.rayIntersect(test_line_start_box, test_line_end_box - test_line_start_box)

  non_intersect_color = "rgb(255,0,0)"
  intersect_color = "rgb(0,255,0)"

  box_color = non_intersect_color
  triangle_color = non_intersect_color
  if (box_intersection):
     box_color=intersect_color
  if (triangle_intersect):
      triangle_color=intersect_color
    

  line_data=np.vstack([test_line_start, test_line_end])

  test_box_points = test_box.getRenderingPoints()
  test_triangle_points = test_triangle.getRenderingPoints()

  box_1 = go.Scatter3d(x=test_box_points[:,0], y=test_box_points[:,1], z=test_box_points[:,2], mode='lines',line=dict(color=box_color))
  triangle_1 = go.Scatter3d(x=test_triangle_points[:,0], y=test_triangle_points[:,1], z=test_triangle_points[:,2], mode='lines',line=dict(color=triangle_color))
  line_1 = go.Scatter3d(x=line_data[:,0], y=line_data[:,1], z=line_data[:,2], mode='lines',line=dict(color="rgb(0,0,0)"))
  fig = go.Figure()
  fig.add_trace(box_1)
  fig.add_trace(triangle_1)
  fig.add_trace(line_1)
  makeAxesEqual(fig)
  fig.show()

if __name__ == '__main__':
    main()
