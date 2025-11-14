FROM smflment/geant4:10.5.1

LABEL maintainer="Livio Calivers <livio.calivers@unibe.ch>"

ENV DEBIAN_FRONTEND=noninteractive
ENV XDG_CONFIG_DIRS="/etc/xdg"

# System packages + VNC environment
RUN apt-get update && apt-get install -y --no-install-recommends \
    tzdata ca-certificates \
    openbox x11-utils x11vnc xvfb xfonts-base dbus-x11 pm-utils vim nano git cmake build-essential \
    x11-xkb-utils xkb-data xauth locales
RUN apt-get install -y xterm

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
set -euo pipefail\n\
export DISPLAY=:99\n\
# start virtual X server (write logs for debugging)\n\
Xvfb :99 -screen 0 1920x1080x24 -nolisten tcp &> /var/log/xvfb.log &\n\
# wait for X server to be ready\n\
for i in $(seq 1 20); do\n\
    xdpyinfo -display :99 >/dev/null 2>&1 && break\n\
    sleep 0.5\n\
done\n\
sleep 0.5\n\
# Start a session dbus for the desktop session (ensures session-aware apps work)\n\
eval "$(dbus-launch --sh-syntax --exit-with-session)"\n\
export DBUS_SESSION_BUS_ADDRESS\n\
# set a default keyboard layout (helps avoid missing XKB mappings)\n\
if command -v setxkbmap >/dev/null 2>&1; then\n\
    DISPLAY=:99 setxkbmap us || true\n\
fi\n\
# start a lightweight window manager (openbox) and log its output\n\
openbox-session &> /var/log/openbox.log &\n\
sleep 0.5\n\
# Start VNC server attached to the virtual display with safe flags for keyboard handling\n\
x11vnc -display :99 -forever -nopw -shared -noxrecord -noxfixes -noxdamage -repeat -rfbport 5901 -o /var/log/x11vnc.log\n\
' > /usr/local/bin/start_vnc && chmod +x /usr/local/bin/start_vnc

EXPOSE 5901

CMD ["/usr/local/bin/start_vnc"]
