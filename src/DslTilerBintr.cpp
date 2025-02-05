/*
The MIT License

Copyright (c) 2019-2025, Prominence AI, Inc.

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
#include "DslTilerBintr.h"
#include "DslBranchBintr.h"
#include "DslServices.h"

namespace DSL
{

    TilerBintr::TilerBintr(const char* name, uint width, uint height)
        : QBintr(name)
        , m_width(width)
        , m_height(height)
        , m_rows(0)
        , m_columns(0)
        , m_frameNumberingEnabled(false)
        , m_notifyClientsTimerId(0)
        , m_showSourceTimeout(0)
        , m_showSourceCounter(0)
        , m_showSourceTimerId(0)
        , m_showSourceCycle(false)
    {
        LOG_FUNC();

        // New Tiler element for this TilerBintr
        m_pTiler = DSL_ELEMENT_NEW("nvmultistreamtiler", name);

        // Don't overwrite the default "best-fit" columns and rows on construction
        m_pTiler->SetAttribute("width", m_width);
        m_pTiler->SetAttribute("height", m_height);
        
        // Get property defaults that aren't specifically set
        m_pTiler->GetAttribute("show-source", &m_showStreamId);
        m_pTiler->GetAttribute("gpu-id", &m_gpuId);
        m_pTiler->GetAttribute("compute-hw", &m_computeHw);
        m_pTiler->GetAttribute("nvbuf-memory-type", &m_nvbufMemType);

        LOG_INFO("");
        LOG_INFO("Initial property values for TilerBintr '" << name << "'");
        LOG_INFO("  rows                 : " << m_rows);
        LOG_INFO("  columns              : " << m_columns);
        LOG_INFO("  width                : " << m_width);
        LOG_INFO("  height               : " << m_height);
        LOG_INFO("  show-source          : " << m_showStreamId);
        LOG_INFO("  gpu-id               : " << m_gpuId);
        LOG_INFO("  nvbuf-memory-type    : " << m_nvbufMemType);
        LOG_INFO("  compute-hw           : " << m_computeHw);
        LOG_INFO("  queue                : " );
        LOG_INFO("    leaky              : " << m_leaky);
        LOG_INFO("    max-size           : ");
        LOG_INFO("      buffers          : " << m_maxSizeBuffers);
        LOG_INFO("      bytes            : " << m_maxSizeBytes);
        LOG_INFO("      time             : " << m_maxSizeTime);
        LOG_INFO("    min-threshold      : ");
        LOG_INFO("      buffers          : " << m_minThresholdBuffers);
        LOG_INFO("      bytes            : " << m_minThresholdBytes);
        LOG_INFO("      time             : " << m_minThresholdTime);

        AddChild(m_pTiler);

        // Float the queue element (from parent QBintr) as a sink-ghost-pad 
        // for this Bintr.
        m_pQueue->AddGhostPadToParent("sink");

        // Float the tiler element as a src-ghost-pad for this Bintr.
        m_pTiler->AddGhostPadToParent("src");
    
        // Add the Buffer and DS Event probes to the tiler element.
        AddSinkPadProbes(m_pTiler->GetGstElement());
        AddSrcPadProbes(m_pTiler->GetGstElement());
        
        // Create the specialized PPH which will be (optionally) used to
        // add a frame-number to each unbatched output buffer crossing the
        // Tiler's src-pad. See SetFrameNumberingEnabled() below. 
        // RE: Tiler plugin sets all frame-numbers to 0.
        std::string adderName = GetName() + "-frame-number-adder";
        m_pFrameNumberAdder = DSL_PPH_FRAME_NUMBER_ADDER_NEW(adderName.c_str());
    }

    TilerBintr::~TilerBintr()
    {
        LOG_FUNC();

        if (m_isLinked)
        {    
            UnlinkAll();
        }
        if (m_notifyClientsTimerId)
        {
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
            
            g_source_remove(m_notifyClientsTimerId);
        }
        if (m_showSourceTimerId)
        {
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
            
            g_source_remove(m_showSourceTimerId);
        }
    }

    bool TilerBintr::AddToParent(DSL_BASE_PTR pParentBintr)
    {
        LOG_FUNC();
        
        return std::dynamic_pointer_cast<BranchBintr>(pParentBintr)->
            AddTilerBintr(shared_from_this());
    }
    
    bool TilerBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("TilerBintr '" << m_name << "' is already linked");
            return false;
        }
        m_pQueue->LinkToSink(m_pTiler);
        m_isLinked = true;
        
        m_pFrameNumberAdder->ResetFrameNumber();
        
        return true;
    }
    
    void TilerBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("TilerBintr '" << m_name << "' is not linked");
            return;
        }
        m_pQueue->UnlinkFromSink();
        m_isLinked = false;
    }
    
    void TilerBintr::GetTiles(uint* columns, uint* rows)
    {
        LOG_FUNC();
        
        *columns = m_columns;
        *rows = m_rows;
    }
    
    bool TilerBintr::SetTiles(uint columns, uint rows)
    {
        LOG_FUNC();
        
        m_columns = columns;
        m_rows = rows;
    
        m_pTiler->SetAttribute("columns", columns);
        m_pTiler->SetAttribute("rows", m_rows);
        
        return true;
    }
    
    void TilerBintr::GetDimensions(uint* width, uint* height)
    {
        LOG_FUNC();
        
        m_pTiler->GetAttribute("width", &m_width);
        m_pTiler->GetAttribute("height", &m_height);
        
        *width = m_width;
        *height = m_height;
    }

    bool TilerBintr::SetDimensions(uint width, uint height)
    {
        LOG_FUNC();

        m_width = width;
        m_height = height;

        m_pTiler->SetAttribute("width", m_width);
        m_pTiler->SetAttribute("height", m_height);
        
        return true;
    }

    bool TilerBintr::GetFrameNumberingEnabled()
    {
        LOG_FUNC();
        
        return m_frameNumberingEnabled;
    }

    bool TilerBintr::SetFrameNumberingEnabled(bool enabled)
    {
        LOG_FUNC();
        
        if (m_frameNumberingEnabled and enabled)
        {
            LOG_ERROR("Can't enable frame-numbering for Tiler '" <<
                GetName() << "' as it's already enabled ");
            return false;
        }
        if (!m_frameNumberingEnabled and !enabled)
        {
            LOG_ERROR("Can't disabled frame-numbering for Tiler '" <<
                GetName() << "' as it's already disabled ");
            return false; 
        }
        m_frameNumberingEnabled = enabled;
        
        if(m_frameNumberingEnabled)
        {
            return AddPadProbeBufferHandler(m_pFrameNumberAdder, DSL_PAD_SRC);
        }
        return RemovePadProbeBufferHandler(m_pFrameNumberAdder, DSL_PAD_SRC);
    }
    

    void TilerBintr::GetShowSource(int* streamId, uint* timeout)
    {
        LOG_FUNC();
        
        *streamId = m_showStreamId;
        *timeout = m_showSourceTimeout;
    }

    bool TilerBintr::SetShowSource(int streamId, uint timeout, bool hasPrecedence)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);

        if (streamId < 0)
        {
            LOG_ERROR("Invalid source Id '" << streamId << "' for TilerBintr '" << GetName());
            return false;
        }

        // call has Precendence over source cycling 
        m_showSourceCycle = false;
        if (streamId != m_showStreamId)
        {
            if (m_showSourceTimerId and !hasPrecedence)
            {
                // don't log error as this may be common with ODE Triggers and Actions calling
                LOG_DEBUG("Show source Timer is running for Source '" << m_showStreamId << 
                   "' New Source '" << streamId << "' without precedence can not be shown");
                return false;
            }

            m_showSourceTimeout = timeout;

            if (!SetShowSource(streamId))
            {
                return false;
            }

            m_showSourceCounter = m_showSourceTimeout*10;
            if (m_showSourceCounter)
            {
                LOG_INFO("Adding show-source timer with timeout = " 
                    << timeout << "' for TilerBintr '" << GetName());
                m_showSourceTimerId = g_timeout_add(100, ShowSourceTimerHandler, this);
            }
            return true;
        }
            
        // otherwise it's the same source.
        
        m_showSourceTimeout = timeout;
        m_showSourceCounter = timeout*10;
        if (!m_showSourceTimerId and m_showSourceCounter)
        {
            LOG_INFO("Adding show-source timer with timeout = " << timeout << "' for TilerBintr '" << GetName());
            m_showSourceTimerId = g_timeout_add(100, ShowSourceTimerHandler, this);
        }
        return true;
    }
    
    bool TilerBintr::CycleAllSources(uint timeout)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
        
        if (!timeout)
        {
            LOG_ERROR("Timeout value can not be 0 when enabling cycle-all-sources for TilerBintr '" << GetName());
            return false;
        }
        // if the timer is currently running, stop and remove first.
        if (m_showSourceTimerId)
        {
            g_source_remove(m_showSourceTimerId);
            m_showSourceTimerId = 0;
        }

        m_showSourceCycle = true;
        m_showSourceTimeout = timeout;

        // Start with stream 0
        SetShowSource(0);

        m_showSourceCounter = m_showSourceTimeout*10;
            
        if (!m_showSourceTimerId and m_showSourceCounter)
        {
            LOG_INFO("Adding show-source timer with timeout = " << timeout << "' for TilerBintr '" << GetName());
            m_showSourceTimerId = g_timeout_add(100, ShowSourceTimerHandler, this);
        }
        return true;
    }
    
    
    void TilerBintr::ShowAllSources()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
        
        if (m_showSourceTimerId)
        {
            g_source_remove(m_showSourceTimerId);
            m_showSourceTimerId = 0;
            // call has Precendence over source cycling 
            m_showSourceCycle = false;
        }
        if (m_showStreamId != -1)
        {
            SetShowSource(-1);
        }
    }

    int TilerBintr::HandleShowSourceTimer()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
        
        // Tiler is no longer linked but the main_loop and timer are still running
        if (!IsLinked())
        {
            // do nothing, but keep the timer running to support relink and play
            // The Timer's Cycle Source setting should remain as is.
            return true;
        }
        if (--m_showSourceCounter == 0)
        {
            // if we are cycling through sources
            if (m_showSourceCycle)
            {
                // reset the timeout counter, cycle to the next source, and return true to continue
                m_showSourceCounter = m_showSourceTimeout*10;
                int newStreamId((m_showStreamId+1)%m_batchSize);
                SetShowSource(newStreamId);
                return true;
            }
            // otherwise, reset the timer Id, show all sources, and return false to destroy the timer
            m_showSourceTimerId = 0;
            SetShowSource(-1);
            return false;
        }
        return true;
    }


    bool TilerBintr::SetShowSource(int streamId)
    {
        LOG_FUNC();
        
        // clear the source name first
        m_wstrSourceName = L"";

        // if showing a specific source, and not all sources.
        if (streamId != -1)
        {
            uint sourceId = 
                (m_pipelineId << DSL_PIPELINE_SOURCE_UNIQUE_ID_OFFSET_IN_BITS) 
                | (uint)streamId;

            const char* cSourceName;          
      
            // check for a valid source name, otherwise, it's an inactive tile. 
            if (Services::GetServices()->_sourceNameGet(sourceId, &cSourceName)
                == DSL_RESULT_SUCCESS)
            {
                // convert the source name to wchar to send to the client.
                std::string cstrSource(cSourceName);
                m_wstrSourceName.assign(cstrSource.begin(), cstrSource.end());
            }
            else
            {
                LOG_ERROR("Inactive stream = " << streamId 
                    << " selected for Tiler '" << GetName() << "'");
                return false;
            }
        }
        // ok to set the element property now
        m_showStreamId = streamId;
        m_pTiler->SetAttribute("show-source", m_showStreamId);

        if (m_showSourceListeners.size() and !m_notifyClientsTimerId)
        {
            m_notifyClientsTimerId = g_timeout_add(1, NotifyClientsTimerHandler, this);
        }

        return true;
    }

    int TilerBintr::HandleNotifiyClients()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);

        // iterate through the map of listeners calling each
        for(auto const& imap: m_showSourceListeners)
        {
            try
            {
                imap.first(GetWStrName().c_str(), 
                    m_wstrSourceName.c_str(), m_showStreamId, imap.second);
            }
            catch(...)
            {
                LOG_ERROR("Exception calling Client Show-Source-Lister");
            }
        }
        // Clear the timer resource id and return false to destroy the timer.
        m_notifyClientsTimerId = 0;
        return false;
    }

    bool TilerBintr::AddShowSourceListener(
        dsl_tiler_source_show_listener_cb listener, void* clientData)
    {
        LOG_FUNC();

        if (m_showSourceListeners.find(listener) != m_showSourceListeners.end())
        {   
            LOG_ERROR("Show Source listener is not unique");
            return false;
        }
        m_showSourceListeners[listener] = clientData;
        
        return true;
    }

    bool TilerBintr::RemoveShowSourceListener(
        dsl_tiler_source_show_listener_cb listener)
    {
        LOG_FUNC();
        
        if (m_showSourceListeners.find(listener) == m_showSourceListeners.end())
        {   
            LOG_ERROR("Show Source listener was not found");
            return false;
        }
        m_showSourceListeners.erase(listener);
        
        return true;
    }

    bool TilerBintr::SetGpuId(uint gpuId)
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("Unable to set GPU ID for TilerBintr '" << GetName() 
                << "' as it's currently in use");
            return false;
        }

        m_gpuId = gpuId;
        m_pTiler->SetAttribute("gpu-id", m_gpuId);
        
        LOG_INFO("TilerBintr '" << GetName() 
            << "' - new GPU ID = " << m_gpuId );
        
        return true;
    }

    bool TilerBintr::SetNvbufMemType(uint nvbufMemType)
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("Unable to set NVIDIA buffer memory type for TilerBintr '" 
                << GetName() << "' as it's currently linked");
            return false;
        }
        m_nvbufMemType = nvbufMemType;
        m_pTiler->SetAttribute("nvbuf-memory-type", m_nvbufMemType);

        return true;
    }

    //----------------------------------------------------------------------------------------------
    
    static int ShowSourceTimerHandler(void* user_data)
    {
        return static_cast<TilerBintr*>(user_data)->
            HandleShowSourceTimer();
    }

    static int NotifyClientsTimerHandler(void* user_data)
    {
        return static_cast<TilerBintr*>(user_data)->
            HandleNotifiyClients();
    }

}