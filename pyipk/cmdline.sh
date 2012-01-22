#!/bin/bash
env PYTHONPATH="${PYTHONPATH}:." python ./bin/ipkcmd --load 'plugins' $@

