# O-RAN M-plane implementation

# Prerequisites
In order to use this M-plane project, libyang and libnetconf2 libraries should exist.

You can use the provided scripts to compile these libraries depending on the targeted version:
```bash
./build_deps_v1.sh
./build_deps_v2.sh
```

The libraries are built in `/opt/mplane-v1` or `/opt/mplane-v2` path.

# 0. Modify path for predefined CU-plane configuration xml file
```bash
vi config-mplane.c #line 45
```

# 1. Build V1
```bash
mkdir build && cd build
cmake -DMPLANE_PATH=/opt/mplane-v1 -DMPLANE_VERSION=V1 ..
make
```

# 2. Build V2
```bash
mkdir build && cd build
cmake -DMPLANE_PATH=/opt/mplane-v2 -DMPLANE_VERSION=V2 ..
make
```

# 3. Run the client
```bash
./my_client <ru-ip-address> ...
```

If no `<ru-ip-address>` specified, `nc_listen_ssh()` command is used instead of `nc_connect_ssh()`.

RU-controller takes the following steps:
1. Connect to the RUs
2. Retreive operational datastore via <get> RPC
3. Load the RU supported yang models
4. Extract RU delay profile
5. Check the RU PTP status. If not synced, subscription to synchronization-state-change
6. Configure CU-plane via <edit-config> RPC
7. Validate the candidate datastore via <validate> RPC
8. Commit the candidate datastore via <commit> RPC
9. Disconnect from the RUs

At this stage, RUs are well configured and ready for testing.
