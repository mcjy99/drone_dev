cd voxl-docker-dev/ros2_humble/mavros_test/ros2_px4_ws/
source install/setup.bash
ros2 launch mavros fcu_url:=/dev/ttyACM0 gcs_url:=udp://10.

