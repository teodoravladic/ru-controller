#!/bin/bash

# libyang
cd $HOME
git clone https://github.com/CESNET/libyang.git
cd libyang
git checkout v2.1.55
mkdir build && cd build
cmake -DENABLE_TESTS=OFF \
      -DENABLE_VALGRIND_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/opt/mplane-v2 \
      -DCMAKE_INSTALL_RPATH=/opt/mplane-v2/lib \
      -DPLUGINS_DIR=/opt/mplane-v2/lib/libyang \
      -DPLUGINS_DIR_EXTENSIONS=/opt/mplane-v2/lib/libyang/extensions \
      -DPLUGINS_DIR_TYPES=/opt/mplane-v2/lib/libyang/types \
      -DYANG_MODULE_DIR=/opt/mplane-v2/share/yang/modules/libyang ..
make -j8
sudo make install
sudo ldconfig

#libnetconf
cd $HOME
git clone https://github.com/CESNET/libnetconf2.git
cd libnetconf2
git checkout v2.1.31
mkdir build && cd build
cmake -DENABLE_TESTS=OFF \
      -DENABLE_EXAMPLES=OFF \
      -DENABLE_VALGRIND_TESTS=OFF \
      -DCLIENT_SEARCH_DIR=/opt/mplane-v2/share/yang/modules \
      -DCMAKE_INSTALL_PREFIX=/opt/mplane-v2 \
      -DCMAKE_INSTALL_RPATH=/opt/mplane-v2/lib \
      -DLIBYANG_INCLUDE_DIR=/opt/mplane-v2/include \
      -DLIBYANG_LIBRARY=/opt/mplane-v2/lib/libyang.so \
      -DLY_VERSION_PATH=/opt/mplane-v2/include \
      -DYANG_MODULE_DIR=/opt/mplane-v2/share/yang/modules/libnetconf2 ..
make -j8
sudo make install
sudo ldconfig
