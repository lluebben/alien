#include "SpatialControlWindow.h"

#include "imgui.h"

#include "Base/StringFormatter.h"
#include "EngineInterface/ChangeDescriptions.h"
#include "EngineInterface/DescriptionHelper.h"
#include "EngineImpl/SimulationController.h"
#include "StyleRepository.h"
#include "Viewport.h"
#include "OpenGLHelper.h"
#include "Resources.h"
#include "GlobalSettings.h"
#include "AlienImGui.h"

_SpatialControlWindow::_SpatialControlWindow(
    SimulationController const& simController,
    Viewport const& viewport,
    StyleRepository const& styleRepository)
    : _simController(simController)
    , _viewport(viewport)
    , _styleRepository(styleRepository)
{
    _zoomInTexture = OpenGLHelper::loadTexture(Const::ZoomInFilename);
    _zoomOutTexture = OpenGLHelper::loadTexture(Const::ZoomOutFilename);
    _resizeTexture = OpenGLHelper::loadTexture(Const::ResizeFilename);
    _on = GlobalSettings::getInstance().getBoolState("windows.spatial control.active", true);
}

_SpatialControlWindow::~_SpatialControlWindow()
{
    GlobalSettings::getInstance().setBoolState("windows.spatial control.active", _on);
}

void _SpatialControlWindow::process()
{
    if (!_on) {
        return;
    }

    ImGui::SetNextWindowBgAlpha(Const::WindowAlpha * ImGui::GetStyle().Alpha);
    ImGui::Begin("Spatial control", &_on);

    processZoomInButton();
    ImGui::SameLine();
    processZoomOutButton();
    ImGui::SameLine();
    processResizeButton();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::BeginChild("##", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {

        ImGui::Text("World size");
        ImGui::PushFont(_styleRepository->getLargeFont());
        ImGui::PushStyleColor(ImGuiCol_Text, Const::TextDecentColor);
        auto worldSize = _simController->getWorldSize();
        ImGui::TextUnformatted(
            (StringFormatter::format(worldSize.x) + " x " + StringFormatter::format(worldSize.y)).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Text("Zoom factor");
        ImGui::PushFont(_styleRepository->getLargeFont());
        ImGui::PushStyleColor(ImGuiCol_Text, Const::TextDecentColor);
        ImGui::TextUnformatted(StringFormatter::format(_viewport->getZoomFactor(), 1).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Text("Center position");
        ImGui::PushFont(_styleRepository->getLargeFont());
        ImGui::PushStyleColor(ImGuiCol_Text, Const::TextDecentColor);
        auto centerPos = _viewport->getCenterInWorldPos();
        ImGui::TextUnformatted(
            (StringFormatter::format(centerPos.x, 1) + ", " + StringFormatter::format(centerPos.y, 1)).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        processZoomSensitivitySlider();
    }
    ImGui::EndChild();

    ImGui::End();

    processResizeDialog();
}

bool _SpatialControlWindow::isOn() const
{
    return _on;
}

void _SpatialControlWindow::setOn(bool value)
{
    _on = value;
}

void _SpatialControlWindow::processZoomInButton()
{
    if (ImGui::ImageButton((void*)(intptr_t)_zoomInTexture.textureId, {32.0f, 32.0f}, {0, 0}, {1.0f, 1.0f})) {
        _viewport->setZoomFactor(_viewport->getZoomFactor() * 2);
    }
}

void _SpatialControlWindow::processZoomOutButton()
{
    if (ImGui::ImageButton((void*)(intptr_t)_zoomOutTexture.textureId, {32.0f, 32.0f}, {0, 0}, {1.0f, 1.0f})) {
        _viewport->setZoomFactor(_viewport->getZoomFactor() / 2);
    }
}

void _SpatialControlWindow::processResizeButton()
{
    if (ImGui::ImageButton((void*)(intptr_t)_resizeTexture.textureId, {32.0f, 32.0f}, {0, 0}, {1.0f, 1.0f})) {
        _showResizeDialog = true;
        auto worldSize = _simController->getWorldSize();
        _width = worldSize.x;
        _height = worldSize.y;
    }
}

void _SpatialControlWindow::processZoomSensitivitySlider()
{
    ImGui::Text("Zoom sensitivity");

    float sensitivity = _viewport->getZoomSensitivity();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderFloat("", &sensitivity, 1.0f, 1.15f, "")) {
        _viewport->setZoomSensitivity(sensitivity);
    }
    ImGui::PopItemWidth();
}

void _SpatialControlWindow::processResizeDialog()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (_showResizeDialog) {
        ImGui::OpenPopup("Resize world");
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Resize world", NULL, 0)) {
            if (ImGui::BeginTable("##", 2, ImGuiTableFlags_SizingStretchProp)) {

                //width
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputInt("##width", &_width);
                ImGui::PopItemWidth();

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Width");

                //height
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputInt("##height", &_height);
                ImGui::PopItemWidth();

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Height");

                ImGui::EndTable();
            }
            ImGui::Checkbox("Scale content", &_scaleContent);

            AlienImGui::Separator();

            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                _showResizeDialog = false;
                onResizing();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                _showResizeDialog = false;
            }

            ImGui::EndPopup();
            _width = std::max(1, _width);
            _height = std::max(1, _height);
        }
    }
}

void _SpatialControlWindow::onResizing()
{
    auto timestep = static_cast<uint32_t>(_simController->getCurrentTimestep());
    auto settings = _simController->getSettings();
    auto symbolMap = _simController->getSymbolMap();
    auto content = _simController->getSimulationData({0, 0}, _simController->getWorldSize());

    _simController->closeSimulation();

    IntVector2D origWorldSize{settings.generalSettings.worldSizeX, settings.generalSettings.worldSizeY};
    settings.generalSettings.worldSizeX = _width;
    settings.generalSettings.worldSizeY = _height;
    
    _simController->newSimulation(timestep, settings, symbolMap);

    DescriptionHelper::correctConnections(content, {_width, _height});
    if (_scaleContent) {
        DescriptionHelper::duplicate(content, origWorldSize, {_width, _height});
    }
    _simController->setSimulationData(content);
}
