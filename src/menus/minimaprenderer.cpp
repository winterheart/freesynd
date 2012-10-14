/************************************************************************
 *                                                                      *
 *  FreeSynd - a remake of the classic Bullfrog game "Syndicate".       *
 *                                                                      *
 *   Copyright (C) 2012  Benoit Blancard <benblan@users.sourceforge.net>*
 *                                                                      *
 *    This program is free software;  you can redistribute it and / or  *
 *  modify it  under the  terms of the  GNU General  Public License as  *
 *  published by the Free Software Foundation; either version 2 of the  *
 *  License, or (at your option) any later version.                     *
 *                                                                      *
 *    This program is  distributed in the hope that it will be useful,  *
 *  but WITHOUT  ANY WARRANTY;without even  the impliedwarranty of      *
 *  MERCHANTABILITY  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 *  General Public License for more details.                            *
 *                                                                      *
 *    You can view the GNU  General Public License, online, at the GNU  *
 *  project'sweb  site;  see <http://www.gnu.org/licenses/gpl.html>.  *
 *  The full text of the license is also included in the file COPYING.  *
 *                                                                      *
 ************************************************************************/

#include "menus/minimaprenderer.h"
#include "mission.h"
#include "gfx/screen.h"
#include "app.h"

const int MinimapRenderer::kMiniMapSizePx = 128;

void MinimapRenderer::setZoom(EZoom zoom) {
    zoom_ = zoom;
    pixpertile_ = 10 - zoom_;
    updateRenderingInfos();
}

BriefMinimapRenderer::BriefMinimapRenderer() : mm_timer(500) {
    scroll_step_ = 0;
}

/*!
 * Init the renderer with a new mission, zoom level and draw_enemies params.
 */
void BriefMinimapRenderer::init(Mission *pMission, EZoom zoom, bool draw_enemies) {
    p_mission_ = pMission;
    setZoom(zoom);
    b_draw_enemies_ = draw_enemies;
    mm_timer.reset();
    minimap_blink_ = 0;

    // Initialize minimap origin by looking for the position
    // of the first found agent on the map
    initMinimapLocation();
}

/*!
 * Centers the minimap on the starting position of agents
 */
void BriefMinimapRenderer::initMinimapLocation() {
    bool found = false;
    int maxx = p_mission_->mmax_x_;
    int maxy = p_mission_->mmax_y_;

    for (int x = 0; x < maxx && (!found); x++) {
        for (int y = 0; y < maxy && (!found); y++) {
            if (p_mission_->getMinimapOverlay(x, y) == 1) {
                // We found a tile with an agent on it
                // stop searching and memorize position
                mm_tx_ = x;
                mm_ty_ = y;
                found = true;
            }
        }
    }

    uint16 halftiles = mm_maxtile_ / 2;
    mm_tx_ = (mm_tx_ < halftiles) ? 0 : (mm_tx_ - halftiles + 1);
    mm_ty_ = (mm_ty_ < halftiles) ? 0 : (mm_ty_ - halftiles + 1);

    clipMinimapToRightAndDown();
}

/*!
 *
 */
void BriefMinimapRenderer::clipMinimapToRightAndDown() {
    if ((mm_tx_ + mm_maxtile_) >= p_mission_->mmax_x_) {
        // We assume that map size in tiles (p_mission_->mmax_x_)
        // is bigger than the minimap size (mm_maxtile_)
        mm_tx_ = p_mission_->mmax_x_ - mm_maxtile_;
    }

    if ((mm_ty_ + mm_maxtile_) >= p_mission_->mmax_y_) {
        // We assume that map size in tiles (p_mission_->mmax_y_)
        // is bigger than the minimap size (mm_maxtile_)
        mm_ty_ = p_mission_->mmax_y_ - mm_maxtile_;
    }
}

void BriefMinimapRenderer::updateRenderingInfos() {
    scroll_step_ = 30 / pixpertile_;
    mm_maxtile_ = 120 / pixpertile_;
}

void BriefMinimapRenderer::zoomOut() {
    switch (zoom_) {
    case ZOOM_X2:
        setZoom(ZOOM_X1);
        break;
    case ZOOM_X3:
        setZoom(ZOOM_X2);
        break;
    case ZOOM_X4:
        setZoom(ZOOM_X3);
        break;
    default:
        break;
    }

    // check if map should be aligned with right and bottom border
    // as when zooming out only displays more tiles but does not
    // move the minimap origin
    clipMinimapToRightAndDown();
}

bool BriefMinimapRenderer::handleTick(int elapsed) {
    if (mm_timer.update(elapsed)) {
        minimap_blink_ ^= 1;
        return true;
    }

    return false;
}

/*!
 * Scrolls right using current scroll step. If scroll is too far,
 * clips scrolling to the map's right border.
 */
void BriefMinimapRenderer::scrollRight() {
    mm_tx_ += scroll_step_;
    clipMinimapToRightAndDown();
}

/*!
 * Scrolls left using current scroll step. If scroll is too far,
 * clips scrolling to the map's left border.
 */
void BriefMinimapRenderer::scrollLeft() {
    // if scroll_step is bigger than mm_tx_
    // then mm_tx_ -= scroll_step_ would be negative
    // but mm_tx_ is usigned so it would be an error
    if (mm_tx_ < scroll_step_) {
        mm_tx_ = 0;
    } else {
        // we know that mm_tx_ >= scroll_step_
        mm_tx_ -= scroll_step_;
    }
}

/*!
 * Scrolls up using current scroll step. If scroll is too far,
 * clips scrolling to the map's top border.
 */
