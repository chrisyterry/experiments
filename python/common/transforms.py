from math import radians
import numpy as np
import copy
from math import cos, sin, comb, asin, acos, atan2

# row-major transforms can be combined by doing T1xT2xT3

"""
create a 3D row-major transform from the specified roll, pitch and yaw along with translations on the x, y and z axes
"""
def create3DTransformFromOffsets(roll: float, pitch: float, yaw: float, tx: float, ty: float, tz: float):
  # pre-calculate sines and cosines for each angle
  p_cos = cos(pitch)
  p_sin = sin(pitch)
  r_cos = cos(roll)
  r_sin = sin(roll)
  y_cos = cos(yaw)
  y_sin = sin(yaw)

  # setup transform (result of yaw_transform * pitch_transform * roll_transform)
  transform = np.identity(4)
  transform[0, 0] = y_cos*p_cos
  transform[0, 1] = (y_cos*p_sin*r_sin)-(y_sin*r_cos)
  transform[0, 2] = (y_cos*p_sin*r_cos)+(y_sin*r_sin)
  transform[1, 0] = y_sin*p_cos
  transform[1, 1] = (y_sin*p_sin*r_sin)+(y_cos*r_cos)
  transform[1, 2] = (y_sin*p_sin*r_cos)-(y_cos*r_sin)
  transform[2, 0] = -p_sin
  transform[2, 1] = p_cos*r_sin
  transform[2, 2] = p_cos*r_cos
  transform[3, 0] = tx
  transform[3, 1] = ty
  transform[3, 2] = tz

  return transform

"""
create a 2D row-major transform from the specified yaw along with translations on the x and y
"""
def create2DTransformFromOffsets(yaw: float, tx: float, ty: float):
  # pre-calculate yaw sine and cosine
  y_cos = cos(yaw)
  y_sin = sin(yaw)

  # setup transform (yaw_transform)
  transform = np.identity(3)
  transform[0, 0] = y_cos
  transform[0, 1] = -y_sin
  transform[0, 2] = tx
  transform[1, 0] = y_sin
  transform[1, 1] = y_cos
  transform[1, 2] = ty
  return transform

"""
create a 3D transformation from a 2D transformation
"""
def transform2DTo3D(transform: np.array):
    assert transform.shape[0] == transform.shape[1]
    assert transform.shape[0] == 2
    return np.block([[transform, np.zeros((2, 1))], [np.zeros(1, 2), 1]])


"""
create a 2D transformation from a 3D transformation
"""
def transform3DTo2D(transform: np.array):
    assert transform.shape[0] == transform.shape[1]
    assert transform.shape[0] == 3
    return transform[:2, :2]


"""
efficently(?) transform a vector using a homogenous transform
"""
def transformVector(point: np.array, transform: np.array, applyOffset=True):
    
    if transform is None:
       return point

    assert transform.shape[0] == transform.shape[1]
    assert len(point.shape) == 1 and point.shape[0] == transform.shape[1] - 1

    transformed_point = np.zeros(point.shape)
    match point.shape[0]:
        # avoid useless 0x calculations
        case 2:
            transformed_point[0] = point[0]*transform[0, 0] + point[1]*transform[1, 0]
            transformed_point[1] = point[1]*transform[0, 1] + point[1]*transform[1, 1]
            # if we want to apply the offset (i.e. it's a point rather than a direction that were are transforming)
            if applyOffset:
               transformed_point[0] += transform[2, 0]
               transformed_point[1] += transform[2, 1]
        case 3:
            transformed_point[0] = (point[0]*transform[0, 0]) + point[1] * transform[1, 0] + point[2]*transform[2, 0]
            transformed_point[1] = (point[0]*transform[0, 1]) + point[1] * transform[1, 1] + point[2]*transform[2, 1]
            transformed_point[2] = (point[0]*transform[0, 2]) + point[1] * transform[1, 2] + point[2]*transform[2, 2]
            # if we want to apply the offset (i.e. it's a point rather than a direction that were are transforming)
            if applyOffset:
               transformed_point[0] += transform[3, 0]
               transformed_point[1] += transform[3, 1]
               transformed_point[2] += transform[3, 2]
        case _:
          raise ("point size of " + point.shape[1] + " is not supported!")

    return transformed_point

