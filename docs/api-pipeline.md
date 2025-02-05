 # Pipeline API Reference
Pipelines are the top level component in DSL. They manage and synchronize Child components when transitioning to states of `READY`, `PAUSED`, and `PLAYING`. There is no practical limit to the number of Pipelines that can be created, only the number of Sources, Secondary GIE's and Sinks that are in-use by one or more Pipelines at any one time; numbers that are constrained by the Jetson/dGPU hardware in use. 

## Pipeline Streammuxer
All DSL Pipelines are created with a built-in **Streammuxer**  providing the following critical services:
* To enable multiple sources to be added to every Pipeline before and while playing, with the frame-buffers from each batched together for efficient processing downstream.
* To create and add the basic batch level metadata structure [`NvDsBatchMeta`](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_metadata.html#metadata-in-the-deepstream-sdk) to each batched buffer required for downstream preprocessing, inference, tracking, on-screen-display, etc.

DSL supports both the [**OLD** NVIDIA Streammux pluging](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux.html) and the [**NEW** NVIDIA Streammux plugin](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux2.html) 

---

### OLD Streammuxer
The old NVIDIA Streammuxer is used by default, however, it will be removed in a future release of DeepStream (presumably, once the NEW Streammuxer is working correctly).

#### Streammuxer Batch-Settings (OLD)
The two Streammuxer batch properties are defined as:
* `batch-size` -- _the maximum number of frames in a batch_ -- which is set to the number of Source components that have been added to the Pipeline when it transitions to a state of PLAYING, unless explicitly set by the client.
* `batch-timeout` -- _the timeout in microseconds to wait after the first buffer is available to push the batch even if a complete batch is not formed._ -- which has a default value of `-1` disabling the timeout.

Both the `batch-size` and `batch-timeout` settings can be updated while the Pipeline is stopped (in a state of NULL) by calling [dsl_pipeline_streammux_batch_properties_set](#dsl_pipeline_streammux_batch_properties_set). The current values can be obtained at anytime by calling [dsl_pipeline_streammux_batch_properties_get](#dsl_pipeline_streammux_batch_properties_get).

**IMPORTANT!** 
If dynamically adding/removing sources at runtime (i.e. while the Pipeline is playing), the `batch-size` should be explicitly set to the maximum number of Sources that can be added.

#### Input Dimensions (OLD)
The Streammuxer scales all input stream to a single resolution (i.e. all frames in the batch have the same resolution). This resolution can be specified using the `width` and `height` properties (i.e. dimensions). The Streammuxer's dimensions, initialized by the plugin to 0, are set by the Pipeline to `DSL_STREAMMUX_DEFAULT_WIDTH` and `DSL_STREAMMUX_DEFAULT_HEIGHT` as defined by the [Pipeline Streammuxer Constant Values](#pipeline-streammuxer-constant-values). The output dimensions can be updated by calling [`dsl_pipeline_streammux_dimensions_set`](#dsl_pipeline_streammux_dimensions_set) while the Pipeline is stopped. The current dimensions can be obtained by calling [`dsl_pipeline_streammux_dimensions_get`](#dsl_pipeline_streammux_dimensions_get)

The enable-padding property can be set to true to preserve the input aspect ratio while scaling by padding with black bands. See * [`dsl_pipeline_streammux_padding_get`](#dsl_pipeline_streammux_padding_get) and [`dsl_pipeline_streammux_padding_set`](#dsl_pipeline_streammux_padding_set).

---

### NEW Streammuxer
**IMPORTANT!** There is at least one critical bug in the NEW NVIDIA Streammuxer (DS 6.3 and 6.4) that prevents DSL from working correctly, especially if using a Stream Demuxer. See: [Pipelines with new nvstreammux and nvstreamdemux fail to play correctly in DS 6.3](https://forums.developer.nvidia.com/t/pipelines-with-new-nvstreammux-and-nvstreamdemux-fail-to-play-correctly-in-ds-6-3/278396).

The NEW NVIDIA Streammuxer requires the following environment variable to be set
```bash
export USE_NEW_NVSTREAMMUX=yes
```

#### Streammuxer Batch-Size (NEW)
The Streammuxer `batch-size` property -- defined as  _the **maximum** number of frames in a batch_ -- is set to the number of Source components that have been added to the Pipeline when it transitions to a state of PLAYING, unless explicitly set. 

The batch size is set by calling [dsl_pipeline_streammux_batch_size_set](#dsl_pipeline_streammux_batch_size_set). The current value can be obtained at anytime by calling [dsl_pipeline_streammux_batch_size_get](#dsl_pipeline_streammux_batch_size_get).

**IMPORTANT!** 
1. DSL implements its own _adaptive-batching_ which overrides the configuration properties; `adaptive-batching` and `batch-size`.
2. If dynamically adding/removing sources at runtime (i.e. while the Pipeline is playing), the `batch-size` should be explicitly set to the maximum number of Sources that can be added.
  
#### Streammuxer Configuration (NEW)
A [Streammuxer Configuration File](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux2.html#mux-config-properties) is provided to the Pipeline by calling [`dsl_pipeline_streammux_config_file_set`](#dsl_pipeline_streammux_config_file_set). The current config-file in use can be queried by calling [`dsl_pipeline_streammux_config_file_get`](#dsl_pipeline_streammux_config_file_get). 

Please review the [_NvStreamMux Tuning Solutions for specific use cases_](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux2.html#nvstreammux-tuning-solutions-for-specific-use-cases) for more information.

--- 
### Common Streammuxer Default Settings
The Pipeline's Streammuxer is created with all default GST properties except for the `batch-size`.  

Below are the Streammux plugin default property values as logged by the Pipeline Sources-bin on creation,. (`$ export GST_DEBUG=1,DSL:4`). The common default settings are the same for both the OLD and NEW Streammuxer.
```
PipelineSourcesBintr:  : Initial property values for Streammux 'pipeline-sources-bin'
PipelineSourcesBintr:  :   num-surfaces-per-frame : 1
PipelineSourcesBintr:  :   attach-sys-ts          : 1
PipelineSourcesBintr:  :   sync-inputs            : 0
PipelineSourcesBintr:  :   max-latency            : 0
PipelineSourcesBintr:  :   frame-duration         : -1
PipelineSourcesBintr:  :   drop-pipeline-eos      : 0
``` 

### Streammuxer NTP Timestamp Support

From the [DeepStream Streammuxer documentation](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux2.html#gst-nvstreammux-new), _"The muxer supports calculation of NTP timestamps for source frames. It supports two modes. In the system timestamp mode, the muxer attaches the current system time as NTP timestamp. In the RTCP timestamp mode, the muxer uses RTCP Sender Report to calculate NTP timestamp of the frame when the frame was generated at source. The NTP timestamp is set in ntp_timestamp field of NvDsFrameMeta."_

**Note:** See the [NTP Timestamp in DeepStream](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_NTP_Timestamp.html#ntp-timestamp-in-deepstream) section under the DeepStream Development Guide for more information.

The _system timestamp mode_ can be enabled/disabled by calling [dsl_pipeline_streammux_attach_sys_ts_enabled_set](#dsl_pipeline_streammux_attach_sys_ts_enabled_set). The default value is

## Pipeline Construction and Destruction
Pipelines are constructed by calling [`dsl_pipeline_new`](#dsl_pipeline_new) or [`dsl_pipeline_new_many`](#dsl_pipeline_new_many).

Pipelines are destructed by calling [`dsl_pipeline_delete`](#dsl_pipeline_delete), [`dsl_pipeline_delete_many`](#dsl_pipeline_delete_many), or [`dsl_pipeline_delete_all`](#dsl_pipeline_delete_all). Deleting a pipeline will not delete its child component, but will unlink them and return to a state of `not-in-use`. The client application is responsible for deleting all child components by calling [`dsl_component_delete`](/docs/api-component.md#dsl_component_delete), [`dsl_component_delete_many`](/docs/api-component.md#dsl_component_delete_many), or [`dsl_component_delete_all`](/docs/api-component.md#dsl_component_delete_all).

## Adding and Removing Components
Child components -- Sources, Inference Engines, Trackers, Tiled-Displays, On Screen-Display, and Sinks -- are added to a Pipeline by calling [`dsl_pipeline_component_add`](#dsl_pipeline_component_add) and [`dsl_pipeline_component_add_many`](#dsl_pipeline_component_add_many). A Pipeline's current number of child components can be obtained by calling [`dsl_pipeline_component_list_size`](#dsl_pipeline_component_list_size)

Child components can be removed from their Parent Pipeline by calling [`dsl_pipeline_component_remove`](#dsl_pipeline_component_remove), [`dsl_pipeline_component_remove_many`](#dsl_pipeline_component_remove_many), and [`dsl_pipeline_component_remove_all`](#dsl_pipeline_component_remove_all)

## Methods Of Component Linking
Components added to a Pipeline can be linked using one of [two methods](/docs/overview.md#linking-components). Click the link for an overview. The method of linking can be queried by calling [`dsl_pipeline_link_method_get`](#dsl_pipeline_link_method_get) and set anytime by calling [`dsl_pipeline_link_method_set`](#dsl_pipeline_link_method_set) as long as the Pipeline is not linked and playing.

## Playing, Pausing and Stopping a Pipeline

Pipelines - with a minimum required set of components - can be **played** by calling [`dsl_pipeline_play`](#dsl_pipeline_play), **paused** by calling [`dsl_pipeline_pause`](#dsl_pipeline_pause) and **stopped** by calling [`dsl_pipeline_stop`](#dsl_pipeline_stop).

## Pipeline Client Callback Functions
Clients can be notified of Pipeline events by registering/deregistering one or more callback functions with the following services.
* _Change of State_ - with [`dsl_pipeline_state_change_listener_add`](#dsl_pipeline_state_change_listener_add) / [`dsl_pipeline_state_change_listener_remove`](#dsl_pipeline_state_change_listener_remove).
* _End of Stream (EOS)_ - with [`dsl_pipeline_eos_listener_add`](#dsl_pipeline_eos_listener_add) / [`dsl_pipeline_eos_listener_remove`](#dsl_pipeline_eos_listener_remove).
* _Error Message Received_ - with [`dsl_pipeline_error_message_handler_add`](#dsl_pipeline_error_message_handler_add) / [`dsl_pipeline_error_message_handler_remove`](#dsl_pipeline_error_message_handler_remove).
* _Buffering Message Received_ - with [`dsl_pipeline_buffering_message_handler_add`](#dsl_pipeline_buffering_message_handler_add) / [`dsl_pipeline_buffering_message_handler_remove`](#dsl_pipeline_buffering_message_handler_remove)

**IMPORTANT!** The Pipelines's _End of Stream (EOS)_ behavior changes if a client listener is added.
* if no listener - the Sink will stop the Pipeline and quit the main-loop.
* if listener(s) added - the Pipeline will only call the listeners(s). It is up to the application to handle the event by stopping the Pipeline and quitting the main-loop. 

---
## Pipeline API
**Client Callback Typedefs**
* [`dsl_state_change_listener_cb`](#dsl_state_change_listener_cb)
* [`dsl_eos_listener_cb`](#dsl_eos_listener_cb)
* [`dsl_error_message_handler_cb`](#dsl_error_message_handler_cb)
* [`dsl_buffering_message_handler_cb`](#dsl_buffering_message_handler_cb)

**Constructors**
* [`dsl_pipeline_new`](#dsl_pipeline_new)
* [`dsl_pipeline_new_many`](#dsl_pipeline_new_many)
* [`dsl_pipeline_new_component_add_many`](#dsl_pipeline_new_component_add_many)

**Destructors**
* [`dsl_pipeline_delete`](#dsl_pipeline_delete)
* [`dsl_pipeline_delete_many`](#dsl_pipeline_delete_many)
* [`dsl_pipeline_delete_all`](#dsl_pipeline_delete_all)

**Audio Mixer Methods**
* [`dsl_pipeline_audiomix_enabled_get`](#dsl_pipeline_audiomix_enabled_get)
* [`dsl_pipeline_audiomix_enabled_set`](#dsl_pipeline_audiomix_enabled_set)
* [`dsl_pipeline_audiomix_mute_enabled_get`](#dsl_pipeline_audiomix_mute_enabled_get)
* [`dsl_pipeline_audiomix_mute_enabled_set`](#dsl_pipeline_audiomix_mute_enabled_set)
* [`dsl_pipeline_audiomix_mute_enabled_set_many`](#dsl_pipeline_audiomix_mute_enabled_set_many)
* [`dsl_pipeline_audiomix_volume_get`](#dsl_pipeline_audiomix_volume_get)
* [`dsl_pipeline_audiomix_volume_set`](#dsl_pipeline_audiomix_volume_set)
* [`dsl_pipeline_audiomix_volume_set_many`](#dsl_pipeline_audiomix_volume_set_many)

**New Streammuxer Methods**
* [`dsl_pipeline_streammux_config_file_get`](#dsl_pipeline_streammux_config_file_get)
* [`dsl_pipeline_streammux_config_file_set`](#dsl_pipeline_streammux_config_file_set)
* [`dsl_pipeline_streammux_batch_size_get`](#dsl_pipeline_streammux_batch_size_get)
* [`dsl_pipeline_streammux_batch_size_set`](#dsl_pipeline_streammux_batch_size_set)

**Old Streammuxer Methods**
* [`dsl_pipeline_streammux_batch_properties_get`](#dsl_pipeline_streammux_batch_properties_get)
* [`dsl_pipeline_streammux_batch_properties_set`](#dsl_pipeline_streammux_batch_properties_set)
* [`dsl_pipeline_streammux_dimensions_get`](#dsl_pipeline_streammux_dimensions_get)
* [`dsl_pipeline_streammux_dimensions_set`](#dsl_pipeline_streammux_dimensions_set)
* [`dsl_pipeline_streammux_padding_get`](#dsl_pipeline_streammux_padding_get)
* [`dsl_pipeline_streammux_padding_set`](#dsl_pipeline_streammux_padding_set)
* [`dsl_pipeline_streammux_gpuid_get`](#dsl_pipeline_streammux_gpuid_get)
* [`dsl_pipeline_streammux_gpuid_set`](#dsl_pipeline_streammux_gpuid_set)
* [`dsl_pipeline_streammux_nvbuf_mem_type_get`](#dsl_pipeline_streammux_nvbuf_mem_type_get)
* [`dsl_pipeline_streammux_nvbuf_mem_type_set`](#dsl_pipeline_streammux_nvbuf_mem_type_set)

**Common Streammuxer Methods**
* [`dsl_pipeline_streammux_num_surfaces_per_frame_get`](#dsl_pipeline_streammux_num_surfaces_per_frame_get)
* [`dsl_pipeline_streammux_num_surfaces_per_frame_set`](#dsl_pipeline_streammux_num_surfaces_per_frame_set)
* [`dsl_pipeline_streammux_attach_sys_ts_enabled_get`](#dsl_pipeline_streammux_attach_sys_ts_enabled_get)
* [`dsl_pipeline_streammux_attach_sys_ts_enabled_set`](#dsl_pipeline_streammux_attach_sys_ts_enabled_set)
* [`dsl_pipeline_streammux_sync_inputs_enabled_get`](#dsl_pipeline_streammux_sync_inputs_enabled_get)
* [`dsl_pipeline_streammux_sync_inputs_enabled_set`](#dsl_pipeline_streammux_sync_inputs_enabled_set)
* [`dsl_pipeline_streammux_max_latency_get`](#dsl_pipeline_streammux_max_latency_get)
* [`dsl_pipeline_streammux_max_latency_set`](#dsl_pipeline_streammux_max_latency_set)
* [`dsl_pipeline_streammux_tiler_add`](#dsl_pipeline_streammux_tiler_add)
* [`dsl_pipeline_streammux_tiler_remove`](#dsl_pipeline_streammux_tiler_remove)
* [`dsl_pipeline_streammux_pph_add`](#dsl_pipeline_streammux_pph_add)
* [`dsl_pipeline_streammux_pph_remove`](#dsl_pipeline_streammux_pph_remove)

**Pipeline Methods**
* [`dsl_pipeline_component_add`](#dsl_pipeline_component_add)
* [`dsl_pipeline_component_add_many`](#dsl_pipeline_component_add_many)
* [`dsl_pipeline_component_list_size`](#dsl_pipeline_component_list_size)
* [`dsl_pipeline_component_remove`](#dsl_pipeline_component_remove)
* [`dsl_pipeline_component_remove_many`](#dsl_pipeline_component_remove_many)
* [`dsl_pipeline_component_remove_all`](#dsl_pipeline_component_remove_all)
* [`dsl_pipeline_state_get`](#dsl_pipeline_state_get)
* [`dsl_pipeline_state_change_listener_add`](#dsl_pipeline_state_change_listener_add)
* [`dsl_pipeline_state_change_listener_remove`](#dsl_pipeline_state_change_listener_remove)
* [`dsl_pipeline_eos_listener_add`](#dsl_pipeline_eos_listener_add)
* [`dsl_pipeline_eos_listener_remove`](#dsl_pipeline_eos_listener_remove)
* [`dsl_pipeline_error_message_handler_add`](#dsl_pipeline_error_message_handler_add)
* [`dsl_pipeline_error_message_handler_remove`](#dsl_pipeline_error_message_handler_remove)
* [`dsl_pipeline_error_message_last_get`](#dsl_pipeline_error_message_last_get)
* [`dsl_pipeline_buffering_message_handler_add`](#dsl_pipeline_buffering_message_handler_add)
* [`dsl_pipeline_buffering_message_handler_remove`](#dsl_pipeline_buffering_message_handler_remove)
* [`dsl_pipeline_link_method_get`](#dsl_pipeline_link_method_get)
* [`dsl_pipeline_link_method_set`](#dsl_pipeline_link_method_set)
* [`dsl_pipeline_play`](#dsl_pipeline_play)
* [`dsl_pipeline_pause`](#dsl_pipeline_pause)
* [`dsl_pipeline_stop`](#dsl_pipeline_stop)
* [`dsl_pipeline_main_loop_new`](#dsl_pipeline_main_loop_new)
* [`dsl_pipeline_main_loop_run`](#dsl_pipeline_main_loop_run)
* [`dsl_pipeline_main_loop_quit`](#dsl_pipeline_main_loop_quit)
* [`dsl_pipeline_main_loop_delete`](#dsl_pipeline_main_loop_delete)
* [`dsl_pipeline_list_size`](#dsl_pipeline_list_size)
* [`dsl_pipeline_dump_to_dot`](#dsl_pipeline_dump_to_dot)
* [`dsl_pipeline_dump_to_dot_with_ts`](#dsl_pipeline_dump_to_dot_with_ts)

---
## Return Values
The following return codes are used by the Pipeline API
```C++
#define DSL_RESULT_PIPELINE_RESULT                                  0x00080000
#define DSL_RESULT_PIPELINE_NAME_NOT_UNIQUE                         0x00080001
#define DSL_RESULT_PIPELINE_NAME_NOT_FOUND                          0x00080002
#define DSL_RESULT_PIPELINE_NAME_BAD_FORMAT                         0x00080003
#define DSL_RESULT_PIPELINE_STATE_PAUSED                            0x00080004
#define DSL_RESULT_PIPELINE_STATE_RUNNING                           0x00080005
#define DSL_RESULT_PIPELINE_THREW_EXCEPTION                         0x00080006
#define DSL_RESULT_PIPELINE_COMPONENT_ADD_FAILED                    0x00080007
#define DSL_RESULT_PIPELINE_COMPONENT_REMOVE_FAILED                 0x00080008
#define DSL_RESULT_PIPELINE_STREAMMUX_GET_FAILED                    0x00080009
#define DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED                    0x0008000A
#define DSL_RESULT_PIPELINE_CALLBACK_ADD_FAILED                     0x0008000B
#define DSL_RESULT_PIPELINE_CALLBACK_REMOVE_FAILED                  0x0008000C
#define DSL_RESULT_PIPELINE_FAILED_TO_PLAY                          0x0008000D
#define DSL_RESULT_PIPELINE_FAILED_TO_PAUSE                         0x0008000E
#define DSL_RESULT_PIPELINE_FAILED_TO_STOP                          0x0008000F
#define DSL_RESULT_PIPELINE_MAIN_LOOP_REQUEST_FAILED                0x00080010
```

## Pipeline Streammuxer Constant Values
```C
#define DSL_STREAMMUX_DEFAULT_WIDTH                                 DSL_1K_HD_WIDTH
#define DSL_STREAMMUX_DEFAULT_HEIGHT                                DSL_1K_HD_HEIGHT
```

## NVIDIA Buffer Memory Types
Jetson 0 & 4 only, dGPU 0 through 3 only
```C
#define DSL_NVBUF_MEM_TYPE_DEFAULT                                  0
#define DSL_NVBUF_MEM_TYPE_CUDA_PINNED                              1
#define DSL_NVBUF_MEM_TYPE_CUDA_DEVICE                              2
#define DSL_NVBUF_MEM_TYPE_CUDA_UNIFIED                             3
#define DSL_NVBUF_MEM_TYPE_SURFACE_ARRAY                            4
```

## Pipeline Link Methods
```C
#define DSL_PIPELINE_LINK_METHOD_BY_POSITION                        0
#define DSL_PIPELINE_LINK_METHOD_BY_ADD_ORDER                       1
#define DSL_PIPELINE_LINK_METHOD_DEFAULT                            DSL_PIPELINE_LINK_METHOD_BY_POSITION
```

## Pipeline States
```C
#define DSL_STATE_NULL                                              1
#define DSL_STATE_READY                                             2
#define DSL_STATE_PAUSED                                            3
#define DSL_STATE_PLAYING                                           4
#define DSL_STATE_IN_TRANSITION                                     5
```

<br>

---

## Client Callback Typedefs
### *dsl_state_change_listener_cb*
```C++
typedef void (*dsl_state_change_listener_cb)(uint old_state, 
    uint new_state, void* client_data);
```
Callback typedef for a client state-change listener. Functions of this type are added to a Pipeline by calling [dsl_pipeline_state_change_listener_add](#dsl_pipeline_state_change_listener_add). Once added, the function will be called on every change of Pipeline state until the client removes the listener by calling [dsl_pipeline_state_change_listener_remove](#dsl_pipeline_state_change_listener_remove).

**Parameters**
* `old_state` - [in] one of [DSL_PIPELINE_STATE](#DSL_PIPELINE_STATE) constants for the old (previous) pipeline state
* `new_state` - [in] one of [DSL_PIPELINE_STATE](#DSL_PIPELINE_STATE) constants for the new pipeline state
* `client_data` - [in] opaque pointer to client's user data, passed into the pipeline on callback add

<br>

### *dsl_eos_listener_cb*
```C++
typedef void (*dsl_eos_listener_cb)(void* client_data);
```
Callback typedef for a client EOS listener function. Functions of this type are added to a Pipeline by calling [dsl_pipeline_eos_listener_add](#dsl_pipeline_eos_listener_add). Once added, the function will be called on the event of Pipeline end-of-stream (EOS). The listener function is removed by calling [dsl_pipeline_eos_listener_remove](#dsl_pipeline_eos_listener_remove) .

**Parameters**
* `client_data` - [in] opaque pointer to client's user data, passed into the pipeline on callback add

<br>

### *dsl_error_message_handler_cb*
```C++
typedef void (*dsl_error_message_handler_cb)(const wchar_t* source, 
    const wchar_t* message, void* client_data);
```
Callback typedef for a client error-message-handler function. Functions of this type are added to a Pipeline by calling [dsl_pipeline_error_message_handler_add](#dsl_pipeline_error_message_handler_add). Once added, the function will be called in the event of an error message received by the Pipeline's bus-watcher. The handler function is removed by calling [dsl_pipeline_error_message_handler_remove](#dsl_pipeline_error_message_handler_remove) .

**Parameters**
* `source` - [in] name of the GST Object that is the source of the message
* `message` - [in] message error parsed from the message data
* `client_data` - [in] opaque pointer to client's user data, passed into the pipeline on callback add

<br>

## *dsl_buffering_message_handler_cb*
```C++
typedef void (*dsl_buffering_message_handler_cb)(const wchar_t* source, 
    uint percent, void* client_data);
```
Callback typedef for a client buffering-message-handler function. Functions of this type are added to a Pipeline by calling [dsl_pipeline_buffering_message_handler_add](#dsl_pipeline_buffering_message_handler_add). Once added, the function will be called in the event of a buffering message received by the Pipeline's bus-watcher. The handler function is removed by calling [dsl_pipeline_buffering_message_handler_remove](#dsl_pipeline_buffering_message_handler_remove) .

**Parameters**
* `source` - [in] name of the GST Object that is the source of the message
* `percent` - [in] percent buffering. 100% means buffering is done.
* `client_data` - [in] opaque pointer to client's user data, passed into the pipeline on callback add

<br>


---
## Constructors
### *dsl_pipeline_new*
```C++
DslReturnType dsl_pipeline_new(const wchar_t* pipeline);
```
The constructor creates a uniquely named Pipeline. Construction will fail
if the name is currently in use.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to create.

**Returns**
* `DSL_RESULT_SUCCESS` on successful creation. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_new('my-pipeline')
```

<br>

### *dsl_pipeline_new_many*
```C++
DslReturnType dsl_pipeline_new_many(const wchar_t** pipelines);
```
The constructor creates multiple uniquely named Pipelines at once. All names are checked for uniqueness, with the call returning `DSL_RESULT_PIPELINE_NAME_NOT_UNIQUE` on first occurrence of a duplicate.

**Parameters**
* `pipelines` - [in] a NULL terminated array of unique names for the Pipelines to create.

**Returns**
* `DSL_RESULT_SUCCESS` on successful creation of all Pipelines. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_new_many(['my-pipeline-a', 'my-pipeline-b', 'my-pipeline-c', None])
```

<br>

### *dsl_pipeline_new_component_add_many*
```C++
DslReturnType dsl_pipeline_new_component_add_many(const wchar_t* pipeline, const wchar_t** components);
```
Creates a new Pipeline with a given list of named Components. The service will fail if any of the components are currently `in-use` by any Pipeline. All of the component's `in-use` states will be set to true on successful add.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to create.
* `components` - [in] a NULL terminated array of uniquely named Components to add.

**Returns**
* `DSL_RESULT_SUCCESS` on successful creation and addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_new_component_add_many('my-pipeline',
    ['my-camera-source', 'my-pgie`, 'my-osd' 'my-tiled-display', 'my-sink', None])
```

<br>

---
## Destructors
### *dsl_pipeline_delete*
```C++
DslReturnType dsl_pipeline_delete(const wchar_t* pipeline);
```
This destructor deletes a single, uniquely named Pipeline.
All components owned by the pipeline move to a state of `not-in-use`.

**Parameters**
* `pipelines` - [in] unique name for the Pipeline to delete

**Returns**
* `DSL_RESULT_SUCCESS` on successful deletion. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_delete('my-pipeline')
```

<br>

### *dsl_pipeline_delete_many*
```C++
DslReturnType dsl_pipeline_delete_many(const wchar_t** pipelines);
```
This destructor deletes multiple uniquely named Pipelines. Each name is checked for existence, with the function returning `DSL_RESULT_PIPELINE_NAME_NOT_FOUND` on first occurrence of failure.
All components owned by the Pipelines move to a state of `not-in-use`

**Parameters**
* `pipelines` - [in] a NULL terminated array of uniquely named Pipelines to delete.

**Returns**
* `DSL_RESULT_SUCCESS` on successful deletion. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_delete_many(['my-pipeline-a', 'my-pipeline-b', 'my-pipeline-c', None])
```

<br>

### *dsl_pipeline_delete_all*
```C++
DslReturnType dsl_pipeline_delete_all();
```
This destructor deletes all Pipelines currently in memory. All components owned by the pipelines move to a state of `not-in-use`

**Returns**
* `DSL_RESULT_SUCCESS` on successful deletion. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_delete_all()
```

<br>

---

## Audio Mixer Methods
### *dsl_pipeline_audiomix_enabled_get*
```C++
DslReturnType dsl_pipeline_audiomix_enabled_get(const wchar_t* name, 
    boolean* enabled);
```
This service returns the current enabled/disabled setting for the named Pipeline's Audiomixer.

**IMPORTANT!** The Pipeline Audio Mixer is mutully exclusive with the Audio Streammuxer. Only one can be enabled. 

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to query.
* `enabled` - [out] true if the Audiomixer is enabled, false if not. 

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, enabled = dsl_pipeline_audiomix_enabled_get('my-pipeline')
```

<br>

### *dsl_pipeline_audiomix_enabled_set*
```C++
DslReturnType dsl_pipeline_audiomix_enabled_set(const wchar_t* name, 
    boolean enabled);
```
This service sets the current enabled/disabled setting for the named Pipeline's Audiomixer. 

**IMPORTANT!** The Pipeline Audio Mixer is mutully exclusive with the Audio Streammuxer. Only one can be enabled. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `enabled` - [in] set to true to enable the Audiomixer, false to disable.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_audiomix_enabled_set('my-pipeline', True)
```

<br>

### *dsl_pipeline_audiomix_mute_enabled_get*
```C++
DslReturnType dsl_pipeline_audiomix_mute_enabled_get(const wchar_t* name, 
    const wchar_t* source, boolean* enabled);
```
This service returns the Audiomixer's mute enabled setting for a specific Audio Source.

**IMPORTANT!** The Source must be a child of the named Pipeline or the service will fail. 

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to query.
* `source` - [in] name of the Audio Source for the mute enabled setting.
* `enabled` - [out] if true, then the Audiomixer's sink pad connected to the named Audio Source is currently muted.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, enabled = dsl_pipeline_audiomix_mute_enabled_get('my-pipeline', 'my-source-1')
```

<br>

### *dsl_pipeline_audiomix_mute_enabled_set*
```C++
DslReturnType dsl_pipeline_audiomix_mute_enabled_set(const wchar_t* name, 
    const wchar_t* source, boolean enabled);
```
This service sets the Audiomixer's mute enabled setting for a specific Audio Source.

**IMPORTANT!** The Source must be a child of the named Pipeline or the service will fail. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `source` - [in] name of the Audio Source for the mute enabled setting.
* `enabled` - [in] set to true to mute the Audiomixer's sink pad connected to the named Audio Source, false to unmute.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_audiomix_mute_enabled_set('my-pipeline', 
    'my-source-1', True)
```

<br>

### *dsl_pipeline_audiomix_mute_enabled_set_many*
```C++
DslReturnType dsl_pipeline_audiomix_mute_enabled_set_many(const wchar_t* name, 
    const wchar_t** sources, boolean enabled);
```
This service sets he Audiomixer's mute enabled setting for a null terminated list of Audio Sources.

**IMPORTANT!** All Sources must be a child of the named Pipeline or the service will fail. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `sources` - [in] null-terminated list of Audio Sources to mute or unmute.
* `enabled` - [in] set to true to mute the Audiomixer's sink pads connected to the named Audio Sources, false to unmute.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_audiomix_mute_enabled_set_many('my-pipeline', 
    ['my-source-1', 'my-source-2', 'my-source-3', None], True)
```

<br>

### *dsl_pipeline_audiomix_volume_get*
```C++
DslReturnType dsl_pipeline_audiomix_volume_get(const wchar_t* name, 
    const wchar_t* source, double* volume);
```
This service returns the Audiomixer's volume setting for a specific Audio Source.

**IMPORTANT!** The Source must be a child of the named Pipeline or the service will fail. 

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to query.
* `source` - [in] name of the Audio Source for the volume setting.
* `volume` - [out] the volume that is assigned to the Audiomixer's sink pad connected to the named Audio Source, between 0.0 and 10.0. Default = 1.0

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, volume = dsl_pipeline_audiomix_volume_get('my-pipeline', 'my-source-1')
```

<br>

### *dsl_pipeline_audiomix_volume_set*
```C++
DslReturnType dsl_pipeline_audiomix_volume_set(const wchar_t* name, 
    const wchar_t* source, double volume);
```
This service sets the Audiomixer's volume setting for a specific Audio Source.

**IMPORTANT!** The Source must be a child of the named Pipeline or the service will fail. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `source` - [in] name of the Audio Source for the volume setting.
* `volume` - [in] a value between 0.0 and 10.0 for the sink pad connected to the named Audio Source. Default = 1.0.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_audiomix_volume_set('my-pipeline', 
    'my-source-1', 2.5)
```

<br>

### *dsl_pipeline_audiomix_volume_set_many*
```C++
DslReturnType dsl_pipeline_audiomix_volume_set_many(const wchar_t* name, 
    const wchar_t** sources, double volume);
```
This service sets the Audiomixer's volume setting for a null-terminated list of Audio Sources.

**IMPORTANT!** All Sources must be a child of the named Pipeline or the service will fail. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `sources` - [in] null terminated list of Audio Source to set the volume setting.
* `volume` - [in] a value between 0.0 and 10.0 for the sink pad connected to the named Audio Source. Default = 1.0.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_audiomix_volume_set_many('my-pipeline', 
    ['my-source-1', 'my-source-2', 'my-source-3', None], 2.5)
```

<br>

---

## New Streammuxer Methods
### *dsl_pipeline_streammux_config_file_get*
```C++
DslReturnType dsl_pipeline_streammux_config_file_get(const wchar_t* name, 
    const wchar_t** config_file);
```
This service returns the current Streammuxer config file in use by the named Pipeline. To use this service, `export USE_NEW_NVSTREAMMUX=yes`.

**IMPORTANT!** The Streammuxer will use default [configuration values](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux2.html#mux-config-properties) if a configuration file is not provided. 

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to query.
* `config_file` - [out] path specification for the current Streammuxer config file in use. Default = NULL. 

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, config_file = dsl_pipeline_streammux_config_file_get('my-pipeline')
```

<br>

### *dsl_pipeline_streammux_config_file_set*
```C++
DslReturnType dsl_pipeline_streammux_config_file_set(const wchar_t* name, 
    const wchar_t* config_file);
```
This service sets the Streammuxer configuration file for the named Pipeline to use. To use this service, `export USE_NEW_NVSTREAMMUX=yes`.

**IMPORTANT!** The Streammuxer will use default [configuration values](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvstreammux2.html#mux-config-properties) if a configuration file is not provided. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `config_file` - [in] absolute or relative path to the Streammuxer configuration file to use

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_config_file_set('my-pipeline', config_file)
```

<br>

### *dsl_pipeline_streammux_batch_size_get*
```C++
DslReturnType dsl_pipeline_streammux_batch_size_get(const wchar_t* name, 
    uint* batch_size);
```
This service returns the current `batch_size` setting for the named Pipeline's Streammuxer. To use this service, `export USE_NEW_NVSTREAMMUX=yes`.

**IMPORTANT:** the Pipeline will set the `batch_size` -- once playing -- to the current number of added Sources if not explicitly set by calling [`dsl_pipeline_streammux_batch_size_set`](#dsl_pipeline_streammux_batch_size_set).

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `batch_size` - [out] the current batch size, set by the Pipeline according to the current number of child Source components by default.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, batch_size = dsl_pipeline_streammux_batch_size_get('my-pipeline')
```

<br>

### *dsl_pipeline_streammux_batch_size_set*
```C++
DslReturnType dsl_pipeline_streammux_batch_size_set(const wchar_t* name, 
    uint batch_size);
```
This service sets the `batch_size` for the named Pipeline's Streammuxer to use. To use this service, `export USE_NEW_NVSTREAMMUX=yes`.

**IMPORTANT:** the Pipeline will set the `batch_size` -- once playing -- to the current number of added Sources if not explicitly set.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `batch_size` - [in] the new batch size to use

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_batch_size_set('my-pipeline', 30)
```

<br>

## Old Streammuxer Methods
### *dsl_pipeline_streammux_batch_properties_get*
```C++
DslReturnType dsl_pipeline_streammux_batch_properties_get(const wchar_t* pipeline,
    uint* batch_size, uint* batch_timeout);
```
This service returns the current `batch_size` and `batch_timeout` for the named Pipeline.

**Note:** The Pipeline will set the `batch_size` to the current number of added Sources if not explicitly set.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `batch_size` - [out] the current batch size, set by the Pipeline according to the current number of child Source components by default.
* `batch_timeout` - [out] timeout in milliseconds before a batch meta push is forced. The default is -1 == disabled.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, batch_size, batch_timeout = dsl_pipeline_streammux_batch_properties_get('my-pipeline')
```

<br>

### *dsl_pipeline_streammux_batch_properties_set*
```C++
DslReturnType dsl_pipeline_streammux_batch_properties_set(const wchar_t* pipeline,
    uint batch_size, uint batch_timeout);
```
This service sets the `batch_size` and `batch_timeout` for the named Pipeline to use.

**Note:** The Pipeline will set the `batch_size` to the current number of added Sources and the `batch_timeout` to `DSL_DEFAULT_STREAMMUX_BATCH_TIMEOUT` if not explicitly set.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `batch_size` - [in] the new batch size to use
* `batch_timeout` - [in] the new timeout in milliseconds before a batch meta push is forced.  The default is -1 == disabled.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_batch_properties_set('my-pipeline',
    batch_size, batch_timeout)
```

<br>

### *dsl_pipeline_streammux_dimensions_get*
```C++
DslReturnType dsl_pipeline_streammux_dimensions_get(const wchar_t* pipeline,
    uint* width, uint* height);
```
This service returns the current Streammuxer output dimensions for the uniquely named Pipeline. The default dimensions, defined in `DslApi.h`, are assigned during Pipeline creation. The values can be changed after creation by calling [dsl_pipeline_streammux_dimensions_set](#dsl_pipeline_streammux_dimensions_set)

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `width` - [out] width of the Streammuxer output in pixels.
* `height` - [out] height of the Streammuxer output in pixels.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, width, height = dsl_pipeline_streammux_dimensions_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_dimensions_set*
```C++
DslReturnType dsl_pipeline_streammux_dimensions_set(const wchar_t* pipeline,
    uint width, uint height);
```
This service sets the Streammuxer output dimensions for the uniquely named Pipeline. The dimensions cannot be updated while the Pipeline is in a state of `PAUSED` or `PLAYING`.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `width` - [in] new width for the Streammuxer output in pixels.
* `height` - [in] new height for the Streammuxer output in pixels.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_dimensions_set('my-pipeline', 1280, 720)
```
<br>

### *dsl_pipeline_streammux_padding_get*
```C++
DslReturnType dsl_pipeline_streammux_padding_get(const wchar_t* name, 
    boolean* enabled);
```
This service returns the current Streammuxer padding-enabled setting for the uniquely named Pipeline. If enabled, each input stream's aspect ratio will be maintained on scaling by padding with black bands.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `endabled` - [out] current value for the padding-enabled property..

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, enabled = dsl_pipeline_streammux_padding_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_padding_set*
```C++
DslReturnType dsl_pipeline_streammux_padding_set(const wchar_t* name, 
    boolean enabled);
```
This service sets the Streammuxer padding-enabled for the uniquely named Pipeline. The property cannot be updated while the Pipeline is linked and playing/paused.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `enabled` - [in] If true, each input stream's aspect ratio will be maintained on scaling by padding with black bands.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_padding_set('my-pipeline', True)
```
<br>

### *dsl_pipeline_streammux_gpuid_get*
```C++
DslReturnType dsl_pipeline_streammux_gpuid_get(const wchar_t* name, uint* gpuid);
```
This service returns the current Streammuxer GPU ID for the uniquely named Pipeline. The default GPU ID is 0. The value can be changed by calling [dsl_pipeline_streammux_gpuid_set](#dsl_pipeline_streammux_gpuid_set)

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `gpuid` - [out] current GPU ID.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, gpuid = dsl_pipeline_streammux_gpuid_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_gpuid_set*
```C++
DslReturnType dsl_pipeline_streammux_gpuid_set(const wchar_t* name, uint gpuid);
```
This service sets the Streammuxer GPU ID for the uniquely named Pipeline. The GPU ID cannot be updated while the Pipeline is linked and playing/paused.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `gpuid` - [in] new GPU ID for the Streammuxer.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_gpuid_set('my-pipeline', 1)
```
<br>

### *dsl_pipeline_streammux_nvbuf_mem_type_get*
```C++
DslReturnType dsl_pipeline_streammux_nvbuf_mem_type_get(const wchar_t* name, 
    uint* type);
```
This service returns the current Streammuxer [NVIDIA buffer-memory-type](#nvidia-buffer-memory-types) for the uniquely named Pipeline. The default type is `DSL_NVBUF_MEM_TYPE_DEFAULT`. The value can be changed by calling [dsl_pipeline_streammux_nvbuf_mem_type_set](#dsl_pipeline_streammux_nvbuf_mem_type_set)

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `type` - [out] one of the `DSL_NVBUF_MEM_TYPE` [constant values](#nvidia-buffer-memory-types).

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, type = dsl_pipeline_streammux_nvbuf_mem_type_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_nvbuf_mem_type_set*
```C++
DslReturnType dsl_pipeline_streammux_nvbuf_mem_type_set(const wchar_t* name, 
    uint type);
```
This service sets the Streammuxer [NVIDIA buffer-memory-type](#nvidia-buffer-memory-types) for the uniquely named Pipeline. The memory type cannot be updated while the Pipeline is linked and playing/paused.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `type` - [in] one of the `DSL_NVBUF_MEM_TYPE` [constant values](#nvidia-buffer-memory-types).

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_nvbuf_mem_type_set('my-pipeline',
    DSL_NVBUF_MEM_TYPE_CUDA_UNIFIED)
```
<br>

## Common Streammuxer Methods
### *dsl_pipeline_streammux_num_surfaces_per_frame_get*
```C++
DslReturnType dsl_pipeline_streammux_num_surfaces_per_frame_get(
    const wchar_t* name, uint* num);
```
This service gets the current num-surfaces-per-frame setting for the named Pipeline's Streammuxer.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `num` - [out] current number of surfaces per frame [1..4].

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, num_surfaces = dsl_pipeline_streammux_num_surfaces_per_frame_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_num_surfaces_per_frame_set*
```C++
DslReturnType dsl_pipeline_streammux_num_surfaces_per_frame_set(
    const wchar_t* name, uint num);
```
This service sets the num-surfaces-per-frame setting for the named Pipeline's Streammuxer. The setting cannot be updated while the Pipeline is in a state of `PAUSED` or `PLAYING`.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `num` - [in] new number of surfaces per frame [1..4].

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_num_surfaces_per_frame_set('my-pipeline', 2)
```
<br>

### *dsl_pipeline_streammux_attach_sys_ts_enabled_get*
```C++
DslReturnType dsl_pipeline_streammux_attach_sys_ts_enabled_get(const wchar_t* name, 
    boolean* enabled);
```
This service gets the current setting - enabled/disabled - for the Streammuxer attach-sys-ts property for the named Pipeline.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `enabled` - [out] true if the attach-sys-ts property is enabled, false otherwise.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, enabled = dsl_pipeline_streammux_attach_sys_ts_enabled_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_attach_sys_ts_enabled_set*
```C++
DslReturnType dsl_pipeline_streammux_attach_sys_ts_enabled_set(const wchar_t* name, 
    boolean enabled);
```
This service sets the attach-sys-ts Streammuxer setting for the named Pipeline. The setting cannot be updated while the Pipeline is in a state of `PAUSED` or `PLAYING`. 

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `enabled` - [in] set to true to enable the attach-sys-ts property, false to disable.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_attach_sys_ts_enabled_set('my-pipeline', False)
```
<br>

### *dsl_pipeline_streammux_sync_inputs_enabled_get*
```C++
DslReturnType dsl_pipeline_streammux_sync_inputs_enabled_get(const wchar_t* name, 
    boolean* enabled);
```
This service gets the current setting - enabled/disabled - for the Streammuxer attach-sys-ts property for the named Pipeline..

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `enabled` - [out] true if the sync-inputs property is enabled, false otherwise.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, enabled = dsl_pipeline_streammux_sync_inputs_enabled_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_sync_inputs_enabled_set*
```C++
DslReturnType dsl_pipeline_streammux_sync_inputs_enabled_set(const wchar_t* name, 
    boolean enabled);
```
This service sets the sync-inputs Streammuxer setting for the named Pipeline. The setting cannot be updated while the Pipeline is in a state of `PAUSED` or `PLAYING`. This service is typically used with live Sources to synchronize the streams with the network time.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `enabled` - [in] set to true to enable the sync-inputs property, false to disable.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_sync_inputs_enabled_set('my-pipeline', True)
```
<br>

### *dsl_pipeline_streammux_max_latency_get*
```C++
DslReturnType dsl_pipeline_streammux_max_latency_get(const wchar_t* name, 
    uint* max_latency);
```
This service gets the current max-latency setting in use by the named Pipeline's Streammuxer.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to query.
* `max_latency` - [out] the maximum upstream latency in nanoseconds. When sync-inputs=1, buffers coming in after max-latency shall be dropped. Default = 0.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, max_latency = dsl_pipeline_streammux_max_latency_get('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_max_latency_set*
```C++
DslReturnType dsl_pipeline_streammux_max_latency_set(const wchar_t* name, 
    uint max_latency);
```
This service sets the max-latency setting for the named Pipeline's Streammuxer to use.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `max_latency` - [in] the maximum upstream latency in nanoseconds. When sync-inputs=1, buffers coming in after max-latency shall be dropped.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_max_latency_set('my-pipeline', 10000)
```
<br>

### *dsl_pipeline_streammux_tiler_add*
```C++
DslReturnType dsl_pipeline_streammux_tiler_add(const wchar_t* name, 
    const wchar_t* tiler);
```
This service adds a named Tiler to a named Pipeline's Streammuxer output prior to any inference components added to the Pipeline.

Note: A Streammuxer can have at most one Tiler.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `tiler` - [in] unique name of the Tiler to add.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_tiler_add('my-pipeline', 'my-tiler')
```
<br>

### *dsl_pipeline_streammux_tiler_remove*
```C++
DslReturnType dsl_pipeline_streammux_tiler_remove(const wchar_t* name);
```
This service removes a named Tiler from a named Pipeline's Streammuxer previously added with [dsl_pipeline_streammux_tiler_add](#dsl_pipeline_streammux_tiler_add).

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_tiler_remove('my-pipeline')
```
<br>

### *dsl_pipeline_streammux_pph_add*
```C++
DslReturnType dsl_pipeline_streammux_pph_add(const wchar_t* name, 
    const wchar_t* handler);
```
This service adds a named Pad-Probe-Handler to a named Pipeline's Streammuxer. One or more Pad Probe Handlers can be added to the SOURCE PAD only.

**Parameters**
 * `name` [in] unique name of the Pipeline to update
 * `handler` [in] unique name of the PPH to add

**Returns**
* `DSL_RESULT_SUCCESS` on successful add. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_pph_add('my-pipeline', 
    'my-source-meter-pph')
```

<br>

### *dsl_pipeline_streammux_pph_remove*
```C++
DslReturnType dsl_pipeline_streammux_pph_remove(const wchar_t* name, 
    const wchar_t* handler);
```
This service removes a named Pad-Probe-Handler from a named Pipeline's Streammuxer.

**Parameters**
 * `name` [in] unique name of the Pipeline to update.
 * `handler` [in] unique name of the Pad probe handler to remove.

**Returns**
* `DSL_RESULT_SUCCESS` on successful remove. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_streammux_pph_remove('my-pipeline', 
    'my-source-meter-pph')
```

<br>

## Pipeline Methods
### *dsl_pipeline_component_add*
```C++
DslReturnType dsl_pipeline_component_add(const wchar_t* pipeline, 
    const wchar_t* component);
```
Adds a single named Component to a named Pipeline. The add service will fail if the component is currently `in-use` by any Pipeline. The add service will also fail if adding a `one-only` type of Component, such as a Tiled-Display, when the Pipeline already has one. The Component's `in-use` state will be set to `true` on successful add.

If a Pipeline is in a `playing` or `paused` state, the service will attempt a dynamic update if possible, returning from the call with the Pipeline in the same state.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `component` - [in] unique name of the Component to add.

**Returns**
* `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_source_csi_new('my-camera-source', 1280, 720, 30, 1)
retval = dsl_pipeline_new('my-pipeline')

retval = dsl_pipeline_component_add('my-pipeline', 'my-camera-source')
```

<br>

### *dsl_pipeline_component_add_many*
```C++
DslReturnType dsl_pipeline_component_add_many(const wchar_t* pipeline, 
    const wchar_t** components);
```
Adds a list of named components to a named Pipeline. The add service will fail if any of the components are currently `in-use` by any Pipeline. The add service will fail if any of the components to add are a `one-only` type and the Pipeline already has one. All of the component's `in-use` states will be set to true on successful add.

If a Pipeline is in a `playing` or `paused` state, the service will attempt a dynamic update if possible, returning from the call with the Pipeline in the same state.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `components` - [in] a NULL terminated array of uniquely named Components to add.

**Returns**
* `DSL_RESULT_SUCCESS` on successful  addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_source_csi_new('my-camera-source', 1280, 720, 30, 1)
retval = dsl_display_new('my-tiled-display', 1280, 720)
retval = dsl_pipeline_new('my-pipeline')

retval = dsl_pipeline_component_add_many('my-pipeline', 
    ['my-camera-source', 'my-tiled-display', None])
```

<br>

### *dsl_pipeline_component_list_size*
```C++
uint dsl_pipeline_list_size(wchar_t* pipeline);
```
This method returns the size of the current list of Components `in-use` by the named Pipeline

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.

**Returns**
* The size of the list of Components currently in use

<br>

### *dsl_pipeline_component_remove*
```C++
DslReturnType dsl_pipeline_component_remove(const wchar_t* pipeline, const wchar_t* component);
```
Removes a single named Component from a named Pipeline. The remove service will fail if the Component is not currently `in-use` by the Pipeline. The Component's `in-use` state will be set to `false` on successful removal.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `component` - [in] unique name of the Component to remove.

**Returns**
* `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_component_remove('my-pipeline', 'my-camera-source')
```
<br>

### *dsl_pipeline_component_remove_many*
```C++
DslReturnType dsl_pipeline_component_remove_many(const wchar_t* pipeline, const wchar_t** components);
```
Removes a list of named components from a named Pipeline. The remove service will fail if any of the components are currently `not-in-use` by the named Pipeline.  All of the removed component's `in-use` state will be set to `false` on successful removal.

If a Pipeline is in a `playing` or `paused` state, the service will attempt a dynamic update if possible, returning from the call with the Pipeline in the same state.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `components` - [in] a NULL terminated array of uniquely named Components to add.

**Returns**
* `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_component_remove_many('my-pipeline', 
    ['my-camera-source', 'my-tiled-display', None])
```

<br>

### *dsl_pipeline_component_remove_all*
```C++
DslReturnType dsl_pipeline_component_add_(const wchar_t* pipeline);
```
Removes all child components from a named Pipeline. The remove service will fail if any of the components are currently `not-in-use` by the named Pipeline.  All of the removed component's `in-use` state will be set to `false` on successful removal.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.

**Returns**
* `DSL_RESULT_SUCCESS` on successful add. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_component_remove_all('my-pipeline')
```

<br>

### *dsl_pipeline_state_change_listener_add*
```C++
DslReturnType dsl_pipeline_state_change_listener_add(const wchar_t* pipeline,
    state_change_listener_cb listener, void* client_data);
```
This service adds a callback function of type [dsl_state_change_listener_cb](#dsl_state_change_listener_cb) to a
pipeline identified by its unique name. The function will be called on every Pipeline change-of-state with `old_state`, `new_state`, and the client provided `client_data`. Multiple callback functions can be registered with one Pipeline, and one callback function can be registered with multiple Pipelines.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `listener` - [in] state change listener callback function to add.
* `client_data` - [in] opaque pointer to user data returned to the listener is called back

**Returns**
* `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
def state_change_listener(old_state, new_state, client_data, client_data):
    print('old_state = ', old_state)
    print('new_state = ', new_state)
   
retval = dsl_pipeline_state_change_listener_add('my-pipeline', 
    state_change_listener, None)
```

<br>

### *dsl_pipeline_state_change_listener_remove*
```C++
DslReturnType dsl_pipeline_state_change_listener_remove(const wchar_t* pipeline,
    state_change_listener_cb listener);
```
This service removes a callback function of type [state_change_listener_cb](#state_change_listener_cb) from a
pipeline identified by its unique name.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `listener` - [in] state change listener callback function to remove.

**Returns**  
* `DSL_RESULT_SUCCESS` on successful removal. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_state_change_listener_remove('my-pipeline', 
    state_change_listener)
```

<br>

### *dsl_pipeline_eos_listener_add*
```C++
DslReturnType dsl_pipeline_eos_listener_add(const wchar_t* pipeline,
    eos_listener_cb listener, void* client_data);
```
This service adds a callback function of type [dsl_eos_listener_cb](#dsl_eos_listener_cb) to a Pipeline identified by its unique name. The function will be called on a Pipeline `EOS` event. Multiple callback functions can be registered with one Pipeline, and one callback function can be registered with multiple Pipelines.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `listener` - [in] EOS listener callback function to add.
* `client_data` - [in] opaque pointer to user data returned to the listener is called back

**Returns**  `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
def eos_listener(client_data):
    print('EOS event received')
   
retval = dsl_pipeline_eos_listener_add('my-pipeline', eos_listener, None)
```

<br>

### *dsl_pipeline_eos_listener_remove*
```C++
DslReturnType dsl_pipeline_eos_listener_remove(const wchar_t* pipeline,
    dsl_eos_listener_cb listener);
```
This service removes a callback function of type [dsl_eos_listener_cb](#dsl_eos_listener_cb) from a Pipeline identified by its unique name.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `listener` - [in] EOS listener callback function to remove.

**Returns**  
* `DSL_RESULT_SUCCESS` on successful removal. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_eos_listener_remove('my-pipeline', eos_listener)
```

<br>


### *dsl_pipeline_error_message_handler_add*
```C++
DslReturnType dsl_pipeline_error_message_handler_add(const wchar_t* pipeline,
    dsl_error_message_handler_cb handler, void* client_data);
```
This service adds a callback function of type [dsl_error_message_handler_cb](#dsl_error_message_handler_cb) to a Pipeline identified by its unique name. The function will be called when the Pipeline's bus-watcher receives an error message from one of the GST Objects. Multiple callback functions can be registered with one Pipeline, and one callback function can be registered with multiple Pipelines.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `handler` - [in] error message handler callback function to add.
* `client_data` - [in] opaque pointer to user data returned to the listener is called back

**Returns**  `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
def error_message_handler(source, message, client_data):
    print('Error: source = ', source, ' message = ', message)
    dsl_main_loop_quit()
   
retval = dsl_pipeline_error_message_handler_add('my-pipeline', 
    error_message_handler, None)
```

<br>

### *dsl_pipeline_error_message_handler_remove*
```C++
DslReturnType dsl_pipeline_error_message_handler_remove(const wchar_t* pipeline,
    dsl_error_message_handler_cb handler);
```
This service remove a callback function of type [dsl_error_message_handler_cb](#dsl_error_message_handler_cb), previously added with [dsl_pipeline_error_message_handler_add](#dsl_pipeline_error_message_handler_add), from a Pipeline identified by its unique name.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `handler` - [in] error message handler callback function to remove.

**Returns**  `DSL_RESULT_SUCCESS` on successful removal. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_error_message_handler_remove('my-pipeline', 
    error_message_handler)
```

<br>

### *dsl_pipeline_buffering_message_handler_add*
```C++
DslReturnType dsl_pipeline_buffering_message_handler_add(const wchar_t* pipeline,
    dsl_buffering_message_handler_cb handler, void* client_data);
```
This service adds a callback function of type [dsl_buffering_message_handler_cb](#dsl_buffering_message_handler_cb) to a Pipeline identified by its unique name. The function will be called when the Pipeline's bus-watcher receives an buffering message from one of the GST Objects. Multiple callback functions can be registered with one Pipeline, and one callback function can be registered with multiple Pipelines.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `handler` - [in] buffering message handler callback function to add.
* `client_data` - [in] opaque pointer to user data returned to the listener is called back

**Returns**  `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
## 
# Function to be called when a buffering-message is received on the Pipeline bus.
## 
def buffering_message_handler(source, percent, client_data):

    global buffering

    if percent == 100:
        print('playing pipeline - buffering complete at 100 % for Source', source)
        dsl_pipeline_play('pipeline')
        buffering = False

    else:
        if not buffering:
            print('pausing pipeline - buffering starting at ', percent,
                '% for Source', source)
            dsl_pipeline_pause('pipeline')
        buffering = True

# Add the callback to the Pipeline   
retval = dsl_pipeline_buffering_message_handler_add('my-pipeline', 
    buffering_message_handler, None)
```

<br>

### *dsl_pipeline_buffering_message_handler_remove*
```C++
DslReturnType dsl_pipeline_buffering_message_handler_remove(const wchar_t* pipeline,
    dsl_buffering_message_handler_cb handler);
```
This service remove a callback function of type [dsl_buffering_message_handler_cb](#dsl_buffering_message_handler_cb), previously added with [dsl_pipeline_buffering_message_handler_add](#dsl_pipeline_buffering_message_handler_add), from a Pipeline identified by its unique name.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `handler` - [in] buffering message handler callback function to remove.

**Returns**  `DSL_RESULT_SUCCESS` on successful removal. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_buffering_message_handler_remove('my-pipeline', 
    buffering_message_handler)
```

<br>

### *dsl_pipeline_error_message_last_get*
```C++
DslReturnType dsl_pipeline_error_message_last_get(const wchar_t* pipeline,
    const wchar_t** source, const wchar_t** message);
```
This service gets the last error message received by the Pipeline's bus watcher. The parameters `source` and `message` will return `NULL` until the first message is received.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to update.
* `source` - [out] source name of the GST object that was the source of the error message.
* `message` - [out] message error message sent from the source object.

**Returns**  `DSL_RESULT_SUCCESS` on successful removal. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval, source, message = dsl_pipeline_error_message_last_get('my-pipeline')
```

<br>

### *dsl_pipeline_link_method_get*
```C++
DslReturnType dsl_pipeline_link_method_get(const wchar_t* name, uint* link_method);
```
This service gets the current link method in use by the named Pipeline.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `link_method` - [out] One of the [Pipeline Link Methods](#pipeline-link-methods) defined above. Default = `DSL_PIPELINE_LINK_METHOD_BY_POSITION`.

**Returns**
* `DSL_RESULT_SUCCESS` on successful query. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval, link_method = dsl_pipeline_link_method_get('my-pipeline')
```
<br>

### *dsl_pipeline_link_method_set*
```C++
DslReturnType dsl_pipeline_link_method_set(const wchar_t* name, uint link_method);
```
This service sets the link method for the named Pipeline to use.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to update.
* `link_method` - [in] One of the [Pipeline Link Methods](#pipeline-link-methods) defined above. Default = `DSL_PIPELINE_LINK_METHOD_BY_POSITION`.

**Returns**
* `DSL_RESULT_SUCCESS` on successful update. One of the [Return Values](#return-values) defined above on failure

**Python Example**
```Python
retval = dsl_pipeline_link_method_set('my-pipeline', DSL_PIPELINE_LINK_METHOD_BY_ORDER)
```
<br>

### *dsl_pipeline_play*
```C++
DslReturnType dsl_pipeline_play(wchar_t* pipeline);
```
This service is used to play a named Pipeline. The service will fail if the Pipeline's list of components are insufficient for the Pipeline to play. The service will also fail if one of the Pipeline's components fails to transition to a state of `playing`.  

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to play.

**Returns**
* `DSL_RESULT_SUCCESS` if the named Pipeline is able to successfully transition to a state of `playing`, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_play('my-pipeline')
```

<br>

### *dsl_pipeline_pause*
```C++
DslReturnType dsl_pipeline_pause(wchar_t* pipeline);
```
This service is used to pause a **non-live** Pipeline. The service will fail if the Pipeline is not currently in a `playing` state. The service will also fail if one of the Pipeline's components fails to transition to a state of `paused`. Attempts to Pause a live Pipeline (having 1 or more live sources) will fail.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to pause.

**Returns**
* `DSL_RESULT_SUCCESS` if the named Pipeline is able to successfully transition to a state of `paused`, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_pause('my-pipeline')
```

<br>

### *dsl_pipeline_stop*
```C++
DslReturnType  dsl_pipeline_stop(wchar_t* pipeline);
```
This service is used to stop a named Pipeline and return it to a state of `ready`. The service will fail if the Pipeline is not currently in a `playing` or `paused` state. The service will also fail if one of the Pipeline's components fails to transition to a state of `ready`.  All components will be unlinked on a successful transition to `stopped` so that updates to the Pipeline can be made; adding and removing components, etc.

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to stop.

**Returns**
* `DSL_RESULT_SUCCESS` if the named Pipeline is able to successfully transition to a state of `stopped`, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_stop('my-pipeline')
```

<br>

### *dsl_pipeline_main_loop_new*
```C++
DslReturnType dsl_pipeline_main_loop_new(const wchar_t* name);
```
This service creates a new main-context and main-loop for a named Pipeline. This service must be called prior to calling [dsl_pipeline_play](#dsl_pipeline_play) and [dsl_pipeline_main_loop_run](#dsl_pipeline_main_loop_run).

**Parameters**
* `name` - [in] unique name for the Pipeline to update.

**Returns**
DSL_RESULT_SUCCESS on success, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_main_loop_new('my-pipeline')
```

<br>

### *dsl_pipeline_main_loop_run*
```C++
DslReturnType dsl_pipeline_main_loop_run(const wchar_t* name);
```
This service runs and joins a Pipeline's main-loop that was previously created with a call to [dsl_pipeline_main_loop_new](#dsl_pipeline_main_loop_new).

**Note:** this call will block until [dsl_pipeline_main_loop_quit](#dsl_pipeline_main_loop_quit) is called. The function will return immediately on failure.

**Parameters**
* `name` - [in] unique name for the Pipeline to update.

**Returns**
DSL_RESULT_SUCCESS on successful return, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_main_loop_run('my-pipeline')
```

<br>

### *dsl_pipeline_main_loop_quit*
```C++
DslReturnType dsl_pipeline_main_loop_quit(const wchar_t* name);
```
This service quits a Pipeline's running main-loop, allowing the thread blocked on the service [dsl_pipeline_main_loop_run](#dsl_pipeline_main_loop_run) to return.

**Note:** this call will block until [dsl_pipeline_main_loop_quit](#dsl_pipeline_main_loop_quit) is called. The function will return immediately on failure.

**Parameters**
* `name` - [in] unique name for the Pipeline to update.

**Returns**
DSL_RESULT_SUCCESS on successful quit, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_main_loop_quit('my-pipeline')
```

<br>

### *dsl_pipeline_main_loop_delete*
```C++
DslReturnType dsl_pipeline_main_loop_delete(const wchar_t* name);
```
This service quits a Pipeline's running main-loop, allowing the thread blocked on the service [dsl_pipeline_main_loop_run](#dsl_pipeline_main_loop_run) to return.

**Note:** this call will block until [dsl_pipeline_main_loop_quit](#dsl_pipeline_main_loop_quit) is called. The function will return immediately on failure.

**Parameters**
* `name` - [in] unique name for the Pipeline to update.

**Returns**
DSL_RESULT_SUCCESS on successful deletion, one of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_main_loop_delete('my-pipeline')
```

<br>

### *dsl_pipeline_state_get*
```C++
DslReturnType dsl_pipeline_state_get(wchar_t* pipeline, uint* state);
```
This service returns the current [state]() of the named Pipeline The service fails if the named Pipeline was not found.  

**Parameters**
* `pipeline` - [in] unique name for the Pipeline to query.
* `state` - [out] the current [Pipeline State](#pipeline-states) of the named Pipeline

**Returns**
* `DSL_RESULT_SUCCESS` on successful addition. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval, state = dsl_pipeline_state_get('my-pipeline')
```

<br>


### *dsl_pipeline_list_size*
```C++
uint dsl_pipeline_list_size();
```
This service returns the size of the current list of Pipelines in memory

**Returns**
* The number of Pipelines currently in memory

**Python Example**
```Python
pipeline_count = dsl_pipeline_list_size()
```

<br>

### *dsl_pipeline_dump_to_dot*
```C++
DslReturnType dsl_pipeline_dump_to_dot(const char* pipeline, char* filename);
```
This method dumps a Pipeline's graph to a dot file. The GStreamer Pipeline will create a topology graph on each change of state to ready, playing and paused if the debug environment variable `GST_DEBUG_DUMP_DOT_DIR` is set.

GStreamer will add the `.dot` suffix and write the file to the directory specified by the environment variable. The caller of this service is responsible for providing a correctly formatted filename.

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to dump
* `filename` - [in] name of the file without extension.

**Returns**  `DSL_RESULT_SUCCESS` on successful file dump. One of the [Return Values](#return-values) defined above on failure.

**Python Example**
```Python
retval = dsl_pipeline_play('my-pipeline')
if retval != DSL_RESULT_SUCCESS:
    # handle error
   
retval = dsl_pipeline_dump_to_dot('my-pipeline', 'my-pipeline-on-playing')
```

<br>

### *dsl_pipeline_dump_to_dot_with_ts*
```C++
DslReturnType dsl_pipeline_dump_to_dot_with_ts(const char* pipeline, char* filename);
```
This method dumps a Pipeline's graph to a dot file prefixed with the current timestamp.
Except for the prefix, this method performs the identical service as
[dsl_pipeline_dump_to_dot](#dsl_pipeline_dump_to_dot).

**Parameters**
* `pipeline` - [in] unique name of the Pipeline to dump
* `filename` - [in] name of the file without extension.

**Returns**  `DSL_RESULT_SUCCESS` on successful file dump. One of the [Return Values](#return-values) defined above on failure.
<br>

---

## API Reference
* [List of all Services](/docs/api-reference-list.md)
* **Pipeline**
* [Player](/docs/api-player.md)
* [Source](/docs/api-source.md)
* [Tap](/docs/api-tap.md)
* [Dewarper](/docs/api-dewarper.md)
* [Preprocessor](/docs/api-preproc.md)
* [Inference Engine and Server](/docs/api-infer.md)
* [Tracker](/docs/api-tracker.md)
* [Segmentation Visualizer](/docs/api-segvisual.md)
* [Tiler](/docs/api-tiler.md)
* [Demuxer and Splitter Tees](/docs/api-tee.md)
* [Remuxer](/docs/api-remuxer.md)
* [On-Screen Display](/docs/api-osd.md)
* [Sink](/docs/api-sink.md)
* [Branch](/docs/api-branch.md)
* [Component](/docs/api-component.md)
* [GST Element](/docs/api-gst.md)
* [Pad Probe Handler](/docs/api-pad-probe-handler.md)
* [ODE Trigger](/docs/api-ode-trigger.md)
* [ODE Accumulator](/docs/api-ode-accumulator.md)
* [ODE Acton](/docs/api-ode-action.md)
* [ODE Area](/docs/api-ode-area.md)
* [ODE Heat-Mapper](/docs/api-ode-heat-mapper.md)
* [Display Type](/docs/api-display-type.md)
* [Mailer](/docs/api-mailer.md)
* [WebSocket Server](/docs/api-ws-server.md)
* [Message Broker](/docs/api-msg-broker.md)
* [Info API](/docs/api-info.md)
