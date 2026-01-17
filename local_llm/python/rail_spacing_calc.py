import os
import argparse

from matplotlib import pyplot as plt
from math import sin, cos, asin, acos

def main():
    parser = argparse.ArgumentParser(description="Calculate rail spacings for can rails")
    parser.add_argument('-rr', '--rail_radius', type=float, help="radius pf rail [m]")
    parser.add_argument('-cl', '--can_lip_radius', type=float, default=None, help="Radius of can lip [m], if no lip is specified lip is assumed equal to body radius")
    parser.add_argument('-rs', '--rail_separation', type=float, default=None, help="Separation of rail centers [m]")
    args = parser.parse_args()


def get_rail_center_underhang(rail_radius:float, can_radius:float, rail_separation:float):
    center_dist = rail_radius + can_radius
    

if __name__ == '__main__':
    main()