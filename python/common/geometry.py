# lines
# triangles
# bounding cubes
# 2D polygons

import numpy as np
import transforms as tf
import misc
import plotting as plt

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
    
    def _transform(self, tform:np.array):
        if self._transform is not None:
            self._transform = tf.combineTransforms(self._transform, tform)
            self._transform_inv = np.linalg.inv(self._transform)


"""
class defining a line either as a segment or a ray
"""
class Line(Shape):
    _start=np.zeros(2)
    _direction=np.zeros(2)
    _end=None
    def __init__(self, start:np.array, direction=None, end=None, tform:np.array=None):

        assert len(start.shape) == 1
        self._start = start

        # if a driection has been specified
        if direction is not None:
            assert isinstance(direction, np.ndarray)
            assert len(direction.shape) == 1 and direction.shape[0] == self._start.shape[0]
            self._direction = direction
        elif end is not None:
            assert isinstance(end, np.ndarray)
            assert len(end.shape) == 1 and end.shape[0] == self._start.shape[0]
            self._end = end
            self._direction = self._end - self._start

        Shape.__init__(self, start.shape[0], tform)

    def dimensions(self):
        return self._start.shape[0]
    
    def transformed(self, tform:np.array):
        assert len(tform.shape) == 2 and tform.shape[0] == tform.shape[1]
        assert tform.shape[0] == self._start.shape[0] + 1

        start = tf.transformVector(self._start, tform)
        if self._end is not None:
            end = tf.transformVector(self._end, tform)
            return Line(start, None, end)
        else:
            direction = tf.transformVector(self._direction, tform, False)
            return Line(start, direction)
        
    def transform(self, tform:np.array):
        assert len(tform.shape) == 2 and tform.shape[0] == tform.shape[1]
        assert tform.shape[0] == self._start.shape[0] + 1

        self._start = tf.transformVector(self._start, tform)
        self._direction = tf.transformVector(self._direction, tform, False)
        if self._end is not None:
            self._end = tf.transformVector(self._end, tform)

    # based on https://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments
    def closestPoints(self, other):
        """
        Get the closest points on two line segments and the distance between them
        """
        
        epsilon = 1e-6

        # get required direction vectors
        starts_dir = self._start - other._start

        self_dir_sqrd = np.dot(self._direction,self._direction)
        self_other_dir_dot = np.dot(self._direction, other._direction)
        other_dir_sqrd = np.dot(other._direction, other._direction)
        self_start_dir_dot = np.dot(self._direction, starts_dir)
        other_starts_dir_dot = np.dot(other._direction, starts_dir)

        determinant = (self_dir_sqrd * other_dir_sqrd) - (self_other_dir_dot * self_other_dir_dot)

        # Parallel or colinear segments
        if determinant < epsilon:
            # find which end points are the closest
            other_start_dist, closest_to_other_start = self.closestPoint(other._start, self)
            other_end_dist, closest_to_other_end = self.closestPoint(other._end, self)
            self_start_dist, closest_to_self_start = self.closestPoint(self._start, other)
            self_end_dist, closest_to_self_end = self.closestPoint(self._end, other)

            line_data = [(other_start_dist, closest_to_other_start, other._start),
                         (other_end_dist, closest_to_other_end, other._end),
                         (self_start_dist, self._start, closest_to_self_start),
                         (self_end_dist, self._end, closest_to_self_end)]
            
            # get the closest pair of points and return
            closest_data = min(line_data, key=lambda x: x[0])
            return closest_data[0], closest_data[1], closest_data[2]

        # non-parallel segments
        else:
            det_inv = 1/determinant

            self_t_inf = (self_other_dir_dot * other_starts_dir_dot - other_dir_sqrd * self_start_dir_dot) * det_inv
            other_t_inf = (self_dir_sqrd * other_starts_dir_dot - self_other_dir_dot * self_start_dir_dot) * det_inv

            # clamp 
            self_t_clamped = max(0.0, min(1.0, self_t_inf))
            other_t_clamped = max(0.0, min(1.0, other_t_inf))

            # if t has to be clamped for the other line
            if self_t_clamped != self_t_inf:
                other_t_clamped = (self_other_dir_dot * self_t_clamped - other_starts_dir_dot) / other_dir_sqrd if other_dir_sqrd > epsilon else other_t_clamped
                other_t_clamped = min(0.0, min(1.0, other_t_clamped))

            # if t has to be clamped for this line
            if other_t_clamped != other_t_inf:
                self_t_clamped = (self_other_dir_dot * other_t_clamped - self_start_dir_dot) / self_dir_sqrd if self_dir_sqrd > epsilon else self_t_clamped
                self_t_clamped = max(0.0, min(1.0, self_t_inf))

            closest_on_self = self._start + self_t_clamped * self._direction
            closest_on_other = other._start + other_t_clamped * other._direction

            return np.linalg.norm(closest_on_other - closest_on_self), closest_on_self, closest_on_other

    def closestPoint(self, point:np.array, segment):
        start_to_point = point - segment._start

        seg_mag_sqrd = np.dot(segment._direction, segment._direction)
        # segment is a single point
        if seg_mag_sqrd == 0.0:
            return self._start
        
        t = np.dot(start_to_point, segment._direction) / seg_mag_sqrd
        t = max(0.0, min(1.0, t))

        closest_point = self._start + t * segment._direction

        return np.linalg.norm(closest_point - point), closest_point


    def getRenderingPoints(self):
        return np.vstack([self._start, self._end])

    def __str__(self):
        return "\n\tstart: " + str(self._start) + "\n\tend: " + str(self._end) + "\n\tdirection: " + str(self._direction)
        

