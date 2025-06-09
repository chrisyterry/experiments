import numpy as np
from math import radians, degrees

# add common utils directory
import sys
import os
import pathlib
sys.path.insert(0, os.path.join(pathlib.Path(__file__).parents[1], 'common'))

import randomization as rando
import transforms as tf
import geometry as geom
from misc import Reference as Ref
import plotting as plt

# we might want the option to have the target be anywhere within a certain region on a bounding box
# we want to be able to recursively plan sections by specifying a bounding region, start and end goal
# maybe we see how close it can get to the target(s) then do a special correction to get them there if they didn't get close enough

# set target
# look at possible ways we could go
# generate new pose and link
#
# not quite sure if we need the orientation for the poses but it might be useful

# generates new nodes given target data and an existing node


class PoseGenerator:
   _constraints = {}  # constraints on node generation
   _weights = {"ang_dist": 1,  # weights for data in node generation
               "lin_dist": 1}
   _goal_tolerances = {"ang_dist": 1,  # tolerances on goal pose for snapping to the goal pose
                       "lin_dist": 0}
   __debug_out = False

   def __init__(self, constraints: dict, weights: dict = None, goal_tolerances: dict = None):
      self._constraints = constraints
      if weights is not None:
        self._weights = weights
      if goal_tolerances is not None:
        self._goal_tolerances = goal_tolerances

   def generate(self, node, target: tf.Pose):
    """
    Generate child nodes, return true if we have reached the target
    """

    # if a maximum depth has been specified
    if "max_depth" in self._constraints.keys():
      # generate a maximum depth for this node
      max_depth = (self._constraints["max_depth"].generate())[0]

      # if the node depth has maxed out
      if node._depth >= max_depth:
        return {}

    num_children = 1
    # if a number of child nodes have been specified
    if "children" in self._constraints.keys():
       # generate the number of children to generate for the node
       num_children = (self._constraints["children"].generate())[0]

    children = {}
    if self.__debug_out:
      padding=""
      for i in range(node._depth):
        padding += "\t"
      print(padding+"current node: position", node._data["pose"].position,"orientation",node._data["pose"].orientation)

    # generate coordinate changes for each child
    for child in range(0, int(num_children)):

       # if we want to change this to work on a grid, we just change the generators so that they generate children with knowledge of the grid data

       # generate, orientation changes
       roll = (self._constraints["roll"].generate())[0]
       pitch = (self._constraints["pitch"].generate())[0]
       yaw = (self._constraints["yaw"].generate())[0]

       # generate translation changes
       tx = (self._constraints["tx"].generate())[0]
       ty = (self._constraints["ty"].generate())[0]
       tz = (self._constraints["tz"].generate())[0]

       # create child pose using gneretaed offsets
       child_pose = tf.Pose(np.array((tx, ty, tz)), np.array((roll, pitch, yaw)))
       child_pose.transformPose(node._data["pose"].transform)
       if self.__debug_out:
        print(padding+"\ttx:",tx,"ty:",ty,"tz",tz,"roll",degrees(roll),"pitch",degrees(pitch),"yaw",degrees(yaw))
        print(padding+"\tchildnode: position", child_pose.position, "orientation", child_pose.orientation)

       # determine the distance of the child node to the target pose
       target_ang_dist = float(np.dot(child_pose.orientation, target.orientation))
       target_lin_dist = float(np.linalg.norm(target.position - child_pose.position, 2))
    
       dist_heuristic = 0
       # if we are within tolerance of the goal pose
       if target_lin_dist <= self._goal_tolerances["lin_dist"] and target_ang_dist <= self._goal_tolerances["ang_dist"]:
          # set the pose to the target
          child_pose = target
       else:
          # calculate the distance heuristic
          dist_heuristic = target_ang_dist * self._weights["ang_dist"] + target_lin_dist * self._weights["lin_dist"]

       # determine the distance of the child pose from the parent pose
       parent_ang_dist = float(np.dot(node._data["pose"].orientation, child_pose.orientation))
       parent_lin_dist = float(np.linalg.norm(child_pose.position - node._data["pose"].position, 2))
       # get the path length to the child node
       selection_weight = parent_ang_dist * self._weights["ang_dist"] + parent_lin_dist * self._weights["lin_dist"]

       # create the child node
       child_node = Node(data={"pose": child_pose}, generator=self, parent=node, depth=(node._depth + 1), weight=selection_weight)

       # if we have reached the target
       if dist_heuristic == 0:
         child_node._on_target_path = True

       # get the weight for deciding which node to select next
       weight = selection_weight + dist_heuristic

       # if somehow, we have child nodes with the same weight
       if weight in children.keys():
          # if we only have 1 prior node
          if not isinstance(children[weight], list):
            children[weight] = [children[weight], child_node]
          # if we have more than 1 prior node(!)
          else:
            children[weight].append(child_node)
       # if it's the first of its weight
       else:
          children[weight] = child_node

    return children

