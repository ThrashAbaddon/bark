# Copyright (c) 2020 Julian Bernhard, Klemens Esterle, Patrick Hart and
# Tobias Kessler
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

import unittest
from bark.runtime.commons.parameters import ParameterServer
from bark.runtime.scenario.interaction_dataset_processing.dataset_decomposer import DatasetDecomposer
import os

class DatasetDecomposerTest(unittest.TestCase):
    def test_decompose_dataset(self):

        map_filename = "bark/runtime/tests/data/DR_DEU_Merging_MT_v01_shifted.xodr"
        track_filename = "bark/runtime/tests/data/interaction_dataset_dummy_track.csv"

        dataset_decomposer = DatasetDecomposer(
            map_filename=map_filename, track_filename=track_filename)

        dataset_decomposer.decompose()



if __name__ == '__main__':
    unittest.main()

