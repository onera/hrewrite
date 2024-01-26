#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ ! -d "${SCRIPT_DIR}/build" ]; then
  mkdir -p "${SCRIPT_DIR}/build"
  cd "${SCRIPT_DIR}/build"
  cmake -DUSE_DEFAULT_FLAGS=ON -DENABLE_PYTHON=ON -DCMAKE_INSTALL_PREFIX="${SCRIPT_DIR}/dist" ..
  cd ..
fi

cd "${SCRIPT_DIR}/build"
make -j
make install
cd ..

PYTHON_EXE=$(which python)
if [ "${PYTHON_EXE}" = "" ]; then
  PYTHON_EXE=$(which python3)
fi

if [ "${PYTHON_EXE}" = "" ]; then
  echo "ERROR: python not found"
  exit -1
fi

PYTHON_MAJOR=$(python3 -c "import sys; print(sys.version_info.major)")
PYTHON_MINOR=$(python3 -c "import sys; print(sys.version_info.minor)")

export PYTHONPATH="${SCRIPT_DIR}/dist/lib/python${PYTHON_MAJOR}.${PYTHON_MINOR}/site-packages/:$PYTHONPATH"

