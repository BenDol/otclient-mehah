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

#include <framework/graphics/framebuffer.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/graphics/image.h>
#include <framework/graphics/painter.h>
#include "lightview.h"
#include "mapview.h"
#include "map.h"

LightView::LightView(const MapViewPtr& mapView)
{
    m_mapView = mapView;
    m_lightbuffer = g_framebuffers.createFrameBuffer();

    generateLightTexture();
    generateShadeTexture();

    resize();
}

void LightView::generateLightTexture()
{
    float brightnessIntensity = 1.2f,
        centerFactor = .0f;

    const uint16 bubbleRadius = 256,
        centerRadius = bubbleRadius * centerFactor,
        bubbleDiameter = bubbleRadius * 2;

    ImagePtr lightImage = ImagePtr(new Image(Size(bubbleDiameter, bubbleDiameter)));
    for(int_fast16_t x = -1; ++x < bubbleDiameter;) {
        for(int_fast16_t y = -1; ++y < bubbleDiameter;) {
            const float radius = std::sqrt((bubbleRadius - x) * (bubbleRadius - x) + (bubbleRadius - y) * (bubbleRadius - y));
            float intensity = stdext::clamp<float>((bubbleRadius - radius) / static_cast<float>(bubbleRadius - centerRadius), .0f, 1.0f);

            // light intensity varies inversely with the square of the distance
            const uint8_t colorByte = std::min<int16>((intensity * intensity * brightnessIntensity) * 0xff, 0xff);

            uint8_t pixel[4] = { 0xff, 0xff, 0xff, colorByte };
            lightImage->setPixel(x, y, pixel);
        }
    }

    m_lightTexture = TexturePtr(new Texture(lightImage));
    m_lightTexture->setSmooth(true);
}

void LightView::generateShadeTexture()
{
    const uint16 diameter = 6;
    const ImagePtr image = ImagePtr(new Image(Size(diameter, diameter)));
    for(int_fast16_t x = -1; ++x < diameter;) {
        for(int_fast16_t y = -1; ++y < diameter;) {
            const uint8 alpha = x == 0 || y == 0 || x == diameter - 1 || y == diameter - 1 ? 0 : 0xff;
            uint8_t pixel[4] = { 0xff, 0xff, 0xff, alpha };
            image->setPixel(x, y, pixel);
        }
    }

    m_shadeTexture = TexturePtr(new Texture(image));
    m_shadeTexture->setSmooth(true);
}

void LightView::addLightSource(const Point& pos, const Light& light)
{
    const uint16 radius = (light.intensity * 1.2) * Otc::TILE_PIXELS * m_mapView->m_scaleFactor;
    m_lights[m_currentFloor].push_back(LightSource{ pos , light.color, radius, (light.intensity > 1 ? 1.f : .1f) });
}

void LightView::setShade(const Point& point)
{
    size_t index = (m_mapView->m_drawDimension.width() * (point.y / m_mapView->m_tileSize)) + (point.x / m_mapView->m_tileSize);
    if(index >= m_shades.size()) return;
    m_shades[index] = ShadeBlock{ m_currentFloor, point };
}

void LightView::drawLights()
{
    // GlobalLight
    const float brightness = m_globalLight.intensity / static_cast<float>(UINT8_MAX);
    Color globalColor = Color::from8bit(m_globalLight.color, brightness);
    g_painter->setCompositionMode(Painter::CompositionMode_Replace);
    g_painter->setColor(globalColor);
    g_painter->drawFilledRect(Rect(0, 0, m_lightbuffer->getSize()));

    const auto orderLight = [&](const LightSource& a, const LightSource& b) -> bool {
        return a.brightness == b.brightness && a.color < b.color || a.brightness < b.brightness;
    };

    // Lights
    g_painter->setCompositionMode(Painter::CompositionMode_Normal);
    for(int_fast8_t z = m_mapView->m_floorMax; z >= m_mapView->m_floorMin; --z) {
        g_painter->setBlendEquation(Painter::BlendEquation_Add);
        g_painter->setColor(globalColor);
        for(auto& shade : m_shades) {
            if(shade.floor != z) continue;

            shade.floor = -1;
            g_painter->drawTexturedRect(Rect(shade.pos - (Point(1, 1) * m_mapView->m_tileSize) / 1.9, (Size(1, 1) * m_mapView->m_tileSize) * 1.9), m_shadeTexture);
        }

        g_painter->setBlendEquation(Painter::BlendEquation_Add);
        auto& floorLightsBlock = m_lights[z];

        std::sort(floorLightsBlock.begin(), floorLightsBlock.end(), orderLight);
        for(LightSource& light : floorLightsBlock) {
            g_painter->setColor(Color::from8bit(light.color, light.brightness));
            g_painter->drawTexturedRect(Rect(light.pos - (Point(1, 1) * light.radius), Size(1, 1) * light.radius * 2), m_lightTexture);
        }
        floorLightsBlock.clear();
    }
}

void LightView::resize()
{
    m_lightbuffer->resize(m_mapView->m_frameCache.tile->getSize());
    m_shades.resize(m_mapView->m_drawDimension.area());
}

void LightView::draw(const Rect& dest, const Rect& src)
{
    // draw light, only if there is darkness
    if(!isDark()) return;

    g_painter->saveAndResetState();
    if(m_lightbuffer->canUpdate()) {
        m_lightbuffer->bind();
        drawLights();
        m_lightbuffer->release();
    }
    g_painter->setCompositionMode(Painter::CompositionMode_Light);

    m_lightbuffer->draw(dest, src);
    g_painter->restoreSavedState();
}