FROM smflment/geant4:10.5.1

LABEL maintainer="Livio Calivers <livio.calivers@unibe.ch>"

ENV DEBIAN_FRONTEND=noninteractive
ENV XDG_CONFIG_DIRS="/etc/xdg"

# System packages + VNC environment
RUN apt-get update && apt-get install -y --no-install-recommends \
    tzdata ca-certificates \
    xfce4 xfce4-terminal x11vnc xvfb xfonts-base dbus-x11 vim nano git cmake build-essential

# Set timezone
RUN ln -fs /usr/share/zoneinfo/UTC /etc/localtime && echo "UTC" > /etc/timezone

#############################################
# Clone and build ArCubeOptSim
#############################################
# RUN git clone https://github.com/Frappa/ArCubeOptSim.git && \
#     cd ArCubeOptSim && mkdir build && cd build && \
#     cmake .. && \
#     make -j4

# # Copy GDML for demo run
# RUN cp /Software/ArCubeOptSim/resources/gdml/module123.gdml \
#        /Software/ArCubeOptSim/resources/gdml/OptSim.gdml && \
#     cp /Software/ArCubeOptSim/run/OptSim_LUT_voxel_table_2x2.txt \
#        /Software/ArCubeOptSim/run/OptSim_LUT_voxel_table.txt

#############################################
# VNC startup script
#############################################
RUN echo '#!/bin/bash\n\
export DISPLAY=:99\n\
Xvfb :99 -screen 0 1920x1080x24 &\n\
sleep 1\n\
mkdir -p /var/run/dbus\n\
dbus-daemon --system --fork\n\
startxfce4 &\n\
sleep 2\n\
x11vnc -display :99 -forever -nopw -listen 0.0.0.0 -rfbport 5901\n\
' > /usr/local/bin/start_vnc && chmod +x /usr/local/bin/start_vnc

EXPOSE 5901

CMD ["/usr/local/bin/start_vnc"]
