/*
 * Copyright (c) 2010-2020 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LIGHTVIEW_H
#define LIGHTVIEW_H

#include <framework/graphics/framebuffer.h>
#include <framework/graphics/declarations.h>
#include <framework/graphics/painter.h>
#include "declarations.h"
#include "thingtype.h"

struct ShadeBlock {
    int8 floor = -1;
    Point pos;
};

struct LightSource {
    Point pos;
    uint8 color;
    uint16 radius;
    float brightness;
};

class LightView : public LuaObject
{
public:
    LightView(const MapViewPtr& mapView);

    const Light& getGlobalLight() { return m_globalLight; }

    void resize();
    void draw(const Rect& dest, const Rect& src);
    void addLightSource(const Point& mainCenter, const Light& light);

    void setGlobalLight(const Light& light) { m_globalLight = light; }
    void setShade(const Point& point);
    void schedulePainting(const uint16_t delay = FrameBuffer::MIN_TIME_UPDATE) const { if(isDark()) m_lightbuffer->schedulePainting(delay); }

    bool canUpdate() const { return isDark() && m_lightbuffer->canUpdate(); }
    bool isDark() const { return m_globalLight.intensity < 250; }
    void setFloor(const uint8 floor) { m_currentFloor = floor; }

private:
    void generateLightTexture(),
        generateShadeTexture(),
        drawLights();

    TexturePtr m_lightTexture,
        m_shadeTexture;

    Light m_globalLight;

    FrameBufferPtr m_lightbuffer;
    MapViewPtr m_mapView;

    int8 m_currentFloor;

    std::vector<ShadeBlock> m_shades;
    std::array<std::vector<LightSource>, Otc::MAX_Z + 1> m_lights;
};

#endif
