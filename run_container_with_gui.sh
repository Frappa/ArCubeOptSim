# Build container and run it with VNC support
docker build -t arcube_vnc .
#Run the container, mounting current directory and exposing VNC port
docker run -v ./:/ArCubeOptSim -p 5901:5901 arcube_vnc

# To connect to the container GUI, use a VNC viewer to connect to localhost:5901

# ArCubeOptSim is not built automatically; you need to build it inside the container.
# If you have once built in inside the container, you can rerun the container and do not need to rebuild unless you delete the build files.

# To build ArCubeOptSim inside the container:
# cd  /ArCubeOptSim
# mkdir build
# cd build
# cmake ..
# make -j8