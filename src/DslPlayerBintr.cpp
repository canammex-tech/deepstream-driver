/*
The MIT License

Copyright (c) 2019-2021, Prominence AI, Inc.


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
#include "DslServices.h"
#include "DslPlayerBintr.h"
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

namespace DSL
{
    PlayerBintr::PlayerBintr(const char* name, 
        DSL_SOURCE_PTR pSource, DSL_SINK_PTR pSink)
        : Bintr(name, true) // Pipeline = true
        , PipelineStateMgr(m_pGstObj)
        , PipelineXWinMgr(m_pGstObj)
        , m_pSource(pSource)
        , m_pSink(pSink)
    {
        LOG_FUNC();

        g_mutex_init(&m_asyncCommMutex);
        
        if (!AddChild(m_pSource))
        {
            LOG_ERROR("Failed to add SourceBintr '" << m_pSource->GetName() 
                << "' to PlayerBintr '" << GetName() << "'");
            throw;
        }
        if (!AddChild(m_pSink))
        {
            LOG_ERROR("Failed to add SinkBintr '" << m_pSink->GetName() 
                << "' to PlayerBintr '" << GetName() << "'");
            throw;
        }
        
        AddEosListener(PlayerTerminate, this);
        AddXWindowDeleteEventHandler(PlayerTerminate, this);
    }

    PlayerBintr::PlayerBintr(const char* name)
        : Bintr(name, true) // Pipeline = true
        , PipelineStateMgr(m_pGstObj)
        , PipelineXWinMgr(m_pGstObj)
    {
        LOG_FUNC();

        g_mutex_init(&m_asyncCommMutex);
        
        AddEosListener(PlayerTerminate, this);
        AddXWindowDeleteEventHandler(PlayerTerminate, this);
    }

    PlayerBintr::~PlayerBintr()
    {
        LOG_FUNC();
        Stop();
        RemoveEosListener(PlayerTerminate);
        RemoveXWindowDeleteEventHandler(PlayerTerminate);
        g_mutex_clear(&m_asyncCommMutex);
    }

    bool PlayerBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_pSource == nullptr or m_pSink == nullptr)
        {
            LOG_ERROR("PlayerBintr '" << GetName() << "' missing required components");
            return false;
        }
        if (m_isLinked)
        {
            LOG_ERROR("PlayerBintr '" << GetName() << "' is already linked");
            return false;
        }
        if (!m_pSource->LinkAll() or ! m_pSink->LinkAll() or 
            !m_pSource->LinkToSink(m_pSink))
        {
            LOG_ERROR("Failed link SourceBintr '" << m_pSource->GetName() 
                << "' to SinkBintr '" << m_pSink->GetName() << "'");
            return false;
        }
        m_isLinked = true;
        return true;
    }

    void PlayerBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (m_pSource == nullptr or m_pSink == nullptr)
        {
            LOG_ERROR("PlayerBintr '" << GetName() << "' missing required components");
            return;
        }
        if (!m_isLinked)
        {
            LOG_ERROR("PlayerBintr '" << GetName() << "' is not linked");
            return;
        }
        if (!m_pSource->UnlinkFromSink())
        {
            LOG_ERROR("Failed unlink SourceBintr '" << m_pSource->GetName() 
                << "' to SinkBintr '" << m_pSink->GetName() << "'");
            return;
        }
        m_pSource->UnlinkAll();
        m_pSink->UnlinkAll();
        m_isLinked = false;
    }
    
    bool PlayerBintr::Play()
    {
        LOG_FUNC();

        GstState currentState;
        GetState(currentState, 0);
        if (currentState == GST_STATE_NULL)
        {
            if (!LinkAll())
            {
                LOG_ERROR("Unable to prepare Pipeline '" << GetName() << "' for Play");
                return false;
            }
            if (!SetState(GST_STATE_PAUSED, DSL_DEFAULT_STATE_CHANGE_TIMEOUT_IN_SEC * GST_SECOND))
            {
                LOG_ERROR("Failed to Pause before playing Pipeline '" << GetName() << "'");
                return false;
            }

        }
        // Call the base class to complete the Play process
        return SetState(GST_STATE_PLAYING, DSL_DEFAULT_STATE_CHANGE_TIMEOUT_IN_SEC * GST_SECOND);
    }
    
    bool PlayerBintr::Pause()
    {
        LOG_FUNC();
        
        GstState state;
        GetState(state, 0);
        if (state != GST_STATE_PLAYING)
        {
            LOG_WARN("Player '" << GetName() << "' is not in a state of Playing");
            return false;
        }
        // If the main loop is running -- normal case -- then we can't change the 
        // state of the Pipeline in the Application's context. 
        if (g_main_loop_is_running(DSL::Services::GetServices()->GetMainLoopHandle()))
        {
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_asyncCommMutex);
            g_timeout_add(1, PlayerPause, this);
            g_cond_wait(&m_asyncCondition, &m_asyncCommMutex);
        }
        // Else, we are running under test without the mainloop
        else
        {
            HandlePause();
        }
        return true;
    }

    void PlayerBintr::HandlePause()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_asyncCommMutex);
        
        // Call the base class to Pause
        if (!SetState(GST_STATE_PAUSED, DSL_DEFAULT_STATE_CHANGE_TIMEOUT_IN_SEC * GST_SECOND))
        {
            LOG_ERROR("Failed to Pause Player '" << GetName() << "'");
        }
        if (g_main_loop_is_running(DSL::Services::GetServices()->GetMainLoopHandle()))
        {
            g_cond_signal(&m_asyncCondition);
        }
    }

    bool PlayerBintr::Stop()
    {
        LOG_FUNC();
        
        if (!IsLinked())
        {
            return false;
        }

        SendEos();
        sleep(1);

        // If the main loop is running -- normal case -- then we can't change the 
        // state of the Pipeline in the Application's context. 
        if (g_main_loop_is_running(DSL::Services::GetServices()->GetMainLoopHandle()))
        {
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_asyncCommMutex);
            g_timeout_add(1, PlayerStop, this);
            g_cond_wait(&m_asyncCondition, &m_asyncCommMutex);
        }
        // Else, we are running under test without the mainloop
        else
        {
            HandleStop();
        }
        return true;
    }
    
    void PlayerBintr::HandleStop()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_asyncCommMutex);

        if (!SetState(GST_STATE_NULL, DSL_DEFAULT_STATE_CHANGE_TIMEOUT_IN_SEC * GST_SECOND))
        {
            LOG_ERROR("Failed to Stop Pipeline '" << GetName() << "'");
        }
        UnlinkAll();
        if (g_main_loop_is_running(DSL::Services::GetServices()->GetMainLoopHandle()))
        {
            g_cond_signal(&m_asyncCondition);
        }
        // iterate through the map of Termination event listeners calling each
        for(auto const& imap: m_terminationEventListeners)
        {
            try
            {
                imap.first(imap.second);
            }
            catch(...)
            {
                LOG_ERROR("Exception calling Client Termination event Listener");
            }
        }
    }

    void PlayerBintr::HandleTermination()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_asyncCommMutex);

        // Start asyn Stop timer, do not wait or block as we are
        // in the State Manager's bus watcher context.
        g_timeout_add(1, PlayerStop, this);
        
        DestroyXWindow();
    }

    
    bool PlayerBintr::AddTerminationEventListener(
        dsl_player_termination_event_listener_cb listener, void* clientData)
    {
        LOG_FUNC();

        if (m_terminationEventListeners.find(listener) != m_terminationEventListeners.end())
        {   
            LOG_ERROR("Player listener is not unique");
            return false;
        }
        m_terminationEventListeners[listener] = clientData;
        
        return true;
    }
    
    bool PlayerBintr::RemoveTerminationEventListener(
        dsl_player_termination_event_listener_cb listener)
    {
        LOG_FUNC();

        if (m_terminationEventListeners.find(listener) == m_terminationEventListeners.end())
        {   
            LOG_ERROR("Player listener was not found");
            return false;
        }
        m_terminationEventListeners.erase(listener);
        
        return true;
    }

   const uint RenderPlayerBintr::m_displayId(0);
   const uint RenderPlayerBintr::m_depth(0);
    
    RenderPlayerBintr::RenderPlayerBintr(const char* name, uint renderType, 
        uint offsetX, uint offsetY, uint zoom)
        : PlayerBintr(name)
        , m_renderType(renderType)
        , m_zoom(zoom)
        , m_offsetX(offsetX)
        , m_offsetY(offsetY)
        , m_width(0)
        , m_height(0)
    {
        LOG_FUNC();

    }
    
    RenderPlayerBintr::~RenderPlayerBintr()
    {
        LOG_FUNC();
    }
    
    bool RenderPlayerBintr::CreateSink()
    {
        LOG_FUNC();
        
        std::string sinkName = m_name + "-render-sink__";
        if (m_renderType == DSL_RENDER_TYPE_OVERLAY)
        {
            m_pSink = DSL_OVERLAY_SINK_NEW(sinkName.c_str(), 
                m_displayId, m_depth, m_offsetX, m_offsetY, m_width, m_height);
        }
        else
        {
            m_pSink = DSL_WINDOW_SINK_NEW(sinkName.c_str(), 
                m_offsetX, m_offsetY, m_width, m_height);
        }
        if (!AddChild(m_pSink))
        {
            LOG_ERROR("Failed to add SinkBintr '" << m_pSink->GetName() 
                << "' to PlayerBintr '" << GetName() << "'");
            return false;
        }
        return true;
    }
    
    FileRenderPlayerBintr::FileRenderPlayerBintr(const char* name, const char* uri, 
        uint renderType, uint offsetX, uint offsetY, uint zoom, bool repeatEnabled)
        : RenderPlayerBintr(name, renderType, offsetX, offsetY, zoom)
        , m_repeatEnabled(repeatEnabled)
    {
        LOG_FUNC();

        
        std::string sourceName = m_name + "-file-source";
        m_pSource = DSL_FILE_SOURCE_NEW(name, uri, repeatEnabled);
            
        if (!AddChild(m_pSource))
        {
            LOG_ERROR("Failed to add SourceBintr '" << m_pSource->GetName() 
                << "' to PlayerBintr '" << GetName() << "'");
            throw;
        }

        // use openCV to open the file and read the Frame width and height properties.
        cv::VideoCapture vidCap;
        vidCap.open(uri, cv::CAP_ANY);

        if (!vidCap.isOpened())
        {
            LOG_ERROR("Failed to open URI '" << uri 
                << "' for FileRenderPlayerBintr '" << GetName() << "'");
            throw;
        }
        m_width = vidCap.get(cv::CAP_PROP_FRAME_WIDTH);
        m_height = vidCap.get(cv::CAP_PROP_FRAME_HEIGHT);
        
        // everything we need to create the SinkBintr
        if (!CreateSink())
        {
            LOG_ERROR("Failed to create RenderSink for FileRenderPlayerBintr '" 
                << GetName() << "'");
            throw;
        }
    }
    
    FileRenderPlayerBintr::~FileRenderPlayerBintr()
    {
        LOG_FUNC();
    }

    ImageRenderPlayerBintr::ImageRenderPlayerBintr(const char* name, const char* uri, 
        uint renderType, uint offsetX, uint offsetY, uint zoom, uint timeout)
        : RenderPlayerBintr(name, renderType, offsetX, offsetY, zoom)
        , m_timeout(timeout)
    {
        LOG_FUNC();
        
        
    }
    
    ImageRenderPlayerBintr::~ImageRenderPlayerBintr()
    {
        LOG_FUNC();
    }
    
    
    //--------------------------------------------------------------------------------
    
    static int PlayerPause(gpointer pPlayer)
    {
        static_cast<PlayerBintr*>(pPlayer)->HandlePause();
        
        // Return false to self destroy timer - one shot.
        return false;
    }
    
    static int PlayerStop(gpointer pPlayer)
    {
        static_cast<PlayerBintr*>(pPlayer)->HandleStop();
        
        // Return false to self destroy timer - one shot.
        return false;
    }
    
    static void PlayerTerminate(void* pPlayer)
    {
        static_cast<PlayerBintr*>(pPlayer)->HandleTermination();
    }    
    
} // DSL   