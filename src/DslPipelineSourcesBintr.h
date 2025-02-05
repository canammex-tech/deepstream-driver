
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

#ifndef _DSL_PIPELINE_SOURCES_BINTR_H
#define _DSL_PIPELINE_SOURCES_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslSourceBintr.h"
#include "DslStreammuxBintr.h"
#include "DslAudiomixBintr.h"

namespace DSL
{
    #define DSL_PIPELINE_SOURCES_PTR std::shared_ptr<PipelineSourcesBintr>
    #define DSL_PIPELINE_SOURCES_NEW(name, uniquePipelineId) \
        std::shared_ptr<PipelineSourcesBintr> \
           (new PipelineSourcesBintr(name, uniquePipelineId))

    typedef enum
    {
        DSL_VIDEOMUX = 0,
        DSL_AUDIOMUX = 1
    } streammux_type;


    class PipelineSourcesBintr : public Bintr
    {
    public: 
    
        PipelineSourcesBintr(const char* name, uint uniquePipelineId);

        ~PipelineSourcesBintr();
        
        /**
         * @brief adds a child SourceBintr to this PipelineSourcesBintr
         * @param pChildSource shared pointer to SourceBintr to add
         * @return true if the SourceBintr was added correctly, false otherwise
         */
        bool AddChild(DSL_SOURCE_PTR pChildSource);
        
        /**
         * @brief removes a child SourceBintr from this PipelineSourcesBintr
         * @param pChildElement a shared pointer to SourceBintr to remove
         * @return true if the SourceBintr was removed correctly, false otherwise
         */
        bool RemoveChild(DSL_SOURCE_PTR pChildSource);

        /**
         * @brief overrides the base method and checks in m_pChildSources only.
         */
        bool IsChild(DSL_SOURCE_PTR pChildSource);

        /**
         * @brief overrides the base Noder method to only return the number of 
         * child SourceBintrs and not the total number of children... 
         * i.e. exclude the nuber of child Elementrs from the count
         * @return the number of Child SourceBintrs held by this PipelineSourcesBintr
         */
        uint GetNumChildren()
        {
            LOG_FUNC();
            
            return m_pChildSources.size();
        }

        /**
         * @brief interates through the list of child source bintrs setting 
         * their Sensor Id's and linking to the Streammux
         */
        bool LinkAll();
        
        /**
         * @brief interates through the list of child source bintrs unlinking
         * them from the Streammux and reseting their Sensor Id's
         */
        void UnlinkAll();

        /**
         * @brief Gets the current Streammuxer enabled setting for either the Audio
         * or Video Streammuxer.
         * @return true if enabled, false otherwise.
         */
        boolean GetStreammuxEnabled(streammux_type streammux);

        /**
         * @brief Gets the current Streammuxer enabled setting for either the Audio
         * or Video Streammuxer.
         * @param[in] streammux either DSL_VIDEOMUX or DSL_AUDIOMUX.
         * @param[in] set to true to enabled, false otherwise.
         * @return true if set successfully, false otherwise.
         */
        bool SetStreammuxEnabled(streammux_type streammux, boolean enabled);
        
        /**
         * @brief Gets the current Streammuxer "play-type-is-live" setting
         * @return true if play-type is live, false otherwise
         */
        bool StreammuxPlayTypeIsLiveGet();

        /**
         * @brief Sets the current Streammuxer play type based on the first source 
         * added
         * @param isLive set to true if all sources are to be Live, and therefore 
         * live only.
         * @return true if live-source is succesfully set, false otherwise
         */
        bool StreammuxPlayTypeIsLiveSet(bool isLive);

        /**
         * @brief Gets the current enabled setting for the Audiomixer.
         * @return true if enabled, false otherwise.
         */
        boolean GetAudiomixEnabled();

        /**
         * @brief Gets the current enabled setting for the Audiomixer.
         * @param[in] enabled set to true to enable, false otherwise.
         * @return true if set successfully, false otherwise.
         */
        bool SetAudiomixEnabled(boolean enabled);
        
        /**
         * @brief Gets the mute enabled setting for one of the Audiomixer's sink pad.
         * @param[in] pChildSource shared pointer to the Child Source that is
         * connected to the Audiomixer's sink pad to query.
         * @param[out] enabled true if mute is enable, false otherwise.
         * @return true if queried successfully, false otherwise.
         */
        bool GetAudiomixMuteEnabled(DSL_AUDIO_SOURCE_PTR pChildSource, 
            boolean* enabled);