void BriefMinimapRenderer::scrollUp() {
    if (mm_ty_ < scroll_step_) {
        mm_ty_ = 0;
    } else {
        // we know that mm_ty_ >= scroll_step_
        mm_ty_ -= scroll_step_;
    }
}

/*!
 * Scrolls down using current scroll step. If scroll is too far,
 * clips scrolling to the map's bottom border.
 */
void BriefMinimapRenderer::scrollDown() {
    mm_ty_ += scroll_step_;
    clipMinimapToRightAndDown();
}

/*!
 * Renders the minimap at the given position on the screen.
 * \param mm_x X coord in absolute pixels.
 * \param mm_y Y coord in absolute pixels.
 */
void BriefMinimapRenderer::render(uint16 mm_x, uint16 mm_y) {
    for (uint16 tx = mm_tx_; tx < (mm_tx_ + mm_maxtile_); tx++) {
        uint16 xc = mm_x + (tx - mm_tx_) * pixpertile_;
        for (uint16 ty = mm_ty_; ty < (mm_ty_ + mm_maxtile_); ty++) {
            unsigned char c = p_mission_->getMinimapOverlay(tx, ty);
            switch (c) {
                case 0:
                    c = p_mission_->getMiniMap()->getColourAt(tx, ty);
                    break;
                case 1:
                    c = minimap_blink_ ? 14 : 12;
                    break;
                case 2:
                    if (b_draw_enemies_)
                        c = minimap_blink_ ? 14 : 5;
                    else
                        c = p_mission_->getMiniMap()->getColourAt(tx, ty);
            }
            g_Screen.drawRect(xc, mm_y + (ty - mm_ty_) * pixpertile_, pixpertile_,
                pixpertile_, c);
        }
    }
}

/*!
 * Default constructor.
 */
GamePlayMinimapRenderer::GamePlayMinimapRenderer() {
    p_mission_ = NULL;
}

/*!
 * Sets a new mission for rendering the minimap.
 * \param pMission A mission.
 */
void GamePlayMinimapRenderer::init(Mission *pMission) {
    p_mission_ = pMission;
    setZoom(ZOOM_X3);
    mm_tx_ = 0;
    mm_ty_ = 0;
    offset_x_ = 0;
    offset_y_ = 0;
}

void GamePlayMinimapRenderer::updateRenderingInfos() {
    mm_maxtile_ = 128 / pixpertile_ + 1;
}

/*!
 * Centers the minimap on the given tile. Usually, the minimap is centered
 * on the selected agent.
 * \param tileX The X coord of the tile.
 * \param tileX The Y coord of the tile.
 * \param offX The offset of the agent on the tile.
 * \param offY The offset of the agent on the tile.
 */
void GamePlayMinimapRenderer::centerOn(uint16 tileX, uint16 tileY, int offX, int offY) {
    uint16 halfSize = mm_maxtile_ / 2;
    mm_tx_ = (tileX < halfSize) ? 0 : tileX - halfSize;
    mm_ty_ = (tileY < halfSize) ? 0 : tileY - halfSize;

    // TODO : offset ratio depends on zoom level
    offset_x_ = (tileX == p_mission_->get_map()->maxX() -1) ? 0 : offX / 32;
    offset_y_ = (tileY == p_mission_->get_map()->maxY() -1) ? 0 : offY / 32;
}

/*!
 * Renders the minimap at the given position on the screen.
 * \param mm_x X coord in absolute pixels.
 * \param mm_y Y coord in absolute pixels.
 */
void GamePlayMinimapRenderer::render(uint16 mm_x, uint16 mm_y) {
    // A temporary buffer composed of 17*17 tiles of 8*8 pixels
    // 17*17*8*8 > 33*33*4*4
    uint8 minimap_layer[17*17*8*8];
    // The final minimap that will be displayed : the minimap is 128*128 pixels
    uint8 minimap_final_layer[kMiniMapSizePx*kMiniMapSizePx];

    // The temporary buffer is 17*17 because when we'll draw the final minimap,
    // the offset will c
    memset(minimap_layer, 0, 17*17*8*8);
    for (int j = 0; j < mm_maxtile_; j++) {
        for (int i = 0; i < mm_maxtile_; i++) {
            uint8 gcolour = p_mission_->getMiniMap()->getColourAt(mm_tx_ + i, mm_ty_ + j);
            for (char inc = 0; inc < pixpertile_; inc ++) {
                memset(minimap_layer + j * pixpertile_ * pixpertile_ * mm_maxtile_ + 
                    i * pixpertile_ + inc * pixpertile_ * mm_maxtile_,
                    gcolour, pixpertile_);
            }
        }
    }
    
    // Copy the temp buffer in the final minimap using the tile offset so the minimap movement
    // is smoother
    for (int j = 0; j < kMiniMapSizePx; j++) {
        memcpy(minimap_final_layer + (kMiniMapSizePx * j), minimap_layer +
            (j + offset_y_) * pixpertile_ * mm_maxtile_ + offset_x_, kMiniMapSizePx);
    }

    // Draw the minimap on the screen
    g_Screen.blit(mm_x, mm_y, kMiniMapSizePx, kMiniMapSizePx, minimap_final_layer);
    
    // Draw the minimap cross
    // TODO cross is not fix. it is centered on the leader.
    g_Screen.drawRect(mm_x + 64, mm_y, 1, kMiniMapSizePx, 0);
    g_Screen.drawRect(mm_x, mm_y + 64, kMiniMapSizePx, 1, 0);
}