class BoundingBox(Shape):
    _points = np.zeros((2,3)) # first row is min, second row is max

    # construct bounding box from pre-specified points
    def __init__(self, min_point:np.array, max_point:np.array, transform=None):
        # checks and shape initialization
        assert len(max_point.shape) == 1 and len (min_point.shape) == 1
        assert max_point.shape[0] == min_point.shape[0] and max_point.shape[0]
        Shape.__init__(self, max_point.shape[0], transform)

        self._points = np.vstack((min_point, max_point))

    def __str__(self):
        return "\n\tmin: " + str(self._points[0,:]) + "\n\tmax: " + str(self._points[1,:])

    # determine if point is within bounding box; point is assumed to be in box coordinates
    def pointInBox(self, point:np.array) -> bool:
        assert len(point.shape) == 1 and point.shape[0] == self._max_point.shape[0]

        for i in range(0, point.shape[0]):
            if self._max_point < point[i] or self._min_point > point[i]:
                return False
            
        return True
    
    # determine if the specified ray intersects the bounding box (based on https://tavianator.com/2022/ray_box_boundary.html)
    # ray is assumed to be in box coordinates
    def rayIntersect(self, ray:Line) -> float:

        assert self._points.shape[1] == ray.dimensions()

        t_min = 0.0
        t_max = float('inf')
        # for each dimension
        for i in range(0,3):
            dir_inv = 1/ray._direction[i]

            # determine which points to use as min and max
            sign_bit = dir_inv > 0
            box_min = self._points[int(not sign_bit),i]
            box_max = self._points[int(sign_bit), i]

            d_min = (box_min - ray._start[i]) * dir_inv
            d_max = (box_max - ray._start[i]) * dir_inv

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

    def __str__(self):
        return "\n\tv0: " + str(self._v0) + "\n\tv1: " + str(self._v1) + "\n\tv2: " + str(self._v2)

    # find the intersect between the triangle and the ray (assumes ray is in triangle coordinates)
    def rayIntersect(self, ray:Line):
        assert ray.dimensions() == self._v0.shape[0]

        # normalize the ray direction
        unit_direction = ray._direction / np.atleast_1d(np.linalg.norm(ray._direction, 2, -1))

        # if ray is pretty much parallel to the triangle plane
        denominator = np.dot(self._plane_normal, unit_direction)
        if abs(denominator) < 1e-8:
            return None
        
        # get the intersect between the ray and the triangle plane
        t = np.dot(self._plane_normal, (self._v0 - ray._start))
        intersect = ray._start + t * unit_direction

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

  # test bounding box
  tform_box = tf.create3DTransformFromOffsets(radians(30), radians(30), radians(30), -0.5, -0.5, 1)
  test_box = BoundingBox(np.array((-1, -1, -1)), np.array((1, 1, 1)), tform_box)

  #print("box transform", test_box._transform)

  # test triangle
  tform_triangle = tf.create3DTransformFromOffsets(radians(45), radians(60), radians(45), 0, 0, 1.5)
  test_triangle = Triangle(np.array([0,0,0]), np.array([1,0,0]), np.array([1,1,0]), tform_triangle)

  #print("triangle transform", test_triangle._transform)

  # test ray
  test_line = Line(np.array((0, 0 ,0)), None, np.array((1,0,0))) # specify line with an ends
  tform_line = tf.create3DTransformFromOffsets(radians(0), radians(0), radians(0), 0, 0, 1)
  test_line.transform(tform_line)

  # triangle intersect test, arbitrary placement
  test_line_triangle = test_line.transformed(test_triangle._transform_inv)
  triangle_intersect = not (test_triangle.rayIntersect(test_line_triangle) is None)
  print("triangle: ", test_triangle)

  # bounding box intersect test, arbitrary placement
  test_line_box = test_line.transformed(test_box._transform_inv)
  print("line: ", test_line_box)
  box_intersection = test_box.rayIntersect(test_line_box)
  print("box: ", test_box)

  # line minimum distance test
  min_dist_line_1 = Line(np.array((1, 2, -1)), None, np.array((1, 1, 1)))
  min_dist_line_2 = Line(np.array((3, -1, 0)), None, np.array((-1, 1, 0)))

  dist, l1_pt, l2_pt = min_dist_line_1.closestPoints(min_dist_line_2)

  non_intersect_color = "rgb(255,0,0)"
  intersect_color = "rgb(0,255,0)"

  box_color = non_intersect_color
  triangle_color = non_intersect_color
  if (box_intersection):
     box_color=intersect_color
  if (triangle_intersect):
      triangle_color=intersect_color

  box_1 = (test_box, dict(color=box_color))
  triangle_1 =(test_triangle, dict(color=triangle_color))
  line_1 = (test_line, dict(color="rgb(0,0,0)"))
  fig = plt.plotObjects([box_1, triangle_1, line_1, min_dist_line_1, min_dist_line_2, l1_pt, l2_pt])
  fig.show()

  # ToDo fix everything to work with new line class
  # write cylinder intersect test
  # write min distance to line 

if __name__ == '__main__':
    main()
