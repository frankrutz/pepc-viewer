#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "spectrum_pair.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#define GLSL_VERSION "#version 100"
#include <SDL.h>
#include <SDL_opengles2.h>
#else
#define GLSL_VERSION "#version 130"
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#endif

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------

static SDL_Window*   g_window     = nullptr;
static SDL_GLContext g_gl_context = nullptr;
static bool          g_done       = false;

static SpectrumPair  g_pair;

// ---------------------------------------------------------------------------
// Sample data
// ---------------------------------------------------------------------------

static void setupExamplePair()
{
    Spectrum top(
        "Spectrum A", "spec_001", 1.0,
        {100.0, 150.0, 200.0, 250.0, 300.0, 350.0},
        {  0.8,   0.6,   1.0,   0.4,   0.7,   0.3}
    );

    Spectrum bottom(
        "Spectrum B", "spec_002", 1.0,
        {100.0, 175.0, 200.0, 275.0, 300.0, 325.0},
        {  0.9,   0.5,   0.8,   0.3,   0.6,   0.2}
    );

    g_pair = SpectrumPair(std::move(top), std::move(bottom));
}

// ---------------------------------------------------------------------------
// Plot helper: draw a single spectrum as stems from the zero baseline
// ---------------------------------------------------------------------------

static void plotStems(const char* label, const std::vector<double>& mz,
                      const std::vector<double>& intensity, double ySign,
                      ImVec4 colour)
{
    ImPlotSpec spec;
    spec.LineColor  = colour;
    spec.LineWeight = 1.5f;
    for (std::size_t i = 0; i < mz.size(); ++i) {
        double xs[2] = {mz[i], mz[i]};
        double ys[2] = {0.0,   ySign * intensity[i]};
        // Use the label only for the first stem so the legend entry appears once
        ImPlot::PlotLine(i == 0 ? label : "##", xs, ys, 2, spec);
    }
}

// ---------------------------------------------------------------------------
// Main render / UI
// ---------------------------------------------------------------------------

static void renderFrame()
{
    // ---- Begin ImGui frame -------------------------------------------------
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // ---- Full-screen dockable window ---------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("pepc-viewer", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    // ---- Info panel --------------------------------------------------------
    auto& tp = g_pair.top;
    auto& bp = g_pair.bottom;
    ImGui::Text("Top:    %s  (id: %s  weight: %.3f  peaks: %zu)",
                tp.name.c_str(), tp.id.c_str(), tp.weight, tp.size());
    ImGui::Text("Bottom: %s  (id: %s  weight: %.3f  peaks: %zu)",
                bp.name.c_str(), bp.id.c_str(), bp.weight, bp.size());
    ImGui::Separator();

    // ---- Mirror plot -------------------------------------------------------
    ImVec2 plotSize = {ImGui::GetContentRegionAvail().x,
                       ImGui::GetContentRegionAvail().y};

    if (ImPlot::BeginPlot("##mirror", plotSize)) {
        ImPlot::SetupAxes("m/z", "Intensity",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

        // Top spectrum – positive y (blue)
        plotStems(tp.name.c_str(), tp.mz, tp.intensity, +1.0,
                  {0.20f, 0.55f, 0.90f, 1.0f});

        // Bottom spectrum – negative y (orange/red)
        plotStems(bp.name.c_str(), bp.mz, bp.intensity, -1.0,
                  {0.90f, 0.40f, 0.10f, 1.0f});

        // Shared peaks – vertical green connecting lines
        auto shared = g_pair.sharedPeaks();
        ImPlotSpec sharedSpec;
        sharedSpec.LineColor  = {0.10f, 0.80f, 0.10f, 1.0f};
        sharedSpec.LineWeight = 2.0f;
        for (std::size_t k = 0; k < shared.size(); ++k) {
            const auto& sp = shared[k];
            double xs[2] = {sp.mz, sp.mz};
            double ys[2] = {-sp.bottomIntensity, +sp.topIntensity};
            ImPlot::PlotLine(k == 0 ? "shared m/z" : "##sh", xs, ys, 2, sharedSpec);
        }

        ImPlot::EndPlot();
    }

    ImGui::End();

    // ---- Render ------------------------------------------------------------
    ImGui::Render();
    int display_w, display_h;
    SDL_GL_GetDrawableSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_window);
}

#ifdef __EMSCRIPTEN__
// Emscripten main loop callback
static void emscriptenLoop(void*)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            g_done = true;
    }
    renderFrame();
}
#endif

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int /*argc*/, char** /*argv*/)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return 1;
    }

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                          SDL_WINDOW_ALLOW_HIGHDPI);
    g_window = SDL_CreateWindow("pepc-viewer – Spectrum Pair",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                1280, 720, window_flags);
    if (!g_window) {
        SDL_Log("SDL_CreateWindow error: %s", SDL_GetError());
        return 1;
    }

    g_gl_context = SDL_GL_CreateContext(g_window);
    SDL_GL_MakeCurrent(g_window, g_gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    setupExamplePair();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(emscriptenLoop, nullptr, 0, true);
#else
    while (!g_done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                g_done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(g_window))
                g_done = true;
        }
        renderFrame();
    }
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(g_gl_context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
