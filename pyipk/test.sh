#!/bin/bash
echo '-------------------------- Running python 2 --------------------------'
env PYTHONPATH="${PYTHONPATH}:." python -m unittest discover
echo '-------------------------- Running python 3 --------------------------'
#env PYTHONPATH="${PYTHONPATH}:." python3 -m unittest discover
