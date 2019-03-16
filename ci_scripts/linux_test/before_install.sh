# export CC=gcc-6
# export CXX=g++-6

git fetch --unshallow
git pull --tags
git describe

pip install --user cpp-coveralls

source /opt/qt*/bin/qt*-env.sh
