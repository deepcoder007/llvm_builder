#!/bin/bash

docker run \
    -v /home/$USER:/home/$USER \
    --user $(id -u):$(id -g)   \
    -it codegen_engine

