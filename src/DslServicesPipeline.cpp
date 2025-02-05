/*
The MIT License

Copyright (c)   2021-2025, Prominence AI, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in-
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"
#include "DslPipelineBintr.h"

namespace DSL
{
    DslReturnType Services::PipelineNew(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            if (m_pipelines[name])
            {   
                LOG_ERROR("Pipeline name '" << name << "' is not unique");
                return DSL_RESULT_PIPELINE_NAME_NOT_UNIQUE;
            }
            
            m_pipelines[name] = std::shared_ptr<PipelineBintr>(new PipelineBintr(name));
            LOG_INFO("New PIPELINE '" << name << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("New Pipeline '" << name << "' threw exception on create");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineDelete(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        try
        {
            
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            m_pipelines[name]->RemoveAllChildren();
            m_pipelines.erase(name);

            LOG_INFO("Pipeline '" << name << "' deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name << "' threw an exception on Delete");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
            
    }

    DslReturnType Services::PipelineDeleteAll()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            for (auto &imap: m_pipelines)
            {
                imap.second->RemoveAllChildren();
                imap.second = nullptr;
            }
            m_pipelines.clear();

            LOG_INFO("All Pipelines deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("DSL threw an exception on PipelineDeleteAll");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    uint Services::PipelineListSize()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        return m_pipelines.size();
    }
    
    DslReturnType Services::PipelineComponentAdd(const char* name, 
        const char* component)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, component);
            
            // Can't add components if they're In use by another Pipeline
            if (m_components[component]->IsInUse())
            {
                LOG_ERROR("Unable to add component '" << component 
                    << "' as it's currently in use");
                return DSL_RESULT_COMPONENT_IN_USE;
            }

            if (!m_components[component]->AddToParent(m_pipelines[name]))
            {
                LOG_ERROR("Pipeline '" << name
                    << "' failed component '" << component << "'");
                return DSL_RESULT_PIPELINE_COMPONENT_ADD_FAILED;
            }
            LOG_INFO("Component '" << component 
                << "' was added to Pipeline '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw exception adding component '" << component << "'");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }    
    
    DslReturnType Services::PipelineComponentRemove(const char* name, 
        const char* component)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, component);

            if (!m_components[component]->IsParent(m_pipelines[name]))
            {
                LOG_ERROR("Component '" << component << 
                    "' is not in use by Pipeline '" << name << "'");
                return DSL_RESULT_COMPONENT_NOT_USED_BY_PIPELINE;
            }
            m_components[component]->RemoveFromParent(m_pipelines[name]);

            LOG_INFO("Component '" << component 
                << "' was removed from Pipeline '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing component");
            return DSL_RESULT_PIPELINE_COMPONENT_REMOVE_FAILED;
        }
    }
    
    //----------------------------------------------------------------------------
    // NEW STREAMMUX SERVICES - Start
    //----------------------------------------------------------------------------
    
    DslReturnType Services::PipelineStreammuxConfigFileGet(const char* name, 
        streammux_type streammux, const char** configFile)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            *configFile = m_pipelines[name]->GetStreammuxConfigFile(streammux);

            LOG_INFO("Pipeline '" << name << "' returned Streammux config-file = '"
                << *configFile << "' successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw exception getting the Streammux config-file");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxConfigFileSet(const char* name, 
        streammux_type streammux, const char* configFile)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            std::ifstream streamConfigFile(configFile);
            if (!streamConfigFile.good())
            {
                LOG_ERROR("Pipeline Streammux config file not found");
                return DSL_RESULT_PIPELINE_STREAMMUX_CONFIG_FILE_NOT_FOUND;
            }
            
            if (!m_pipelines[name]->SetStreammuxConfigFile(streammux, configFile))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to set the Streammux config-file");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set Streammux config-file = '"
                << configFile << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Dewarper '" << name 
                << "' threw exception setting config-file");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxBatchSizeGet(const char* name,
        streammux_type streammux, uint* batchSize)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *batchSize = m_pipelines[name]->GetStreammuxBatchSize(streammux);
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Streammuxe batch-size = " 
                << *batchSize << "' successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception getting the Streammux batch-size");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxBatchSizeSet(const char* name,
        streammux_type streammux, uint batchSize)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxBatchSize(streammux, batchSize))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to set Streammux batch-size = "
                    << batchSize );
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set batch-size = " 
                << batchSize << "' successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting the Streammux batch-size");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    //----------------------------------------------------------------------------
    // COMMON STREAMMUX SERVICES - End
    //----------------------------------------------------------------------------

    DslReturnType Services::PipelineStreammuxEnabledGet(const char* name,
        streammux_type streammux, boolean* enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *enabled = m_pipelines[name]->GetStreammuxEnabled(streammux);
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Streammuxer enabled = " 
                << *enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Streammux enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxEnabledSet(const char* name,
        streammux_type streammux, boolean enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxEnabled(streammux, enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Streammux enabled setting");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' set the Streammuxer enabled setting = " 
                << enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting Streammux enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxNumSurfacesPerFrameGet(const char* name,
        streammux_type streammux, uint* num)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *num = m_pipelines[name]->GetStreammuxNumSurfacesPerFrame(streammux);

            LOG_INFO("Pipeline '" << name 
                << "' returned Streammux num-surfaces-per-frame = " << *num 
                << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Streammux num-surfaces-per-frame");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxNumSurfacesPerFrameSet(const char* name,
        streammux_type streammux, uint num)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (num > 4)
            {
                LOG_ERROR("The value of '" << num 
                    << "' is invalid for Streammux num-surfaces-per-frame setting");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            
            if (!m_pipelines[name]->SetStreammuxNumSurfacesPerFrame(streammux, num))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to set the Streammux num-surfaces-per-frame setting = "
                    << num);
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set the num-surfaces-per-frame setting = " 
                << num << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting Streammux num-surfaces-per-frame");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxAttachSysTsEnabledGet(const char* name,
        streammux_type streammux, boolean* enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *enabled = m_pipelines[name]->GetStreammuxAttachSysTsEnabled(streammux);
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Streammuxer attach-sys-inputs enabled = " 
                << *enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Streammux attach-sys-inputs enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxAttachSysTsEnabledSet(const char* name,
        streammux_type streammux, boolean enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxAttachSysTsEnabled(streammux, enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Streammux attach-sys-inputs enabled setting");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' set the Streammuxer attach-sys-inputs enabled setting = " 
                << enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting Streammux attach-sys-inputs enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxSyncInputsEnabledGet(const char* name,
        streammux_type streammux, boolean* enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *enabled = m_pipelines[name]->GetStreammuxSyncInputsEnabled(streammux);
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Streammuxer sync-inputs enabled = " 
                << *enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Streammux sync-inputs enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxSyncInputsEnabledSet(const char* name,
        streammux_type streammux, boolean enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxSyncInputsEnabled(streammux, enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Streammux sync-inputs enabled setting");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' set the Streammuxer sync-inputs enabled setting = " 
                << enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting Streammux sync-inputs enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxMaxLatencyGet(const char* name, 
        streammux_type streammux, uint* maxLatency)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *maxLatency = m_pipelines[name]->GetStreammuxMaxLatency(streammux);

            LOG_INFO("Pipeline '" << name 
                << "' returned Streammuxer max-latency = " 
                << *maxLatency << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw exception getting max-latency");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStreammuxMaxLatencySet(const char* name, 
        streammux_type streammux, uint maxLatency)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxMaxLatency(streammux, maxLatency))
            {
                LOG_INFO("Pipeline '" << name 
                    << "' faild to set Streammuxer max-latency = " 
                    << maxLatency);
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }

            LOG_INFO("Pipeline '" << name 
                << "' set the Streammuxer max-latency setting = " 
                << maxLatency << "' successfully");

            return DSL_RESULT_SUCCESS;
            }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw exception setting GPU Id");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }

    //----------------------------------------------------------------------------
    // OLD STREAMMUX SERVICES - Start
    //----------------------------------------------------------------------------

    DslReturnType Services::PipelineStreammuxBatchPropertiesGet(const char* name,
        uint* batchSize, int* batchTimeout)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            m_pipelines[name]->GetStreammuxBatchProperties(batchSize, batchTimeout);
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Streammux batch-size = " 
                << *batchSize << " and batch-timeout = " 
                << *batchTimeout << "' successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception getting the Streammux batch properties");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxBatchPropertiesSet(const char* name,
        uint batchSize, int batchTimeout)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxBatchProperties(batchSize, batchTimeout))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to set Streammux batch-size = "
                    << batchSize << " and batch-timeout = "
                    << batchTimeout);
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set batch-size = " 
                << batchSize << " and batch-timeout = " 
                << batchTimeout << "' successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting the Streammux batch properties");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStreammuxNvbufMemTypeGet(const char* name, 
        uint* type)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *type = m_pipelines[name]->GetStreammuxNvbufMemType();
            
            LOG_INFO("Pipeline '" << name << "' returned nvbuf memory type = " 
                << *type << " successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception getting the Streammux nvbuf memory type");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxNvbufMemTypeSet(const char* name, 
        uint type)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (type > DSL_NVBUF_MEM_TYPE_SURFACE_ARRAY)
            {
                LOG_INFO("Invalid nvbuf memory type " << type <<
                    " for Pipeline '" << name <<"'");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            if (!m_pipelines[name]->SetStreammuxNvbufMemType(type))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Streammux nvbuf memory type = "
                    << type);
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set nvbuf memeory type = " 
                << type << "  successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting the Streammux nvbuf memory type");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxGpuIdGet(const char* name, uint* gpuid)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *gpuid = m_pipelines[name]->GetGpuId();

            LOG_INFO("Current GPU ID = " << *gpuid 
                << " for Pipeline '" << name << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw exception getting GPU ID");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStreammuxGpuIdSet(const char* name, uint gpuid)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetGpuId(gpuid))
            {
                LOG_INFO("Pipeline '" << name 
                    << "' faild to set GPU ID = " << gpuid);
                return DSL_RESULT_COMPONENT_SET_GPUID_FAILED;
            }

            LOG_INFO("New GPU ID = " << gpuid 
                << " for Pipeline '" << name << "'");

            return DSL_RESULT_SUCCESS;
            }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw exception setting GPU Id");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStreammuxDimensionsGet(const char* name,
        uint* width, uint* height)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            m_pipelines[name]->GetStreammuxDimensions(width, height);
            
            LOG_INFO("Pipeline '" << name << "' returned Streammux width = " 
                << *width << " and  height = " << *height << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting the Streammux output dimensions");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxDimensionsSet(const char* name,
        uint width, uint height)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxDimensions(width, height))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Streammux output dimensions");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set Streammux width = " 
                << width << " and Streammux height = " << height << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting the Streammux output dimensions");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxPaddingGet(const char* name,
        boolean* enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *enabled = m_pipelines[name]->GetStreammuxPadding();

            LOG_INFO("Pipeline '" << name << "' returned padding Enabled = " 
                << *enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Streammux padding enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStreammuxPaddingSet(const char* name,
        boolean enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetStreammuxPadding(enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Streammux padding enabled setting");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' set Streammux padding enabled = " 
                << enabled << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting Streammux padding enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    //----------------------------------------------------------------------------
    // OLD STREAMMUX SERVICES - End
    //----------------------------------------------------------------------------

    DslReturnType Services::PipelineStreammuxTilerAdd(const char* name,
        const char* tiler)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, tiler);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, tiler, TilerBintr)
            
            if (!m_pipelines[name]->AddStreammuxTiler(m_components[tiler]))
            {
                LOG_ERROR("Pipeline '" << name << "' failed to add Tiler '" 
                    << tiler << "' to the Streammux's output");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name << "' added Tiler '" 
                << tiler << "' to the Streammux's output successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception adding a Tiler to Streammux's output");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStreammuxTilerRemove(const char* name)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->RemoveStreammuxTiler())
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to remove a Tiler from the Streammux's output");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' removed Tiler from to the Streammux's output successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing a Tiler from Streammux's output");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineStreammuxPphAdd(const char* name, 
        streammux_type streammux, const char* handler)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_PPH_NAME_NOT_FOUND(m_padProbeHandlers, handler);

            // call on the Handler to add itself to the Tee as a PadProbeHandler
            if (!m_pipelines[name]->AddStreammuxPph(m_padProbeHandlers[handler],
                streammux))   
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to add Pad Probe Handler");
                return DSL_RESULT_PIPELINE_STREAMMUX_HANDLER_ADD_FAILED;
            }
            LOG_INFO("Pad Probe Handler '" << handler 
                << "' added to Pipeline '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception adding Pad Probe Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
   
    DslReturnType Services::PipelineStreammuxPphRemove(const char* name, 
        streammux_type streammux, const char* handler) 
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_PPH_NAME_NOT_FOUND(m_padProbeHandlers, handler);

            // call on the Handler to remove itself from the Tee
            if (!m_pipelines[name]->RemoveStreammuxPph(m_padProbeHandlers[handler],
                streammux))
            {
                LOG_ERROR("Pad Probe Handler '" << handler 
                    << "' is not a child of Pipeline '" << name << "'");
                return DSL_RESULT_PIPELINE_STREAMMUX_HANDLER_REMOVE_FAILED;
            }
            LOG_INFO("Pad Probe Handler '" << handler 
                << "' removed from Pipeline '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing Pad Probe Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
            
    DslReturnType Services::PipelineAudiomixEnabledGet(const char* name,
        boolean* enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *enabled = m_pipelines[name]->GetAudiomixEnabled();
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Audiomixer enabled = " 
                << *enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Audiomixer enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineAudiomixEnabledSet(const char* name,
        boolean enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->SetAudiomixEnabled(enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Audiomixer enabled setting");
                return DSL_RESULT_PIPELINE_STREAMMUX_SET_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' set the Audiomixer enabled setting = " 
                << enabled << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting Audiomixer enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineAudiomixMuteEnabledGet(const char* name,
        const char* source, boolean* enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, source);
            DSL_RETURN_IF_COMPONENT_IS_NOT_AUDIO_SOURCE(m_components, source);
            
            DSL_AUDIO_SOURCE_PTR pAudioSource = 
                std::dynamic_pointer_cast<AudioSourceBintr>(m_components[source]);

            if (!m_pipelines[name]->GetAudiomixMuteEnabled(
                pAudioSource, enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to get the Audiomixer mute enabled setting");
                return DSL_RESULT_PIPELINE_AUDIOMIX_GET_FAILED;
            }
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Audiomixer mute enabled = " 
                << *enabled << "' for Audio Source '"
                << pAudioSource->GetName() << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Audiomixer enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineAudiomixMuteEnabledSet(const char* name,
        const char* source, boolean enabled)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, source);
            DSL_RETURN_IF_COMPONENT_IS_NOT_AUDIO_SOURCE(m_components, source);
            
            DSL_AUDIO_SOURCE_PTR pAudioSource = 
                std::dynamic_pointer_cast<AudioSourceBintr>(m_components[source]);

            if (!m_pipelines[name]->SetAudiomixMuteEnabled(
                pAudioSource, enabled))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to set the Audiomixer mute enabled setting");
                return DSL_RESULT_PIPELINE_AUDIOMIX_SET_FAILED;
            }
            
            LOG_INFO("Pipeline '" << name 
                << "' set Audiomixer mute enabled = " 
                << enabled << "' for Audio Source '"
                << pAudioSource->GetName() << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Audiomixer mute enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineAudiomixVolumeGet(const char* name,
        const char* source, double* volume)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, source);
            DSL_RETURN_IF_COMPONENT_IS_NOT_AUDIO_SOURCE(m_components, source);
            
            DSL_AUDIO_SOURCE_PTR pAudioSource = 
                std::dynamic_pointer_cast<AudioSourceBintr>(m_components[source]);

            if (!m_pipelines[name]->GetAudiomixVolume(
                pAudioSource, volume))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to get the Audiomixer volume setting");
                return DSL_RESULT_PIPELINE_AUDIOMIX_GET_FAILED;
            }
            
            LOG_INFO("Pipeline '" << name 
                << "' returned Audiomixer volume = " 
                << *volume << "' for Audio Source '"
                << pAudioSource->GetName() << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Audiomixer volume");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineAudiomixVolumeSet(const char* name,
        const char* source, double volume)    
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, source);
            DSL_RETURN_IF_COMPONENT_IS_NOT_AUDIO_SOURCE(m_components, source);
            
            DSL_AUDIO_SOURCE_PTR pAudioSource = 
                std::dynamic_pointer_cast<AudioSourceBintr>(m_components[source]);

            if (volume < 0.0 or volume > 10.0)
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' cannot Set the Audiomixer volume to " 
                    << volume << " - out of range!");
                return DSL_RESULT_PIPELINE_AUDIOMIX_SET_FAILED;
            }
            if (!m_pipelines[name]->SetAudiomixVolume(
                pAudioSource, volume))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to Set the Audiomixer volume setting");
                return DSL_RESULT_PIPELINE_AUDIOMIX_SET_FAILED;
            }
            
            LOG_INFO("Pipeline '" << name 
                << "' set Audiomixer volume = " 
                << volume << "' for Audio Source '"
                << pAudioSource->GetName() << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name
                << "' threw an exception getting Audiomixer enabled");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineLinkMethodGet(const char* name, uint* linkMethod)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *linkMethod = m_pipelines[name]->GetLinkMethod();

            LOG_INFO("Pipeline '" << name 
                << "' returned link method = " << *linkMethod << " successfully");
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception getting link method");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineLinkMethodSet(const char* name, uint linkMethod)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (linkMethod > DSL_PIPELINE_LINK_METHOD_BY_ADD_ORDER)
            {
                LOG_ERROR("Invalid link method = " << linkMethod 
                    << " for Pipeline '" << name << "'");
                return DSL_RESULT_PIPELINE_SET_FAILED;
            }
            m_pipelines[name]->SetLinkMethod(linkMethod);

            LOG_INFO("Pipeline '" << name 
                << "' set link method = " << linkMethod << " successfully");
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception setting link method");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelinePause(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!std::dynamic_pointer_cast<PipelineBintr>(m_pipelines[name])->Pause())
            {
                return DSL_RESULT_PIPELINE_FAILED_TO_PAUSE;
            }

            LOG_INFO("Player '" << name 
                << "' transitioned to a state of PAUSED successfully");
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception on Pause");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelinePlay(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!std::dynamic_pointer_cast<PipelineBintr>(m_pipelines[name])->Play())
            {
                return DSL_RESULT_PIPELINE_FAILED_TO_PLAY;
            }

            LOG_INFO("Pipeline '" << name 
                << "' transitioned to a state of PLAYING successfully");
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception on Play");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStop(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!std::dynamic_pointer_cast<PipelineBintr>(m_pipelines[name])->Stop())
            {
                return DSL_RESULT_PIPELINE_FAILED_TO_STOP;
            }

            LOG_INFO("Pipeline '" << name 
                << "' transitioned to a state of NULL successfully");
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception on Play");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineStateGet(const char* name, uint* state)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            GstState gstState;
            std::dynamic_pointer_cast<PipelineBintr>(m_pipelines[name])->GetState(gstState, 0);
            *state = (uint)gstState;

            LOG_INFO("Pipeline '" << name 
                << "' returned a current state of '" << *state << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception getting state");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineIsLive(const char* name, boolean* isLive)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            *isLive = std::dynamic_pointer_cast<PipelineBintr>(m_pipelines[name])->IsLive();

            LOG_INFO("Pipeline '" << name 
                << "' returned is-live = " << *isLive << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception getting 'is-live'");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineDumpToDot(const char* name, 
        const char* filename)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

        // TODO check state of debug env var and return NON-success if not set

        m_pipelines[name]->DumpToDot(const_cast<char*>(filename));
        
        return DSL_RESULT_SUCCESS;
    }   
    
    DslReturnType Services::PipelineDumpToDotWithTs(const char* name, 
        const char* filename)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

        // TODO check state of debug env var and return NON-success if not set

        m_pipelines[name]->DumpToDot(const_cast<char*>(filename));

        return DSL_RESULT_SUCCESS;
    }

    DslReturnType Services::PipelineStateChangeListenerAdd(const char* name, 
        dsl_state_change_listener_cb listener, void* clientData)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->AddStateChangeListener(listener, clientData))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to add a State Change Listener");
                return DSL_RESULT_PIPELINE_CALLBACK_ADD_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' added State Change Listener successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception adding a State Change Lister");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineStateChangeListenerRemove(const char* name, 
        dsl_state_change_listener_cb listener)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
    
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->RemoveStateChangeListener(listener))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to remove a State Change Listener");
                return DSL_RESULT_PIPELINE_CALLBACK_REMOVE_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' removed State Change Listener successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing a State Change Lister");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineEosListenerAdd(const char* name, 
        dsl_eos_listener_cb listener, void* clientData)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->AddEosListener(listener, clientData))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to add an EOS Listener");
                return DSL_RESULT_PIPELINE_CALLBACK_ADD_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' added End of Stream Listener successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception adding an EOS Lister");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineEosListenerRemove(const char* name, 
        dsl_eos_listener_cb listener)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
    
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->RemoveEosListener(listener))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to remove an EOS Listener");
                return DSL_RESULT_PIPELINE_CALLBACK_REMOVE_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' removed End of Stream Listener successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing an EOS Listener");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineErrorMessageHandlerAdd(const char* name, 
        dsl_error_message_handler_cb handler, void* clientData)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->AddErrorMessageHandler(handler, clientData))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to add an Error Message Handler");
                return DSL_RESULT_PIPELINE_CALLBACK_ADD_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' added Error Message Handler successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception adding an Error Message Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineErrorMessageHandlerRemove(const char* name, 
        dsl_error_message_handler_cb handler)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
    
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            if (!m_pipelines[name]->RemoveErrorMessageHandler(handler))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to remove an Error Message Handler");
                return DSL_RESULT_PIPELINE_CALLBACK_REMOVE_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' added a Error Message Handler successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing an Error Message Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineErrorMessageLastGet(const char* name,
        std::wstring& source, std::wstring& message)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
    
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            m_pipelines[name]->GetLastErrorMessage(source, message);
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing an Error Message Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineBufferingMessageHandlerAdd(const char* name, 
        dsl_buffering_message_handler_cb handler, void* clientData)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);
            
            if (!m_pipelines[name]->AddBufferingMessageHandler(handler, clientData))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to add a Buffering Message Handler");
                return DSL_RESULT_PIPELINE_CALLBACK_ADD_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' added Buffering Message Handler successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception adding an Buffering Message Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
        
    DslReturnType Services::PipelineBufferingMessageHandlerRemove(const char* name, 
        dsl_buffering_message_handler_cb handler)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
    
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            if (!m_pipelines[name]->RemoveBufferingMessageHandler(handler))
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to remove a Buffering Message Handler");
                return DSL_RESULT_PIPELINE_CALLBACK_REMOVE_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' added a Buffering Message Handler successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception removing an Buffering Message Handler");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineMainLoopNew(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            if (!m_pipelines[name]->NewMainLoop())
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to create a new Main-Loop");
                return DSL_RESULT_PIPELINE_MAIN_LOOP_REQUEST_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' created its own Main-Loop successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception creating Main-Loop");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::PipelineMainLoopRun(const char* name)
    {
        LOG_FUNC();
        
        // Note: do not lock mutex - blocking call
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            if (!m_pipelines[name]->RunMainLoop())
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to run its own Main-Loop");
                return DSL_RESULT_PIPELINE_MAIN_LOOP_REQUEST_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' returned from running its own Main-Loop successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception running its own Main-Loop");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PipelineMainLoopQuit(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            if (!m_pipelines[name]->QuitMainLoop())
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to quit running its own Main-Loop");
                return DSL_RESULT_PIPELINE_MAIN_LOOP_REQUEST_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' quit running its own Main-Loop successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception quiting its own Main-Loop");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }


    DslReturnType Services::PipelineMainLoopDelete(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PIPELINE_NAME_NOT_FOUND(m_pipelines, name);

            if (!m_pipelines[name]->DeleteMainLoop())
            {
                LOG_ERROR("Pipeline '" << name 
                    << "' failed to delete its own Main-Loop");
                return DSL_RESULT_PIPELINE_MAIN_LOOP_REQUEST_FAILED;
            }
            LOG_INFO("Pipeline '" << name 
                << "' deleted its own Main-Loop successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Pipeline '" << name 
                << "' threw an exception deleting its own Main-Loop");
            return DSL_RESULT_PIPELINE_THREW_EXCEPTION;
        }
    }
    
}