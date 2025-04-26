import matplotlib as mpl
import cv2 as cv
import numpy as np
import yaml
import plotly
import torch
import torch.nn as nn

class MyNetwork(nn.Module):
    def __init__(self, input_size, hidden_size, output_size):
        super(MyNetwork, self).__init__()
        self.linear1 = nn.Linear(input_size, hidden_size)
        self.relu = nn.ReLU()
        self.linear2 = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        x = self.linear1(x)
        x = self.relu(x)
        x = self.linear2(x)
        return x

# Instantiate the network
input_size = 10
hidden_size = 20
output_size = 5
model = MyNetwork(input_size, hidden_size, output_size)

# Example input
input_data = torch.randn(32, input_size)  # Batch size 32
output = model(input_data)
print(output.shape)

# what are trying to do
# we're trying to develop an algorithm to generate fun dungeons for the game
# how are we going to do it?
# first I think I should start with generating a simple set of nodes almost A* style
# I define a start and end point and visualize in 3D
# I come up with something akin to A* and put constraints on minimmum section size, angle etc.
# I then have it generate some test paths to verify that it can actually make it to the target point
# I then turn the line into a simple mesh (like just a bunch of rectangles linked together to form a complete corridor)
# I then add multiple detsinations so the thing supports junctions
# then I maybe add zones or rooms? will haev to see how the corridor generation works out
# will definitely have to figure out how to stop it self intersecting
# should also probably define some general boundaries for the algorithm; I think having mesh/triangles and bounding cuboids defined would be the best bet


