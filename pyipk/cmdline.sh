#!/bin/bash
env PYTHONPATH="${PYTHONPATH}:." python cmdline/ipkcmd --load 'plugins' $@

