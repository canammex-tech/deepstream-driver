################################################################################
# The MIT License
#
# Copyright (c) 2024, Prominence AI, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
################################################################################

################################################################################
#
# This simple example demonstrates how to create a set of Pipeline components, 
# specifically:
#   - A File Source
#   - Primary GST Inference Engine (PGIE)
#   - IOU Tracker
#   - On-Screen Display
#   - Window Sink
#   - V4L2 Sink
# ...and how to add them to a new Pipeline and play.
#
# The V4L2 Sink is used to display video to v4l2 video devices. 
# 
# V4L2 Loopback can be used to create "virtual video devices". Normal (v4l2) 
# applications will read these devices as if they were ordinary video devices.
# See: https://github.com/umlaeute/v4l2loopback for more information.
#
# You can install v4l2loopback with
#    $ sudo apt-get install v4l2loopback-dkms
#
# Run the following to setup '/dev/video3'
#    $ sudo modprobe v4l2loopback video_nr=3
#
# When the script is running, you can use the following GStreamer launch 
# command to test the loopback
#    $ gst-launch-1.0 v4l2src device=/dev/video3 ! videoconvert  ! xvimagesink
#
# The example registers handler callback functions for:
#   - key-release events
#   - delete-window events
#   - end-of-stream EOS events
#   - Pipeline change-of-state events
#  
################################################################################

#!/usr/bin/env python

import sys
import time
from dsl import *

file_path = "/opt/nvidia/deepstream/deepstream/samples/streams/sample_1080p_h265.mp4"

# Filespecs (Jetson and dGPU) for the Primary GIE
primary_infer_config_file = \
    '/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_infer_primary.txt'
primary_model_engine_file = \
    '/opt/nvidia/deepstream/deepstream/samples/models/Primary_Detector/resnet18_trafficcamnet.etlt_b8_gpu0_int8.engine'

# Filespec for the IOU Tracker config file
iou_tracker_config_file = \
    '/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_IOU.yml'

# V4L2 Sink device location to stream to
DEVICE_LOCATION = '/dev/video3'

## 
# Function to be called on XWindow KeyRelease event
## 
def xwindow_key_event_handler(key_string, client_data):
    print('key released = ', key_string)
    if key_string.upper() == 'P':
        dsl_pipeline_pause('pipeline')
    elif key_string.upper() == 'R':
        dsl_pipeline_play('pipeline')
    elif key_string.upper() == 'Q' or key_string == '' or key_string == '':
        dsl_pipeline_stop('pipeline')
        dsl_main_loop_quit()
 
## 
# Function to be called on XWindow Delete event
## 
def xwindow_delete_event_handler(client_data):
    print('delete window event')
    dsl_pipeline_stop('pipeline')
    dsl_main_loop_quit()

## 
# Function to be called on End-of-Stream (EOS) event
## 
def eos_event_listener(client_data):
    print('Pipeline EOS event')
    dsl_pipeline_stop('pipeline')
    dsl_main_loop_quit()

## 
# Function to be called on every change of Pipeline state
## 
def state_change_listener(old_state, new_state, client_data):
    print('previous state = ', old_state, ', new state = ', new_state)
    if new_state == DSL_STATE_PLAYING:
        dsl_pipeline_dump_to_dot('pipeline', "state-playing")

def main(args):

    # Since we're not using args, we can Let DSL initialize GST on first call
    while True:

        # New File Source using the file path specified above, repeat enabled.
        retval = dsl_source_file_new('file-source', file_path, True)
        if retval != DSL_RETURN_SUCCESS:
            break

        # New Primary GIE using the filespecs above with interval = 0
        retval = dsl_infer_gie_primary_new('primary-gie', 
            primary_infer_config_file, primary_model_engine_file, 0)
        if retval != DSL_RETURN_SUCCESS:
            break

        # New IOU Tracker, setting operational width and height.
        retval = dsl_tracker_new('iou-tracker', iou_tracker_config_file, 480, 272)
        if retval != DSL_RETURN_SUCCESS:
            break

        # New OSD with text, clock and bbox display all enabled. 
        retval = dsl_osd_new('on-screen-display', 
            text_enabled=True, clock_enabled=True, 
            bbox_enabled=True, mask_enabled=False)
        if retval != DSL_RETURN_SUCCESS:
            break

        # New EGL Window Sink with offests and dimensions
        retval = dsl_sink_window_egl_new('window-sink', 100, 100, 1280, 720)
        if retval != DSL_RETURN_SUCCESS:
            break
        # Add the XWindow event handler functions defined above
        retval = dsl_sink_window_key_event_handler_add('window-sink', 
            xwindow_key_event_handler, None)
        if retval != DSL_RETURN_SUCCESS:
            break
        retval = dsl_sink_window_delete_event_handler_add('window-sink', 
            xwindow_delete_event_handler, None)
        if retval != DSL_RETURN_SUCCESS:
            break

        # New V4L2 Sink 
        retval = dsl_sink_v4l2_new('v4l2-sink', 
            device_location=DEVICE_LOCATION)
        if retval != DSL_RETURN_SUCCESS:
            break

        # Add all the components to our pipeline
        retval = dsl_pipeline_new_component_add_many('pipeline', 
            ['file-source', 'primary-gie', 'iou-tracker', 'on-screen-display', 
            'window-sink', 'v4l2-sink', None])
        if retval != DSL_RETURN_SUCCESS:
            break

        ## Add the listener callback functions defined above
        retval = dsl_pipeline_state_change_listener_add('pipeline', 
            state_change_listener, None)
        if retval != DSL_RETURN_SUCCESS:
            break
        retval = dsl_pipeline_eos_listener_add('pipeline', 
            eos_event_listener, None)
        if retval != DSL_RETURN_SUCCESS:
            break

        # Play the pipeline
        retval = dsl_pipeline_play('pipeline')
        dsl_pipeline_dump_to_dot('pipeline', "state-playing")
        if retval != DSL_RETURN_SUCCESS:
            break

        dsl_main_loop_run()
        retval = DSL_RETURN_SUCCESS
        break

    # Print out the final result
    print(dsl_return_value_to_string(retval))

    dsl_delete_all()

if __name__ == '__main__':
    sys.exit(main(sys.argv))

