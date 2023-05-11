import itertools
import random

class Sudoku(object):

    """
    This source code for generating sudoku matrix is based on one from scripts of old framework which is based on matlab.
    Modified by Simon Kojima to adopt to Python version 3.x
    """

    def __init__(self):
        pass

    def _create_filter_for_vector(self, vector):
        return (lambda permutation: not self._has_any_same_elements_on_same_positions(permutation, vector))

    def _has_any_same_elements_on_same_positions(self, v1, v2):
        # TODO : impliment in more efficient way
        res = list()
        for idx, x in enumerate(list(v1)):
            res.append(x == v2[idx])
        return any(res)

    def generate_matrix(self, rows, columns):

        """
        This method creates "sudoku" matrix:
        1. Creates a list of all permutations of a vector
        2. Randomly selects an element.
        3. Filters a list of possible vectors to remove conflicting

        Note : should be rows <= columns
        """

        column_values = range(1, columns + 1)
        column_permutations = list(itertools.permutations(column_values))

        random.seed()
        
        matrix = [None] * rows
        for i in range(rows):
            column_permutation = random.choice(column_permutations)
            matrix[i] = list(column_permutation)
            column_permutations = list(filter(self._create_filter_for_vector(column_permutation), column_permutations))
        return matrix

def generate_stimulation_plan(n_stim_types = 5, itrs = 10):
    sudoku = Sudoku()
    mat = sudoku.generate_matrix(n_stim_types, n_stim_types)
    plan = mat[0]
    del mat[0]

    idx = -1
    for itr in range(1, itrs):
        is_finished = False
        while is_finished is False:
            idx += 1
            if idx >= len(mat):
                mat += sudoku.generate_matrix(n_stim_types, n_stim_types)
            is_finished = plan[-1] != mat[idx][0]
        plan += mat[idx]
        del mat[idx]
        idx = -1
        if len(mat) == 0:
            mat = sudoku.generate_matrix(n_stim_types, n_stim_types)
    return plan

def check_min_nstims_before_first_target(plan, target):
    for idx, m in enumerate(plan):
        if m == target:
            return idx 