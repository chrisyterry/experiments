import os
import argparse

from matplotlib import pyplot as plt
from matplotlib import patches as patch
from math import sin, cos, asin, acos

"""
Dimensions:
    small can - 65mm dia (with lip), 60mm dia (without lip), 36mm height
    standard can - 74mm dia (with lip), 70mm dia (without lip), 111mm height
    large can (tall) - 85mm dia (with lip), 80mm (without lip), 145mm height
"""

def main():
    parser = argparse.ArgumentParser(description="Calculate rail spacings for can rails")
    parser.add_argument('-rr', '--rail_radius', type=float, help="radius pf rail [mm]")
    parser.add_argument('-cl', '--can_lip_radius', type=float, default=None, help="Radius of can lip [mm], if no lip is specified lip is assumed equal to body radius")
    parser.add_argument('-rs', '--rail_separation', type=float, default=None, help="Separation of rail centers [mm]")
    args = parser.parse_args()

    get_rail_center_underhang(args.rail_radius, args.can_lip_radius)


def get_rail_center_underhang(rail_radius:float, can_radius:float, rail_separation:float):
    
    # distance between centers
    center_dist = rail_radius + can_radius

    # angle between horizontal and rail-can centerline
    centerline_angle = acos((rail_separation / 2)/center_dist)

    # get the underhang from the rail center
    rail_center_underhang = can_radius - sin(centerline_angle) * center_dist

    axis = plt.gca()
    right_rail = patch.Circle((rail_separation/2, 0), rail_radius)
    left_rail = patch.Circle((rail_separation/2, 0), rail_radius)
    can = patch.Circle((0, sin(centerline_angle) * center_dist), can_radius)
    axis.plot(right_rail)

    plt.show()

if __name__ == '__main__':
    main()