# defines a node for a node graph search
class Node:
  __debug_out = False
  _data = {}  # data associated with the node
  _parent = None  # the parent of the node
  _children = {}  # children of the current node (ordered by weight)
  _depth = 0  # how deep this node is in the graph (0 represents the origin node)
  _generator = None  # generator object used to generate
  _explored = False  # whether this node has been explored
  _weight = 0  # weight of the node
  _on_target_path = False # whether this node is on the path to the target

  def __init__(self, data: dict, generator, parent=None, depth=0, weight=0):
    self._data = data
    self._parent = parent
    self._generator = generator
    self._depth = depth
    self._weight = weight

  def expand(self, target):
    """
    expands the node, if the target is found amongst the children, returns that node as part of the target path
    """

    self._explored = True
    # generate child nodes
    self._children = self._generator.generate(self, target)

    # get a sorted list of weights
    sorted_weights = list(self._children.keys())
    sorted_weights.sort()

    # for each child node
    for weight in sorted_weights:
      # if there's a bunch of children with the same weight:
      if isinstance(self._children[weight], list):
        # for each child with the same weight
        for child in self._children[weight]:
          # if the child is on the target path without itself having to be expanded (it is the target)
          if child._on_target_path:
            # record that this node is on the target path and add the child as the first node on the path
            self._on_target_path = True
            return [child]
          
          # if the child is not the target
          else:
            # expand the child to see if it leads to the target
            target_path = child.expand(target)
            # if the child is on the path
            if target_path is not None:
              self._on_target_path = True
              # add the child to the path
              target_path.append(self._children[weight])
              return target_path

      # if there's just one child with this weight
      else:
        # if the child is on the target path without itself having to be expanded (it is the target)
        if self._children[weight]._on_target_path:
          self._on_target_path = True

        # if the child is the target
        else:
          # expand the node
          target_path = self._children[weight].expand(target)
          # if the child is on the target path
          if target_path is not None:
            self._on_target_path = True
            target_path.append(self._children[weight])
            return target_path

    return None

  def getRenderingPoints(self):
    plotting_points = self._data["pose"].position
    if self.__debug_out:
      padding = ""
      for i in range(self._depth):
        padding += "\t"
      print(padding+"current node position:",plotting_points)
      if len(self._children) > 0:
        print(padding+"these are my children:")
    # for each child
    for child in self._children:
      if isinstance(self._children[child], list):
        for sub_child in self._children[child]:
          # add the points for the child
          plotting_points = np.vstack((plotting_points, sub_child.getRenderingPoints()))
          # add the point for this node to make it a clean branch
          plotting_points = np.vstack((plotting_points, self._data["pose"].position))
      else:
        # add the points for the child
        plotting_points = np.vstack((plotting_points, self._children[child].getRenderingPoints()))
        # add the point for this node to make it a clean branch
        plotting_points = np.vstack((plotting_points, self._data["pose"].position))

    return plotting_points


class NodeGraph:
  __debug_out = False
  _nodes = None  # nodes associated with the nodegraph
  _generator = None  # generator used to generate new nodes

  def __init__(self, generator):
    self._generator = generator

  def generate(self, start:tf.Pose, target:tf.Pose):
    """
    generate a node graph to the specified target position(s)
    """
    # create the initial node
    self._nodes = {0: Node({"pose": start}, self._generator)}
    # for each node present, in order of weighting
    for node in self._nodes.keys():
      # expand the node
      target_path = self._nodes[node].expand(target)
      # if a path could be found
      if target_path is not None:
        target_path.append(self._nodes[node])
        return target_path

    return None # no path could be found

  def getRenderingPoints(self):
   """
   get points to render the node graph
   """
   points = self._nodes[0]._data["pose"].position
   if self.__debug_out:
    print("current node position:",points)
    print("these are my children:")
   for node in self._nodes:
      if isinstance(self._nodes[node], list):
        for i in range(len(self._nodes[node])):
          
          # add the points for the node
          points = np.vstack((points, self._nodes[node][i].getRenderingPoints()))

          # add the current node to make things neat and tidy
          points = np.vstack((points, self._nodes[0]._data["pose"].position))
      else:
        # add the points for the node
        points = np.vstack((points, self._nodes[node].getRenderingPoints()))

        # add the current node to make things neat and tidy
        points = np.vstack((points, self._nodes[0]._data["pose"].position))


   return points

def main():

  # set our max translation in ground-plane
  min_translation_gp = -2 # m
  max_translation_gp = 1 # m
  # set our max translation vertically
  min_translation_vt = -1 # m
  max_translation_vt = -1 # m

  # set our max angle change
  min_ang = radians(-90)
  max_ang = radians(90)
  ang_res = radians(90)

  x_generator = rando.UniformGenerator(1, 5,1, False)
  y_generator = rando.UniformGenerator(0, 0,1, False)
  z_generator = rando.UniformGenerator(0, 0,1, False)
  ang_generator = rando.UniformGenerator(min_ang, max_ang, ang_res, False)
  roll_generator = rando.ConstantGenerator(0)
  depth_generator = rando.ConstantGenerator(10)
  child_generator = rando.UniformGenerator(1, 2, 1)

  constraints = {"roll" : roll_generator,
                 "pitch" : roll_generator,
                 "yaw" : ang_generator,
                 "tx" : x_generator,
                 "ty" : y_generator,
                 "tz" : z_generator,
                 "max_depth" : depth_generator,
                 "children" : child_generator}

  generator = PoseGenerator(constraints)
  node_graph = NodeGraph(generator)

  # set our start point
  start = tf.Pose(np.array((0,0,0)),np.array((0,0,0)))

  # set our goal point
  goal = tf.Pose(np.array((30,0,0)),np.array((0,0,0)))

  # generate the node graph
  print("generating")
  node_graph.generate(start, goal)
  
  print("plotting")
  fig = plt.plotObjects([(node_graph,{})])
  fig.show()
  # something like:
  # generate a bunch of child nodes
  # check each child node to see if they're better based on our metrics
  # generate child nodes from the node that is better

  # we'll want to have a get points function for any node graphs we create that can render them out

if __name__ == '__main__':
    main()