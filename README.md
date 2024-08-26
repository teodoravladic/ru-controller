# Build V1
```bash
mkdir && cd build
cmake -DMPLANE_PATH=<path-to>/mplane-v1 ..
make
```

# Build V2
```bash
mkdir && cd build
cmake -DMPLANE_PATH=<path-to>/mplane-v2 -DMPLANE_VERSION=V2 ..
make
```

# Run the client
```bash
./my_client
cat delay.xml
```
