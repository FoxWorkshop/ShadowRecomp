#include "installer_wizard.h"

#include <nfd.h>

#include <audio/embedded_player.h>
#include <install/xiso_extract.h>
#include <gpu/video.h>
#include <locale/locale.h>
#include <user/config.h>
#include <ui/game_window.h>
#include <ui/imgui_utils.h>
#include <os/logger.h>

#include <thread>
#include <atomic>
#include <chrono>

enum class WizardPage
{
    SelectISO,
    Installing,
    InstallSucceeded,
    InstallFailed,
};

static WizardPage g_currentPage = WizardPage::SelectISO;
static std::filesystem::path g_installPath;
static std::string g_isoPath;
static std::string g_statusMessage;

static std::unique_ptr<std::thread> g_installerThread;
static std::atomic<bool> g_installerFinished{false};
static std::atomic<bool> g_installerFailed{false};
static std::string g_installerErrorMessage;
static float g_installerProgress = 0.0f;
static std::atomic<float> g_installerProgressTarget{0.0f};
static std::string g_installerCurrentFileName;
static std::atomic<XisoExtract::Phase> g_installerPhase{XisoExtract::Phase::Hashing};

static bool g_isQuitting = false;
static bool g_browseClicked = false;

static void InstallerThread()
{
    auto progressCallback = [](const XisoExtract::Progress &progress)
    {
        g_installerCurrentFileName = progress.currentFileName;
        g_installerPhase.store(progress.phase);

        if (progress.phase == XisoExtract::Phase::Hashing)
        {
            g_installerProgressTarget.store(progress.ratio);
        }
        else
        {
            g_installerProgressTarget.store(progress.ratio);
        }
    };

    bool success = XisoExtract::Extract(g_isoPath, g_installPath, progressCallback);

    g_installerProgressTarget.store(1.0f);

    if (!success)
    {
        g_installerFailed.store(true);
        g_installerErrorMessage = "The selected file is not a valid Shadow the Hedgehog Xbox ISO. Only specific known disc images are supported.";
    }

    g_installerFinished.store(true);
}

void InstallerWizard::Init()
{
}

