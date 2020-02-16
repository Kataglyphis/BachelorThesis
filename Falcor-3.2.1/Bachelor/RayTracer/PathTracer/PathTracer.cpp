/***************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "PathTracer.h"
#include "RenderPasses/GGXGlobalIllumination.h"
#include "RenderPasses/GBufferRaster.h"
#include "RenderPasses/Sorting.h"
#include "RenderPasses/Retargeting.h"
#include "RenderPasses/TemporalReprojection.h"

void PathTracer::onGuiRender(SampleCallbacks* pCallbacks, Gui* pGui)
{
    if (pGui->addButton("Load Scene"))
    {
        assert(mpGraph != nullptr);
        std::string filename;
        if (openFileDialog(Scene::kFileExtensionFilters, filename))
        {
            ProgressBar::SharedPtr pBar = ProgressBar::create("Loading Scene", 100);

            RtScene::SharedPtr pScene = RtScene::loadFromFile(filename);
            if (pScene != nullptr)
            {
                Fbo::SharedPtr pFbo = pCallbacks->getCurrentFbo();
                pScene->setCamerasAspectRatio(float(pFbo->getWidth()) / float(pFbo->getHeight()));
                mpGraph->setScene(pScene);
            }
        }
    }

    pGui->addSeparator();

    if (pGui->addButton(mDisableCameraPath ? "Enable Camera Path" : "Disable Camera Path"))
    {
        toggleCameraPathState();
    }

    if (mpGraph != nullptr)
    {
        mpGraph->renderUI(pGui, nullptr);
    }
}

void PathTracer::toggleCameraPathState()
{
    Scene::SharedPtr pScene = mpGraph->getScene();
    if (pScene != nullptr && pScene->getPathCount() > 0)
    {
        mDisableCameraPath = !mDisableCameraPath;
        if (mDisableCameraPath)
        {
            pScene->getPath(0)->detachObject(pScene->getActiveCamera());
        }
        else
        {
            pScene->getPath(0)->attachObject(pScene->getActiveCamera());
        }
    }
}

void PathTracer::onLoad(SampleCallbacks* pCallbacks, RenderContext* pRenderContext)
{
    mpGraph = RenderGraph::create("Path Tracer");

    mpGraph->addPass(GBufferRaster::create(), "GBuffer");
    auto pGIPass = GGXGlobalIllumination::create();
    mpGraph->addPass(pGIPass, "GlobalIllumination");

    //make improvements by sorting frames after rendering
    mpGraph->addPass(Sorting::create(), "Sorting");
    //for retarget seeds before rendering frame!
    mpGraph->addPass(Retargeting::create(), "Retargeting");
    //adding temporal reprojection step for the seeds
    //mpGraph->addPass(TemporalReprojection::create(), "TemporalReprojection");


    mpGraph->addEdge("GBuffer", "Retargeting");
    mpGraph->addEdge("Retargeting", "GlobalIllumination");
    mpGraph->addEdge("GBuffer", "GlobalIllumination");
    //improvements in sorting happens after rendering frame
    mpGraph->addEdge("GlobalIllumination", "Sorting");

    mpGraph->addEdge("Retargeting.output_seed", "GlobalIllumination.input_seed");

    //bevor render now retarget seeds to accumulate improvements
    //mpGraph->addEdge("Sorting.output_seed", "GlobalIllumination.input_seed");

    mpGraph->addEdge("GBuffer.posW", "GlobalIllumination.posW");
    mpGraph->addEdge("GBuffer.normW", "GlobalIllumination.normW");
    mpGraph->addEdge("GBuffer.diffuseOpacity", "GlobalIllumination.diffuseOpacity");
    mpGraph->addEdge("GBuffer.specRough", "GlobalIllumination.specRough");
    mpGraph->addEdge("GBuffer.emissive", "GlobalIllumination.emissive");
    mpGraph->addEdge("GBuffer.matlExtra", "GlobalIllumination.matlExtra");

    //add edges for marking dependencies!!

    //mpGraph->addEdge("Sorting","Retargeting");

    //Edges for our temporal algorithm

    //the retargeted seeds will come into our path tracer
    //the rendered frame from our path tracer where we get our values to sort
    mpGraph->addEdge("GlobalIllumination.output", "Sorting.input_frame");
    mpGraph->addEdge("GlobalIllumination.output_seed","Sorting.input_seed");

    //edges for our retargeting pass
    //mpGraph->addEdge("Sorting.output_seed","Retargeting.input_seed");

    mpGraph->markOutput("GlobalIllumination.output");
    mpGraph->markOutput("Sorting.output_seed");
    //mpGraph->markOutput("Retargeting.output_seed");

    // Initialize the graph's record of what the swapchain size is, for texture creation
    mpGraph->onResize(pCallbacks->getCurrentFbo().get());

    {
        ProgressBar::SharedPtr pBar = ProgressBar::create("Loading Scene", 100);

        RtScene::SharedPtr pScene = RtScene::loadFromFile("Arcade/Arcade.fscene");
        //RtScene::SharedPtr pScene = RtScene::loadFromFile("pink_room/pink_room.fscene");
        //RtScene::SharedPtr pScene = RtScene::loadFromFile("Bistro_v4/Bistro_Exterior.fscene");
        //RtScene::SharedPtr pScene = RtScene::loadFromFile("Bistro_v4/Bistro_Interior.fscene");
        //RtModel::SharedPtr pModel = RtModel::createFromFile("CornellBox/CornellBox-Original.obj");
        //RtScene::SharedPtr pScene = RtScene::createFromModel(pModel);
        //pScene->addModelInstance();
        //RtScene::SharedPtr pScene = RtScene::loadFromFile("EmeraldSquare/EmeraldSquare_Day.fscene");
        if (pScene != nullptr)
        {
            Fbo::SharedPtr pFbo = pCallbacks->getCurrentFbo();
            pScene->setCamerasAspectRatio(float(pFbo->getWidth()) / float(pFbo->getHeight()));
            mpGraph->setScene(pScene);
        }
        trace_count = 0;
    }
}

void PathTracer::onFrameRender(SampleCallbacks* pCallbacks, RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    // const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    const glm::vec4 clearColor(0.f, 0.f, 0.0f, 1);
    pRenderContext->clearFbo(pTargetFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if (!hasrunonce) {

        //Texture::SharedPtr seed_texture = createTextureFromFile("seeds_RGBA.png", false, false,Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess |
        //                                                                                                                                    Resource::BindFlags::RenderTarget);
        Texture::SharedPtr seed_texture = createTextureFromFile("seeds_init_MersenneTwister 9.2.2020 - 21_31.png", false, false, Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess |
                                                                                                                                         Resource::BindFlags::RenderTarget);
        mpGraph->setInput("Retargeting.input_seed", seed_texture);

        //just do nothing; we will load starting seed texture in globalillumination pass 
        hasrunonce = true;

        //from our initialized seeds
        //takeScreenshot(pCallbacks);

    } else {

        //bring our retargeted seeds into the globalillumination stage
        //Resource::SharedPtr retarget_seeds = mpGraph->getOutput("Retargeting.output_seed");
        //mpGraph->setInput("GlobalIllumination.seed_input", retarget_seeds);

        Resource::SharedPtr retarget_seeds = mpGraph->getOutput("Sorting.output_seed");
        mpGraph->setInput("Retargeting.input_seed", retarget_seeds);

        //if(this->trace_count <= 9) takeScreenshot(pCallbacks);
    }

    //enable this llop for saving the very first 10 screenshots!!!
 

    if (mpGraph->getScene() != nullptr)
    {
        mpGraph->getScene()->update(pCallbacks->getCurrentTime(), &mCamController);
        mpGraph->execute(pRenderContext);
        pRenderContext->blit(mpGraph->getOutput("GlobalIllumination.output")->getSRV(), pTargetFbo->getRenderTargetView(0));
        //pRenderContext->blit(mpGraph->getOutput("Sorting.output_seed")->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
}

void PathTracer::onShutdown(SampleCallbacks* pCallbacks)
{
}

bool PathTracer::onKeyEvent(SampleCallbacks* pCallbacks, const KeyboardEvent& keyEvent)
{
    if (keyEvent.key == KeyboardEvent::Key::Minus && keyEvent.type == KeyboardEvent::Type::KeyPressed)
    {
        toggleCameraPathState();
        return true;
    }

    bool handled = false;
    if (mpGraph->getScene() != nullptr) handled = mpGraph->onKeyEvent(keyEvent);
    return handled ? true : mCamController.onKeyEvent(keyEvent);
}

bool PathTracer::onMouseEvent(SampleCallbacks* pCallbacks, const MouseEvent& mouseEvent)
{
    bool handled = false;
    if (mpGraph->getScene() != nullptr) handled = mpGraph->onMouseEvent(mouseEvent);
    return handled ? true : mCamController.onMouseEvent(mouseEvent);
}

void PathTracer::onDataReload(SampleCallbacks* pCallbacks)
{

}

void PathTracer::onResizeSwapChain(SampleCallbacks* pCallbacks, uint32_t width, uint32_t height)
{
    if (mpGraph)
    {
        mpGraph->onResize(pCallbacks->getCurrentFbo().get());
        if(mpGraph->getScene() != nullptr) mpGraph->getScene()->setCamerasAspectRatio((float)width / (float)height);
    }
}

void PathTracer::takeScreenshot(SampleCallbacks* pCallbacks) {

        std::stringstream ss;
        ss << "seed_debug_" << this->trace_count;
        std::string filename = ss.str();
        pCallbacks->captureScreen(filename, "Screenshots");
        trace_count++;

}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    PathTracer::UniquePtr pRenderer = std::make_unique<PathTracer>();
    SampleConfig config;
    config.windowDesc.title = "Blue Noise Distribution Path Tracer";
    config.windowDesc.width = 1920;
    config.windowDesc.height = 1080;
    config.windowDesc.resizableWindow = true;
    config.freezeTimeOnStartup = true;
    Sample::run(config, pRenderer);
    return 0;
}