# combine the specified transforms into a single transform
def combineTransforms(first_transform:np.array, second_transform:np.array):
   return np.matmul(second_transform, first_transform)

class Pose:
    position = np.zeros(3)  # likely (but not strictly) meters
    orientation = np.zeros(3) # unit vector capturing orienation of pose
    transform = None # transform that is equivalent to this pose

    # constructor from pose from position and RPY orientations
    def __init__(self, position: np = np.zeros(3), orientation: np.array = np.zeros(3)):
      self.setFromOrientationAndTranslation(position, orientation)

    # to string method for printing easily
    def __str__(self):
      return ('Position: ' + str(self.position) + '\nOrientation:' + str(self.orientation))

    # set the position and orientation directly
    def setFromOrientationAndTranslation(self, position:np.array = np.zeros(3), orientation:np.array = np.zeros(3)):
       assert len(position.shape) == 1 and len(orientation.shape) == 1
       # orienations are related to pairs of axes)
       assert orientation.shape[0] == comb(position.shape[0], 2)

       # get the transform corresponding to this pose
       transform=None
       match position.shape[0]:
        case 2:
          transform=create2DTransformFromOffsets(orientation[0], position[0], position[1])
        case 3:
          transform=create3DTransformFromOffsets(orientation[0], orientation[1], orientation[2], position[0], position[1], position[2])
        case _:
          raise (position.shape[0] + " dimensional poses are not supported!")
       
       base_orientation = np.zeros(self.position.shape[0])
       base_orientation[0] = 1

       self.position = position
       self.transform = transform
       self.orientation = transformVector(base_orientation, self.transform, False)

    # set the position and orientation from a row-major transform
    def setFromTransform(self, transform: np.array):
       assert len(transform.shape) == 2 and transform.shape[0] == transform.shape[1]

       # size the position and orientation
       self.position = np.zeros(transform.shape[0]-1)
       self.orientation = np.zeros(transform.shape[0]-1)
       self.orientation[0] = 1

       # set the position and orientation
       self.position = transform[transform.shape[0]-1,0:transform.shape[0]-1]
       self.orientation = transformVector(self.orientation, transform, False)

       # record the transform
       self.transform = transform
    
    # transform the pose using the specified transformation matrix
    def transformPose(self, transform:np.array):
      assert self.position.shape[0] == (transform.shape[0] - 1)

      # get the new position of the pose
      self.position = transformVector(self.position, transform)
      self.orientation = transformVector(self.orientation, transform, False)
      self.transform = np.matmul(self.transform, transform)

    # create a new pose that is this pose with the specified transform applied
    def transformedPose(self, transform:np.array):
       assert self.position.shape[0] == (transform.shape[0] - 1)
       new_pose = copy.deepcopy(self)
       print(new_pose)
       new_pose.transformPose(transform)

       return new_pose
       
    # get the angles for the pose
    def getAngles(self):
      dimension = 0
      angles = []
      for i in range(1, self.orientation.shape[0]):
         for j in range(0,i):
            angles[dimension] = atan2(self.orientation[i], self.orientation[j])
            dimension += 1

      return angles 


# test code


def main():
  tform = create2DTransformFromOffsets(radians(90), 1, 1)
  print(tform)
  test_pt = np.array([1, 0])
  print(len(test_pt.shape))
  new_pt = transformVector(test_pt, tform)
  print("original pt:", test_pt)
  print("transform:", tform)
  print("transformed pt:", new_pt)

  tform = create3DTransformFromOffsets(radians(0), radians(0), radians(90), 0, 0, 0)
  new_pt = np.array([1, 1, 1])
  print("original pt:", new_pt)
  print("transform:", tform)
  new_pt = transformVector(new_pt, tform)
  print("transformed pt:", new_pt)

  x = [new_pt]


if __name__ == '__main__':
    main()
