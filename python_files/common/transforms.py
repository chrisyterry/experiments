import numpy as np
from math import cos, sin, comb

# row-major transforms can be combined by doing T1xT2xT3

"""
create a 3D row-major transform from the specified roll, pitch and yaw along with translations on the x, y and z axes
"""
def create3DTransformFromOffsets(roll:float, pitch:float, yaw:float, tx:float, ty:float, tz:float):
  # pre-calculate sines and cosines for each angle
  p_cos = cos(pitch)
  p_sin = sin(pitch)
  r_cos = cos(roll)
  r_sin = sin(roll)
  y_cos = cos(yaw)
  y_sin = sin(yaw)

  # setup transform
  transform = np.identity(4)
  transform[0,0] = y_cos*p_cos
  transform[0,1] = (y_cos*p_sin*r_sin)-(y_sin*r_cos)
  transform[0,2] = (y_cos*r_sin*p_cos)+(y_sin*r_sin)
  transform[0,3] = tx
  transform[1,0] = y_sin*p_cos
  transform[1,1] = (y_sin*p_sin*r_sin)+(y_cos*r_cos)
  transform[1,2] = (y_sin*p_sin*r_cos)-(y_cos*r_sin)
  transform[1,3] = ty
  transform[2,0] = -p_sin
  transform[2,1] = p_cos*r_sin
  transform[2,2] = p_cos*r_cos
  transform[2,3] = tz

  return transform
"""
create a 2D row-major transform from the specified yaw along with translations on the x and y
"""
def create2DTransformFromOffsets(yaw:float, tx:float, ty:float):
  # pre-calculate yaw sine and cosine
  y_cos = cos(yaw)
  y_sin = sin(yaw)

  # setup transform
  transform = np.identity(3)
  transform[0,0]=y_cos
  transform[0,1]=-y_sin
  transform[0,2]=tx
  transform[1,0]=y_sin
  transform[1,1]=y_cos
  transform[1,2]=ty
  return transform
"""
create a 3D transformation from a 2D transformation
"""
def transform2DTo3D(transform:np.array):
    assert transform.shape[0] == transform.shape[1]
    assert transform.shape[0] == 2
    return np.block([[transform, np.zeros((2,1))], [np.zeros(1,2), 1]])
"""
create a 2D transformation from a 3D transformation
"""
def transform3DTo2D(transform:np.array):
    assert transform.shape[0] == transform.shape[1]
    assert transform.shape[0] == 3
    return transform[:2,:2]

"""
efficently(?) transform a point using a homogenous transform
"""
def transformPoint(point:np.array, transform:np.array):
    assert transform.shape[0] == transform.shape[1]
    assert len(point.shape) == 1 and point.shape[0] == transform.shape[1] - 1

    transformed_point = np.zeros(point.shape)
    match point.shape[0]:
       # avoid useless 0x calculations
        case 2:
            transformed_point[0] = point[0]*transform[0, 0] + point[1]*transform[0, 1] + transform[0, 2]
            transformed_point[1] = point[1]*transform[1, 0] + point[1]*transform[1, 1] + transform[1, 2]
        case 3:
            transformed_point[0] = point[0]*transform[0, 0] + point[1]*transform[0, 1] + point[2]*transform[0, 2] + transform[0, 3]
            transformed_point[1] = point[0]*transform[1, 0] + point[1]*transform[1, 1] + point[2]*transform[1, 2] + transform[1, 3]
            transformed_point[2] = point[0]*transform[2, 0] + point[1]*transform[2, 1] + point[2]*transform[2, 2] + transform[2, 3]
        case _:
          raise ("point size of " + point.shape[1] + " is not supported!")
       
    return transformed_point

def transformPose(pose:np.array, transform:np.array):

class Pose:
    position=np.zeros(3) # likely (but not strictly) meters
    orientation=np.zeros(3) # radians

    # initialize the pose from position and orientation
    def Pose(self, position:np=np.zeros(3), orientation:np.array=np.zeros(3)):
      self.setPositionAndOrientation(position, orientation)

    # set the position and orientation directly
    def setPositionAndOrientation(self, position:np=np.zeros(3), orientation:np.array=np.zeros(3)):
       assert len(position.shape) == 1 and len(orientation.shape) == 1
       assert orientation.shape == comb(position.shape[0], 2) # orienations are related to pairs of axes)
       self.position = position
       self.orientation = orientation

    # set the position and orientation from a row-major transform
    def setFromTransform(self, transform:np.array):
       
       assert len(transform.shape) == 2 and transform.shape[0] == transform.shape[1]

       # size the position and oreintation
       position=np.zeros(transform.shape[0]-1)
       orientation=np.zeros(comb(position.shape[0],2))

       # for each position
       for i in range(0, position.shape[0]):
          position[i] = transform[i][transform.shape[1]-1]

        # orientation will have to be specific for now

    def getTransform(self):
       if self.position.shape[0] == 3:
          return create3DTransformFromOffsets(self.position[0], self.position[1], self.position[2], self.orientation[0], self.orientation[1], self.orientation[2])
       elif self.position.shape[0] == 2:
          return create2DTransformFromOffsets(self.position[0], self.position[1], self.orientation[0])

from math import radians
# test code
def main():
  tform = create2DTransformFromOffsets(radians(90),1,1)
  print(tform)
  test_pt = np.array([1,0])
  print(len(test_pt.shape))
  new_pt = transformPoint(test_pt, tform)
  print("original pt:", test_pt)
  print("transform:", tform)
  print("transformed pt:", new_pt)


if __name__ == '__main__':
    main()

