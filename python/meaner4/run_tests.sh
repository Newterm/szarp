#!/bin/bash

# ugly way of calling python tests, it should be integrated somehow with python pybuild tests launched by dh_auto_test

env PYTHONPATH="${PYTHONPATH}:.:../protobuf" python -m unittest discover -s ./unittests/
