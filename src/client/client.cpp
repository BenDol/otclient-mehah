/*
 * Copyright (c) 2010-2022 OTClient <https://github.com/edubart/otclient>
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

#include "client.h"
#include "game.h"
#include "map.h"
#include "uimap.h"
#include "minimap.h"
#include "spriteappearances.h"
#include "spritemanager.h"

#include <framework/ui/ui.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/asyncdispatcher.h>
#include <framework/core/resourcemanager.h>
#include <framework/graphics/shadermanager.h>

Client g_client;

void Client::init(std::vector<std::string>& /*args*/)
{
    // register needed lua functions
    registerLuaFunctions();

    g_gameConfig.init();
    g_map.init();
    g_minimap.init();
    g_game.init();
    g_shaders.init();
    g_sprites.init();
    g_spriteAppearances.init();
    g_things.init();
}

void Client::terminate()
{
#ifdef FRAMEWORK_EDITOR
    g_creatures.terminate();
#endif
    g_game.terminate();
    g_map.terminate();
    g_minimap.terminate();
    g_things.terminate();
    g_sprites.terminate();
    g_spriteAppearances.terminate();
    g_shaders.terminate();
    g_gameConfig.terminate();
}

bool Client::drawForgroundTile(DrawPool* forgroundTilePool, DrawPool* textPool, DrawPool* mapPool)
{
    if (g_game.isOnline()) {
        if (!m_mapWidget)
            m_mapWidget = g_ui.getRootWidget()->recursiveGetChildById("gameMapPanel")->static_self_cast<UIMap>();

        if (textPool->canRepaint() || forgroundTilePool->canRepaint()) {
            g_asyncDispatcher.dispatch([this, &textPool] {
                std::scoped_lock l(textPool->getMutex());
                g_textDispatcher.poll();

                if (m_mapWidget) {
                    m_mapWidget->drawSelf(DrawPoolType::TEXT);
                    m_mapWidget->drawSelf(DrawPoolType::FOREGROUND_TILE);
                }
            });
        }

        {
            std::scoped_lock l(mapPool->getMutex());
            m_mapWidget->drawSelf(DrawPoolType::MAP);
        }
    }
    else m_mapWidget = nullptr;

    return true;
}

bool Client::canDrawTexts() const
{
    return !g_map.getStaticTexts().empty() || !g_map.getAnimatedTexts().empty();
}

bool Client::isLoadingAsyncTexture()
{
    return g_game.isUsingProtobuf();
}

bool Client::isUsingProtobuf()
{
    return g_game.isUsingProtobuf();
}

void Client::setLoadingAsyncTexture(bool loadingAsync)
{
    g_sprites.reload();
}
