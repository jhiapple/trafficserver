#! /usr/bin/env bash

#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# check for python3
python3 - << _END_
import sys

if sys.version_info.major < 3 or sys.version_info.minor < 6:
    exit(1)
_END_

if [ $? = 1 ]; then
    echo "Python 3.6 or newer is not installed/enabled."
    exit 1
else
    echo "Python 3.6 or newer detected!"
fi

# check for pipenv
pipenv --version &> /dev/null
if [ $? -eq 0 ]; then
    echo "pipenv detected!"
    pipenv --venv &> /dev/null
    if [ $? -ne 0 ]; then
        echo "Installing a new virtual environment via pipenv"
        pipenv install
    else
        echo "Using the pre-existing virtual environment."
    fi
else
    echo "pipenv is not installed/enabled. "
    exit 1
fi