void InstallerWizard::Draw()
{
    if (!s_isVisible)
        return;

    auto &io = ImGui::GetIO();
    auto drawList = ImGui::GetBackgroundDrawList();
    auto &displaySize = io.DisplaySize;

    drawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(20, 20, 30, 255));

    auto *font = ImGui::GetFont();
    float fontSize = 24.0f;
    float smallFontSize = 16.0f;

    float centerX = displaySize.x * 0.5f;
    float panelWidth = 600.0f;
    float panelHeight = 400.0f;
    float panelX = centerX - panelWidth * 0.5f;
    float panelY = displaySize.y * 0.5f - panelHeight * 0.5f;

    ImVec2 panelMin(panelX, panelY);
    ImVec2 panelMax(panelX + panelWidth, panelY + panelHeight);

    drawList->AddRectFilled(panelMin, panelMax, IM_COL32(30, 30, 50, 255), 8.0f);
    drawList->AddRect(panelMin, panelMax, IM_COL32(80, 80, 120, 255), 8.0f, 0, 1.0f);

    auto titleText = "Shadow the Hedgehog Recompiled - Setup";
    auto titleSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, titleText);
    float titleX = centerX - titleSize.x * 0.5f;
    drawList->AddText(font, fontSize, ImVec2(titleX, panelY + 25), IM_COL32(220, 220, 255, 255), titleText);

    float contentY = panelY + 80;

    switch (g_currentPage)
    {
        case WizardPage::SelectISO:
        {
            auto instrText = "Select your Shadow the Hedgehog Xbox (NTSC-U) ISO file.";
            auto instrSize = font->CalcTextSizeA(smallFontSize, panelWidth - 40, 0, instrText);
            drawList->AddText(font, smallFontSize, ImVec2(panelX + 20, contentY), IM_COL32(200, 200, 200, 255), instrText, nullptr, panelWidth - 40);

            contentY += 50;

            float inputWidth = panelWidth - 40;
            float inputHeight = 32;
            ImVec2 inputMin(panelX + 20, contentY);
            ImVec2 inputMax(panelX + 20 + inputWidth, contentY + inputHeight);

            drawList->AddRectFilled(inputMin, inputMax, IM_COL32(40, 40, 60, 255), 4.0f);
            drawList->AddRect(inputMin, inputMax, IM_COL32(100, 100, 140, 255), 4.0f, 0, 1.0f);

            const char *displayPath = g_isoPath.empty() ? "No file selected..." : g_isoPath.c_str();
            if (strlen(displayPath) > 60)
            {
                displayPath = displayPath + strlen(displayPath) - 60;
            }
            drawList->AddText(font, smallFontSize, ImVec2(inputMin.x + 8, inputMin.y + 7), IM_COL32(180, 180, 180, 255), displayPath, nullptr, inputWidth - 100);

            float btnWidth = 120;
            float btnHeight = 32;
            float btnX = inputMax.x - btnWidth - 4;
            float btnY = inputMin.y;

            bool btnHovered = false;
            if (io.MousePos.x >= btnX && io.MousePos.x <= btnX + btnWidth &&
                io.MousePos.y >= btnY && io.MousePos.y <= btnY + btnHeight)
            {
                btnHovered = true;
            }

            ImU32 btnColor = btnHovered ? IM_COL32(80, 80, 140, 255) : IM_COL32(60, 60, 100, 255);
            drawList->AddRectFilled(ImVec2(btnX, btnY), ImVec2(btnX + btnWidth, btnY + btnHeight), btnColor, 4.0f);
            auto browseText = "Browse...";
            auto browseSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, browseText);
            drawList->AddText(font, smallFontSize, ImVec2(btnX + (btnWidth - browseSize.x) * 0.5f, btnY + 7), IM_COL32(220, 220, 255, 255), browseText);

            if (btnHovered && io.MouseClicked[0])
            {
                g_browseClicked = true;
            }

            contentY += inputHeight + 30;

            if (!g_statusMessage.empty())
            {
                drawList->AddText(font, smallFontSize, ImVec2(panelX + 20, contentY), IM_COL32(255, 100, 100, 255), g_statusMessage.c_str());
                contentY += 30;
            }

            if (!g_isoPath.empty())
            {
                float nextBtnWidth = 160;
                float nextBtnHeight = 40;
                float nextBtnX = centerX - nextBtnWidth * 0.5f;
                float nextBtnY = panelMax.y - nextBtnHeight - 25;

                bool nextHovered = false;
                if (io.MousePos.x >= nextBtnX && io.MousePos.x <= nextBtnX + nextBtnWidth &&
                    io.MousePos.y >= nextBtnY && io.MousePos.y <= nextBtnY + nextBtnHeight)
                {
                    nextHovered = true;
                }

                ImU32 nextColor = nextHovered ? IM_COL32(50, 150, 50, 255) : IM_COL32(30, 120, 30, 255);
                drawList->AddRectFilled(ImVec2(nextBtnX, nextBtnY), ImVec2(nextBtnX + nextBtnWidth, nextBtnY + nextBtnHeight), nextColor, 6.0f);

                auto nextText = "Install";
                auto nextSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, nextText);
                drawList->AddText(font, fontSize, ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + 8), IM_COL32(220, 255, 220, 255), nextText);

                if (nextHovered && io.MouseClicked[0])
                {
                    g_currentPage = WizardPage::Installing;
                    g_installerFinished.store(false);
                    g_installerFailed.store(false);
                    g_installerProgress = 0.0f;
                    g_installerProgressTarget.store(0.0f);
                    g_installerThread = std::make_unique<std::thread>(InstallerThread);
                }
            }

            float quitBtnWidth = 100;
            float quitBtnHeight = 36;
            float quitBtnX = panelMax.x - quitBtnWidth - 20;
            float quitBtnY = panelMax.y - quitBtnHeight - 25;

            bool quitHovered = false;
            if (io.MousePos.x >= quitBtnX && io.MousePos.x <= quitBtnX + quitBtnWidth &&
                io.MousePos.y >= quitBtnY && io.MousePos.y <= quitBtnY + quitBtnHeight)
            {
                quitHovered = true;
            }

            ImU32 quitColor = quitHovered ? IM_COL32(140, 50, 50, 255) : IM_COL32(100, 30, 30, 255);
            drawList->AddRectFilled(ImVec2(quitBtnX, quitBtnY), ImVec2(quitBtnX + quitBtnWidth, quitBtnY + quitBtnHeight), quitColor, 6.0f);

            auto quitText = "Quit";
            auto quitSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, quitText);
            drawList->AddText(font, smallFontSize, ImVec2(quitBtnX + (quitBtnWidth - quitSize.x) * 0.5f, quitBtnY + 9), IM_COL32(255, 200, 200, 255), quitText);

            if (quitHovered && io.MouseClicked[0])
            {
                g_isQuitting = true;
                s_isVisible = false;
            }

            break;
        }

        case WizardPage::Installing:
        {
            auto phase = g_installerPhase.load();
            const char *headerText = (phase == XisoExtract::Phase::Hashing)
                                        ? "Verifying disc image..."
                                        : "Extracting game files...";
            drawList->AddText(font, fontSize, ImVec2(panelX + 20, contentY), IM_COL32(220, 220, 255, 255), headerText);

            contentY += 50;

            if (!g_installerCurrentFileName.empty())
            {
                char fileText[512];
                snprintf(fileText, sizeof(fileText), "File: %s", g_installerCurrentFileName.c_str());
                drawList->AddText(font, smallFontSize, ImVec2(panelX + 20, contentY), IM_COL32(180, 180, 180, 255), fileText, nullptr, panelWidth - 40);
            }

            contentY += 30;

            float barX = panelX + 20;
            float barWidth = panelWidth - 40;
            float barHeight = 24;
            ImVec2 barMin(barX, contentY);
            ImVec2 barMax(barX + barWidth, contentY + barHeight);

            float target = g_installerProgressTarget.load();
            g_installerProgress += (target - g_installerProgress) * 0.1f;
            if (std::abs(g_installerProgress - target) < 0.001f)
                g_installerProgress = target;

            drawList->AddRectFilled(barMin, barMax, IM_COL32(40, 40, 60, 255), 4.0f);

            float fillWidth = barWidth * std::min(g_installerProgress, 1.0f);
            if (fillWidth > 0)
            {
                ImVec2 fillMax(barX + fillWidth, contentY + barHeight);
                drawList->AddRectFilled(barMin, fillMax, IM_COL32(50, 150, 50, 255), 4.0f);
            }

            char percentText[32];
            snprintf(percentText, sizeof(percentText), "%.1f%%", g_installerProgress * 100.0f);
            auto percentSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, percentText);
            drawList->AddText(font, smallFontSize, ImVec2(barX + (barWidth - percentSize.x) * 0.5f, contentY + 3), IM_COL32(255, 255, 255, 255), percentText);

            contentY += barHeight + 20;

            if (g_installerFinished.load())
            {
                if (g_installerFailed.load())
                {
                    g_currentPage = WizardPage::InstallFailed;
                }
                else
                {
                    g_currentPage = WizardPage::InstallSucceeded;
                }
            }

            break;
        }

        case WizardPage::InstallSucceeded:
        {
            auto successText = "Installation Complete!";
            auto successSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, successText);
            drawList->AddText(font, fontSize, ImVec2(centerX - successSize.x * 0.5f, contentY), IM_COL32(100, 255, 100, 255), successText);

            contentY += 50;

            auto launchDesc = "Game files have been extracted successfully.\nYou can now play Shadow the Hedgehog!";
            drawList->AddText(font, smallFontSize, ImVec2(panelX + 20, contentY), IM_COL32(200, 200, 200, 255), launchDesc, nullptr, panelWidth - 40);

            float playBtnWidth = 200;
            float playBtnHeight = 48;
            float playBtnX = centerX - playBtnWidth * 0.5f;
            float playBtnY = panelMax.y - playBtnHeight - 30;

            bool playHovered = false;
            if (io.MousePos.x >= playBtnX && io.MousePos.x <= playBtnX + playBtnWidth &&
                io.MousePos.y >= playBtnY && io.MousePos.y <= playBtnY + playBtnHeight)
            {
                playHovered = true;
            }

            ImU32 playColor = playHovered ? IM_COL32(50, 180, 50, 255) : IM_COL32(30, 140, 30, 255);
            drawList->AddRectFilled(ImVec2(playBtnX, playBtnY), ImVec2(playBtnX + playBtnWidth, playBtnY + playBtnHeight), playColor, 8.0f);

            auto playText = "Play Game";
            auto playSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, playText);
            drawList->AddText(font, fontSize, ImVec2(playBtnX + (playBtnWidth - playSize.x) * 0.5f, playBtnY + 12), IM_COL32(220, 255, 220, 255), playText);

            if (playHovered && io.MouseClicked[0])
            {
                s_isVisible = false;
            }

            break;
        }

        case WizardPage::InstallFailed:
        {
            auto failText = "Installation Failed";
            auto failSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, failText);
            drawList->AddText(font, fontSize, ImVec2(centerX - failSize.x * 0.5f, contentY), IM_COL32(255, 100, 100, 255), failText);

            contentY += 50;

            const char *errorMsg = g_installerErrorMessage.empty() ? "An unknown error occurred." : g_installerErrorMessage.c_str();
            drawList->AddText(font, smallFontSize, ImVec2(panelX + 20, contentY), IM_COL32(255, 180, 180, 255), errorMsg, nullptr, panelWidth - 40);

            float retryBtnWidth = 120;
            float retryBtnHeight = 40;
            float retryBtnX = centerX - retryBtnWidth - 10;
            float retryBtnY = panelMax.y - retryBtnHeight - 30;

            bool retryHovered = false;
            if (io.MousePos.x >= retryBtnX && io.MousePos.x <= retryBtnX + retryBtnWidth &&
                io.MousePos.y >= retryBtnY && io.MousePos.y <= retryBtnY + retryBtnHeight)
            {
                retryHovered = true;
            }

            ImU32 retryColor = retryHovered ? IM_COL32(80, 80, 140, 255) : IM_COL32(60, 60, 100, 255);
            drawList->AddRectFilled(ImVec2(retryBtnX, retryBtnY), ImVec2(retryBtnX + retryBtnWidth, retryBtnY + retryBtnHeight), retryColor, 6.0f);

            auto retryText = "Retry";
            auto retrySize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, retryText);
            drawList->AddText(font, smallFontSize, ImVec2(retryBtnX + (retryBtnWidth - retrySize.x) * 0.5f, retryBtnY + 10), IM_COL32(220, 220, 255, 255), retryText);

            if (retryHovered && io.MouseClicked[0])
            {
                if (g_installerThread && g_installerThread->joinable())
                    g_installerThread->join();
                g_installerThread.reset();

                g_currentPage = WizardPage::SelectISO;
                g_statusMessage.clear();
                g_installerFinished.store(false);
                g_installerFailed.store(false);
                g_installerProgress = 0.0f;
            }

            float quitBtnWidth = 120;
            float quitBtnHeight = 40;
            float quitBtnX = centerX + 10;
            float quitBtnY = panelMax.y - quitBtnHeight - 30;

            bool quitHovered = false;
            if (io.MousePos.x >= quitBtnX && io.MousePos.x <= quitBtnX + quitBtnWidth &&
                io.MousePos.y >= quitBtnY && io.MousePos.y <= quitBtnY + quitBtnHeight)
            {
                quitHovered = true;
            }

            ImU32 quitColor = quitHovered ? IM_COL32(140, 50, 50, 255) : IM_COL32(100, 30, 30, 255);
            drawList->AddRectFilled(ImVec2(quitBtnX, quitBtnY), ImVec2(quitBtnX + quitBtnWidth, quitBtnY + quitBtnHeight), quitColor, 6.0f);

            auto quitText = "Quit";
            auto quitSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, quitText);
            drawList->AddText(font, smallFontSize, ImVec2(quitBtnX + (quitBtnWidth - quitSize.x) * 0.5f, quitBtnY + 10), IM_COL32(255, 200, 200, 255), quitText);

            if (quitHovered && io.MouseClicked[0])
            {
                g_isQuitting = true;
                s_isVisible = false;
            }

            break;
        }
    }
}