        /**
         * @brief Sets the mute enabled setting for one of the Audiomixer's sink pad.
         * @param[in] pChildSource shared pointer to the Child Source that is
         * connected to the Audiomixer's sink pad to update.
         * @param[in] enabled set to true to enable, false otherwise.
         * @return true if set successfully, false otherwise.
         */
        bool SetAudiomixMuteEnabled(DSL_AUDIO_SOURCE_PTR pChildSource, 
            boolean enabled);

        /**
         * @brief Gets the volume setting for one of the Audiomixer's sink pad.
         * @param[in] pChildSource shared pointer to the Child Source that is
         * connected to the Audiomixer's sink pad to query.
         * @param[out] volume current volume between 0.0 and 10.0, default = 1.
         * @return true if queried successfully, false otherwise.
         */
        bool GetAudiomixVolume(DSL_AUDIO_SOURCE_PTR pChildSource, 
            double* volume);

        /**
         * @brief Sets the volume setting for one of the Audiomixer's sink pad.
         * @param[in] pChildSource shared pointer to the Child Source that is
         * connected to the Audiomixer's sink pad to update.
         * @param[in] volume new volume between 0.0 and 10.0.
         * @return true if set successfully, false otherwise.
         */
        bool SetAudiomixVolume(DSL_AUDIO_SOURCE_PTR pChildSource, 
            double volume);

        void EosAll();

        /**
         * @brief Calls on all child Sources to disable their EOS consumers.
         */
        void DisableEosConsumers();

        /**
         * @brief Video Streammuxer for this PipelineSourcesBintr, 
         * enabled by default.
         */
        DSL_STREAMMUX_PTR m_pVideomux;

        /**
         * @brief Audio Streammuxer for this PipelineSourcesBintr, 
         * disabled by default.
         */
        DSL_STREAMMUX_PTR m_pAudiomux;

        /**
         * @brief Audiomixer for this PipelineSourcesBintr, 
         * disabled by default.
         */
        DSL_AUDIOMIX_PTR m_pAudiomix;

    private:

        void SetBatchSizes();

        void ClearBatchSizes();
        
        /**
         * @brief Links a child Source to this PipelineSourcesBintr.
         * @param pChildSource a shared pointer to the Source to link.
         * @return true on successful add, false otherwise.
         */
        bool LinkChildToSinkMuxers(DSL_SOURCE_PTR pChildSource);

        /**
         * @brief Unlinks a child Source from this PipelineSourcesBintr.
         * @param pChildSource a shared pointer to the Source to unlink.
         * @return true on successful add, false otherwise.
         */
        bool UnlinkChildFromSinkMuxers(DSL_SOURCE_PTR pChildSource);

        /**
         * @brief unique id for the Parent Pipeline, used to offset all source
         * Id's (if greater than 0)
         */
        uint m_uniquePipelineId; 
        
        /**
         * @brief adds a child Elementr to this PipelineSourcesBintr
         * @param pChildElement a shared pointer to the Elementr to add
         * @return a shared pointer to the Elementr if added correctly, 
         * nullptr otherwise
         */
        bool AddChild(DSL_BASE_PTR pChildElement);
        
        /**
         * @brief removes a child Elementr from this PipelineSourcesBintr
         * @param pChildElement a shared pointer to the Elementr to remove
         */
        bool RemoveChild(DSL_BASE_PTR pChildElement);

        /**
         * @brief Updates the PipelineSourcesBintr media-type base on
         * which muxer and/or mixer has been enabled.
         */
        void UpdateMediaType();

        /**
         * @brief current number of child Sources that support AUDIO 
         */       
        uint m_numAudioSources;

        /**
         * @brief current number of child Sources that support VIDEO 
         */       
        uint m_numVideoSources;

        /**
         * @brief container of all child sources mapped by their unique names
         */
        std::map<std::string, DSL_SOURCE_PTR> m_pChildSources;
        
        /**
         * @brief container of all child sources mapped by their unique stream-id
         */
        std::map<uint, DSL_SOURCE_PTR> m_pChildSourcesIndexed;

        /**
         * @brief true if all sources are live, false if all sources are non-live
         */
        bool m_areSourcesLive;
        
        /**
         * @brief Each source is assigned a unique pad/stream id used to define the
         * streammuxer sink pad when linking. The vector is used on add/remove 
         * to find the next available pad id.
         */
        std::vector<bool> m_usedRequestPadIds;
        
    };

    
}

#endif // _DSL_PIPELINE_SOURCES_BINTR_H
