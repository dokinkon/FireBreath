/**********************************************************\
 Original Author: Georg Fritzsche
 
 Created:    Mar 26, 2010
 License:    Dual license model; choose one of two:
 Eclipse Public License - Version 1.0
 http://www.eclipse.org/legal/epl-v10.html
 - or -
 GNU Lesser General Public License, version 2.1
 http://www.gnu.org/licenses/lgpl-2.1.html
 
 Copyright 2010 Georg Fritzsche, Firebreath development team
 \**********************************************************/

#include "PluginEvents/MacEventCocoa.h"
#include "PluginEvents/GeneralEvents.h"
#include "PluginEvents/DrawingEvents.h"
#include "PluginEvents/MouseEvents.h"
#include "PluginEvents/KeyboardEvents.h"
#include "PluginWindowMacCarbonCG.h"

using namespace FB;

PluginWindowMacCarbonCG::PluginWindowMacCarbonCG(NP_CGContext* context) {
    this->cgContext = context;
}

PluginWindowMacCarbonCG::~PluginWindowMacCarbonCG() {
    this->clearWindow();
}

NP_CGContext* PluginWindowMacCarbonCG::getContext() {
    if (this->cgContext != NULL) {
        return this->cgContext;
    } else {
        // TODO: error
        return 0;
    }
}

void PluginWindowMacCarbonCG::setContext(NP_CGContext* context) {
    cgContext = context;
}

// Intercepting Carbon events probably should happen in PluginWindowMacCarbon,
// but for now it's here since we need a CG windowRef to calculate mouse coords
int16_t PluginWindowMacCarbonCG::HandleEvent(EventRecord* evt) {
    // Give the plugin a chance to handle the event itself if desired
    MacEventCarbon macEvent(evt);
    switch (evt->what) {
        case mouseDown:
        {
            HIPoint* point = new HIPoint();
            point->x = evt->where.h;
            point->y = evt->where.v;
            HIPointConvert(point, kHICoordSpaceScreenPixel, NULL, kHICoordSpaceWindow, this->cgContext->window);

            int x_0 = point->x - m_x;
            int y_0 = m_height - (point->y - m_y);
            MouseDownEvent ev(MouseButtonEvent::MouseButton_Left, x_0, y_0);                               
            return SendEvent(&ev);
        }

        case mouseUp:
        {
            HIPoint* point = new HIPoint();
            point->x = evt->where.h;
            point->y = evt->where.v;
            HIPointConvert(point, kHICoordSpaceScreenPixel, NULL, kHICoordSpaceWindow, this->cgContext->window);

            int x_0 = point->x - m_x;
            int y_0 = m_height - (point->y - m_y);
            MouseUpEvent ev(MouseButtonEvent::MouseButton_Left, x_0, y_0);
            return SendEvent(&ev);
        }

        case keyDown:
        {
            // I don't know why, but the key codes are shifted 8 bits left,
            // we must shift them back to their natural positions before
            // the keymap can work properly
            FBKeyCode fb_key = CarbonKeyCodeToFBKeyCode(evt->message >> 8);
            KeyDownEvent ev(fb_key, evt->message);
            return SendEvent(&ev);
        }

        case keyUp:
        {
            // I don't know why, but the key codes are shifted 8 bits left,
            // we must shift them back to their natural positions before
            // the keymap can work properly
            FBKeyCode fb_key = CarbonKeyCodeToFBKeyCode(evt->message >> 8);
            KeyUpEvent ev(fb_key, evt->message);
            return SendEvent(&ev);
        }

        case nullEvent:
        {
            //TODO: Figure out more efficient timing mechanism
            // Get mouse coordinates and fire an event to the plugin
            CGEventRef nullEvent = CGEventCreate(NULL);
            CGPoint pointCG = CGEventGetLocation(nullEvent);
            HIPoint* point = new HIPoint();
            point->x = pointCG.x;
            point->y = pointCG.y;
            HIPointConvert(point, kHICoordSpaceScreenPixel, NULL, kHICoordSpaceWindow, this->cgContext->window);
            // <hack>
            int px = point->x - m_x;
            int py = m_height - (point->y - m_y);
            // </hack>
            // px & py have been translated to plugin window's coordinates space
            if((px > 0) && (px < m_width)) {
                if((py > 0) && (py < m_height)) {
                    if ((px == this->m_old_x) && (py == this->m_old_y)) {
                        SendEvent(&macEvent);
                        return false;
                    } else {
                        this->m_old_x = px;
                        this->m_old_y = py;
                        // Mouse event happened inside the plugin's window
                        MouseMoveEvent mmEvt(px, py);
                        SendEvent(&mmEvt);
                    }
                }
            }

            // The plugin is still expecting a nullEvent (for drawing)
            SendEvent(&macEvent);

            return false; 
        }
    
        case updateEvt:
        {
            RefreshEvent refEv;
            return SendEvent(&refEv);
            break;
        }
        default:
            if (SendEvent(&macEvent)) {
                return true;
            }
    }
    return false;
}