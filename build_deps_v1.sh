#!/bin/bash

# libyang
cd $HOME
git clone https://github.com/CESNET/libyang.git
cd libyang
git checkout v1.0.240
mkdir build && cd build
cmake -DENABLE_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/opt/mplane-v1 \
      -DCMAKE_INSTALL_RPATH=/opt/mplane-v1/lib \
      -DPLUGINS_DIR=/opt/mplane-v1/lib/libyang1 \
      -DYANG_MODULE_DIR=/opt/mplane-v1/share/yang/modules/libyang ..
make -j8
sudo make install
sudo ldconfig

#libnetconf
cd $HOME
git clone https://github.com/CESNET/libnetconf2.git
cd libnetconf2
git checkout v1.1.46
mkdir build && cd build
cmake -DENABLE_BUILD_TESTS=OFF \
      -DENABLE_VALGRIND_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/opt/mplane-v1 \
      -DCMAKE_INSTALL_RPATH=/opt/mplane-v1/lib \
      -DLIBYANG_INCLUDE_DIR=/opt/mplane-v1/include \
      -DLIBYANG_LIBRARY=/opt/mplane-v1/lib/libyang.so \
      -DLY_HEADER_PATH=/opt/mplane-v1/include \
      -DYANG_MODULE_DIR=/opt/mplane-v1/share/yang/modules/libyang ..
make -j8
sudo make install
sudo ldconfig
