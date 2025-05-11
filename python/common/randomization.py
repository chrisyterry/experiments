import numpy as np

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

    _min = 0 # minimum value
    _max = 0 # maximum value
    _max_min_dif = None # difference between maximum and minimum value
    _res = None # resolution of output
    def __init__(self, min:float, max:float, res=None):
        """
        generate a random value in the specified range to the specified resolution using the uniform distribution

        Args:
            min - the minimum value
            max - the maximum value
            res - resolution for generation
        """
        self._min = min
        self._max = max
        if res is not None:
            self._max_min_dif = (max - min) / res
            self._res = res
    
    # generate the specified number of samples
    def generate(self, n=1):
        min = np.full(n, self._min)
        
        # if there's no sepcified resolution
        if self._res is None:
            return np.random.uniform(self._min, self._max, n)
        
        # if a resolution has been specified
        else:
            values = np.random.uniform(0, 1, n)
            min = np.full(n, self._min)
            return min + np.round(values * self._max_min_dif) * self._res

class NormalGenerator:
    _mean = 0 # mean value
    _std = 0 # std deviation value
    _res = None # resolution (if specified)
    _res_inv = None # inverse of resolution (if specified)
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
    _alpha = 0 # the alpha parameter
    _beta = 0 # the beta parameter
    _max_min_dif = 0 # difference between maximum and minimum value divided by the resolution
    _min = 0 # the minimum value for the distribution
    _res = None # resolution (if specified)
    def __init__(self, alpha, beta, min, max, res = None):
        self._alpha = alpha
        self._beta = beta
        self._max_min_dif = max-min
        self._min = min

        if res is not None:
            self._res = res
            self._max_min_dif = self._max_min_dif/res

    def generate(self, n=1):
        # generate values from teh distribution
        values = np.random.beta(self._alpha, self._beta, n)

        if self._res is not None:
            values = np.round(values * self._max_min_dif) * self._res
        else:
            values = values * self._max_min_dif

        return np.full(n, self._min) + values
