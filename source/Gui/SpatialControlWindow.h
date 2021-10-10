#pragma once

#include "EngineImpl/Definitions.h"
#include "EngineInterface/Descriptions.h"

#include "Definitions.h"

class _SpatialControlWindow
{
public:
    _SpatialControlWindow(
        SimulationController const& simController,
        Viewport const& viewport,
        StyleRepository const& styleRepository);

    void process();

    bool isOn() const;
    void setOn(bool value);

private:
    void processZoomInButton();
    void processZoomOutButton();
    void processResizeButton();
    void processZoomSensitivitySlider();

    void processResizeDialog();

    void onResizing();

    SimulationController _simController;
    Viewport _viewport;
    StyleRepository _styleRepository;

    TextureData _zoomInTexture;
    TextureData _zoomOutTexture;
    TextureData _resizeTexture;

    bool _on = false;
    bool _showResizeDialog = false;
    bool _scaleContent = false;
    int _width = 0;
    int _height = 0;
};