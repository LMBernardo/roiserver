#!/bin/bash
git submodule update --init
git submodule foreach --recursive git submodule update --init
git submodule foreach --recursive git submodule update --init
