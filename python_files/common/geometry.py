# lines
# triangles
# bounding cubes
# 2D polygons

import numpy as np
"""
class defining a line segment and various useful methods for working with line segments
"""
class LineSegment:
    pass

class BoundingBox:
    _max_point=np.zeros(3) # maximum extent on each axis
    _min_point=np.zeros(3) # minimum extend on each axis

    # construct bounding box from pre-specified points
    def __init__(self, max_point:np.array, min_point:np.array):

        assert len(max_point.shape) == 1 and len (min_point.shape) == 1
        assert max_point.shape[0] == min_point.shape[0] and max_point.shape[0]

        self._max_point = max_point
        self._min_point = min_point

    def pointInBox(self, point:np.array):
        assert len(point.shape) == 1 and point.shape[0] == self._max_point.shape[0]

        for i in range(0, point.shape[0]):
            if self._max_point < point[i] or self._min_point > point[i]:
                return False
            
        return True

class Triangle:
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
    def __init__(self, v0:np.array, v1:np.array, v2:np.array, invert_normal:bool=False):
        assert len(v0.shape) == 1 and len(v1.shape) == 1 and len(v2.shape) == 1
        assert v0.shape[0] == v1.shape[0] and v0.shape[0] == v2.shape[0]
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

    # find the intersect between the triangle and the ray
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

class Polygon2D:

    # intersect test
    # convex hull
    # combining polygons
    # mean value coordinate interpolation
    pass

# test code
def main():
  triangle = Triangle(np.array((0,0,1)), np.array((1,1,1)), np.array((1,0,1)))
  ray_origin = np.array((0.75,0.5,0))
  ray_direction = np.array((0,1,1))
  intersect = triangle.rayIntersect(ray_origin, ray_direction)
  print(intersect)


if __name__ == '__main__':
    main()
