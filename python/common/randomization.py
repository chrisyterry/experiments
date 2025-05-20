import numpy as np
import math

#todo make this a nice class hierarchy with an interface


class ConstantGenerator:
    """
    Generator class that returns a constant, to make it easy to switch between randomized and fixed values
    """

    _const = 0 # constant value to return
    def __init__(self, constant):
        self._const = constant

    def generate(self, n = 1):
        return np.full(n, self._const)

class UniformGenerator:
    """
    Generates values in the specified range with the specified resolution
    """

    def __init__(self, min:float, max:float, res=None, duplicates=True):
        """
        generate a random value in the specified range to the specified resolution using the uniform distribution

        Args:
            min - the minimum value
            max - the maximum value
            res - resolution for generation
        """
        self._min = min
        self._max = max

        #if a resolution has been specified
        if res is not None:
            self._max_min_dif = (max - min) / res
            self._res = res
            # record info for duplicateless generation (only works if a res has been specified)
            self._duplicates = duplicates
            self._possible_values = min + (np.array(range(0, math.floor(max/res) + 1))) * res 
    
    # generate the specified number of samples
    def generate(self, n=1):
        min = np.full(n, self._min)
        
        # if there's no sepcified resolution
        if self._res is None:
            return np.random.uniform(self._min, self._max, n)
        
        # if a resolution has been specified
        else:
            # if duplicates are permitted
            if self._duplicates:
                # generate N samples in the 0-1 range and scale accordingly
                values = np.random.uniform(0, 1, n)
                min = np.full(n, self._min)
                return min + np.round(values * self._max_min_dif) * self._res
            
            # if duplicates are not permitted
            else:
                output = np.zeros(n)
                output_entry = 0
                available_values = self._possible_values 
                max_index = len(available_values) - 1
                for i in range(n):
                    # generate an index using the distribution
                    selection_index = round(np.random.uniform(0, 1) * max_index)

                    # add the value corresponding to the index
                    output[output_entry] = available_values[selection_index]

                    # move the value to the end of the list
                    available_values[selection_index], available_values[max_index] = available_values[max_index], available_values[selection_index]

                    # reduce the number of remaining values and bump the entry 
                    if max_index == 0:
                        max_index = len(available_values) - 1
                    else:
                        max_index += -1
                    output_entry += 1

                return output



class NormalGenerator:
    def __init__(self, mean, std, res = None):
        self._mean = mean
        self._std = std
        if res is not None:
            self._res = res
            self._res_inv = 1 / res

        # ToDo, should we offer the option for a truncated normal distribution?

    def generate(self, n=1):
        """
        generate random values from the configured normal distribution for the specified resolution
        """
        
        if self._res is None:
            return np.random.normal(self._mean, self._std, n)
        else:
            values = np.random.normal(0, self._std, n)
            values = np.full(n, self._mean) + np.round(values * self._res_inv) * self._res

            return values

class BetaGenerator:
    def __init__(self, alpha, beta, min, max, res = None, duplicates=True):
        self._alpha = alpha
        self._beta = beta
        self._max_min_dif = max-min
        self._min = min

        # if a resolution has been specified
        if res is not None:
            self._res = res
            self._max_min_dif = self._max_min_dif/res
            # record info for duplicateless generation (only works if a res has been specified)
            self._duplicates = duplicates
            self._possible_values = min + (np.array(range(0, math.floor(max/res) + 1))) * res 

    def generate(self, n=1):
        # generate values from the distribution
        values = np.zeros(n) 

        if self._res is not None:
            if self._duplicates:
                samples = np.random.beta(self._alpha, self._beta, n)
                values = self._min + np.round(samples * self._max_min_dif) * self._res             
            else:
                output_entry = 0
                available_values = self._possible_values 
                max_index = len(available_values) - 1
                for i in range(n):
                    # generate an index using the distribution
                    selection_index = round(np.random.beta(self._alpha, self._beta) * max_index)

                    # add the value corresponding to the index
                    values[output_entry] = available_values[selection_index]

                    # move the value to the end of the list
                    available_values[selection_index], available_values[max_index] = available_values[max_index], available_values[selection_index]

                    # reduce the number of remaining values and bump the entry 
                    if max_index == 0:
                        max_index = len(available_values) - 1
                    else:
                        max_index += -1
                    output_entry += 1
                
        else:
            values = self._min + np.random.beta(self._alpha, self._beta, n) * self._max_min_dif

        return values