void InstallerWizard::Shutdown()
{
    if (g_installerThread && g_installerThread->joinable())
        g_installerThread->join();
    g_installerThread.reset();

    g_currentPage = WizardPage::SelectISO;
    g_isoPath.clear();
    g_statusMessage.clear();
    g_installerFinished.store(false);
    g_installerFailed.store(false);
    g_installerProgress = 0.0f;
}

bool InstallerWizard::Run(std::filesystem::path installPath)
{
    g_installPath = installPath / "game";
    g_isQuitting = false;
    g_currentPage = WizardPage::SelectISO;

    EmbeddedPlayer::Init();
    NFD_Init();

    GameWindow::SetFullscreenCursorVisibility(true);
    s_isVisible = true;

    constexpr auto frameTime = std::chrono::duration<float, std::milli>(1000.0f / 30.0f);

    while (s_isVisible)
    {
        auto frameStart = std::chrono::steady_clock::now();

        Video::WaitOnSwapChain();

        EmbeddedPlayer::PlayMusic();

        SDL_PumpEvents();

        if (g_browseClicked)
        {
            g_browseClicked = false;

            nfdu8filteritem_t filters[] = {{"Xbox ISO", "iso"}};
            nfdu8char_t *outPath = nullptr;
            nfdresult_t result = NFD_OpenDialogU8(&outPath, filters, 1, nullptr);

            if (result == NFD_OKAY && outPath != nullptr)
            {
                g_isoPath = outPath;
                g_statusMessage.clear();
                NFD_FreePathU8(outPath);
            }
            else if (result == NFD_ERROR)
            {
                g_statusMessage = NFD_GetError();
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                if (g_currentPage == WizardPage::Installing)
                    continue;

                g_isQuitting = true;
                s_isVisible = false;
            }
        }

        GameWindow::Update();
        Video::Present();

        std::this_thread::sleep_until(frameStart + frameTime);
    }

    SDL_CaptureMouse(SDL_FALSE);

    SDL_PumpEvents();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

    GameWindow::SetFullscreenCursorVisibility(false);

    Config::Fullscreen = GameWindow::IsFullscreen();
    if (!GameWindow::IsFullscreen() && !GameWindow::IsMaximised())
    {
        Config::WindowX = GameWindow::s_x;
        Config::WindowY = GameWindow::s_y;
        Config::WindowWidth = GameWindow::s_width;
        Config::WindowHeight = GameWindow::s_height;
    }
    Config::Save();

    NFD_Quit();
    InstallerWizard::Shutdown();
    EmbeddedPlayer::Shutdown();

    return !g_isQuitting;
}
