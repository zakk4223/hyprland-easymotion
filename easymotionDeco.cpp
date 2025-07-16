#include "easymotionDeco.hpp"

#include <cairo/cairo.h>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/pass/RectPassElement.hpp>
#include <hyprland/src/render/pass/TexPassElement.hpp>
#include <hyprland/src/render/pass/BorderPassElement.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <pango/pangocairo.h>

#include "globals.hpp"

CHyprEasyLabel::CHyprEasyLabel(PHLWINDOW pWindow, SMotionActionDesc *actionDesc) : IHyprWindowDecoration(pWindow) {
    m_pWindow = pWindow;

    const auto PMONITOR       = pWindow->m_monitor.lock();
    PMONITOR->m_scheduledRecalc = true;
		m_szWindowAddress = std::format("0x{:x}", (uintptr_t)pWindow.get());
		m_szActionCmd = std::vformat(actionDesc->commandString, std::make_format_args(m_szWindowAddress));
		m_iTextSize = actionDesc->textSize;
	  m_cTextColor = actionDesc->textColor;
		m_cBackgroundColor = actionDesc->backgroundColor;
		m_szTextFont = actionDesc->textFont;
		m_iPaddingTop = actionDesc->boxPadding.m_top;
		m_iPaddingBottom = actionDesc->boxPadding.m_bottom;
		m_iPaddingRight = actionDesc->boxPadding.m_right;
		m_iPaddingLeft = actionDesc->boxPadding.m_left;
		m_iRounding = actionDesc->rounding;
		m_iBorderSize = actionDesc->borderSize;
		m_cBorderGradient = actionDesc->borderColor;
    m_iBlur = actionDesc->blur;
    m_iBlurA = actionDesc->blurA;
    m_iXray = actionDesc->xray;
}

CHyprEasyLabel::~CHyprEasyLabel() {
    damageEntire();
    std::erase(g_pGlobalState->motionLabels, m_self);
}

SDecorationPositioningInfo CHyprEasyLabel::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.policy         = DECORATION_POSITION_ABSOLUTE;
    info.edges = DECORATION_EDGE_BOTTOM;
    info.priority = 10000;
    return info;
}

void CHyprEasyLabel::onPositioningReply(const SDecorationPositioningReply& reply) {
	return;
}

std::string CHyprEasyLabel::getDisplayName() {
    return "EasyMotion";
}

void CHyprEasyLabel::renderMotionString(Vector2D& bufferSize, const float scale) {
	m_tTextTex = makeShared<CTexture>();
	int textSize = m_iTextSize;
	const auto scaledSize = textSize * scale;
	const auto textColor = CHyprColor(m_cTextColor);

	const auto LAYOUTSURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
	const auto LAYOUTCAIRO = cairo_create(LAYOUTSURFACE);
	cairo_surface_destroy(LAYOUTSURFACE);

	PangoLayout *layout = pango_cairo_create_layout(LAYOUTCAIRO);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
	pango_layout_set_text(layout, m_szLabel.c_str(), -1);
	PangoFontDescription *fontDesc = pango_font_description_from_string(m_szTextFont.c_str());
	
	pango_font_description_set_size(fontDesc, scaledSize * PANGO_SCALE);
	pango_layout_set_font_description(layout, fontDesc);
	pango_font_description_free(fontDesc);

	PangoRectangle ink_rect;
	PangoRectangle logical_rect;
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);

	layoutWidth = ink_rect.width+m_iPaddingLeft+m_iPaddingRight;
	layoutHeight = ink_rect.height+m_iPaddingTop+m_iPaddingBottom;

	bufferSize.x = layoutWidth;
	bufferSize.y = layoutHeight;

	const auto CAIROSURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bufferSize.x, bufferSize.y);
	const auto CAIRO = cairo_create(CAIROSURFACE);

	cairo_save(CAIRO);
	cairo_set_operator(CAIRO, CAIRO_OPERATOR_CLEAR);
	cairo_paint(CAIRO);
	cairo_restore(CAIRO);
	cairo_move_to(CAIRO, -ink_rect.x+(m_iPaddingLeft), -ink_rect.y+(m_iPaddingTop));
	cairo_set_source_rgba(CAIRO, textColor.r, textColor.g, textColor.b, textColor.a);
	pango_cairo_show_layout(CAIRO, layout);
	g_object_unref(layout);
	cairo_surface_flush(CAIROSURFACE);
	const auto DATA = cairo_image_surface_get_data(CAIROSURFACE);
	m_tTextTex->allocate();
  m_tTextTex->m_size = {bufferSize.x, bufferSize.y};
  glBindTexture(GL_TEXTURE_2D, m_tTextTex->m_texID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#ifndef GLES2
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferSize.x, bufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);

    // delete cairo
	cairo_destroy(LAYOUTCAIRO);
  cairo_destroy(CAIRO);
  cairo_surface_destroy(CAIROSURFACE);

  //renderText doesn't use ink_rect, but logical_rect. Makes the rectangle too tall
  //m_tTextTex = g_pHyprOpenGL->renderText(m_szLabel, m_cTextColor, textSize, false, m_szTextFont, bufferSize.x-2);
}


void CHyprEasyLabel::draw(PHLMONITOR pMonitor, float const &a) {
    if (!validMapped(m_pWindow))
        return;
 
    const auto PWINDOW = m_pWindow.lock();

    if (!PWINDOW->m_windowData.decorate.valueOrDefault())
        return;

    const auto PWORKSPACE      = PWINDOW->m_workspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !PWINDOW->m_pinned ? PWORKSPACE->m_renderOffset->value() : Vector2D();

    const auto DECOBOX = assignedBoxGlobal();

    const auto BARBUF = DECOBOX.size() * pMonitor->m_scale;

    //CBox       motionBox = {DECOBOX.x - pMonitor->vecPosition.x, DECOBOX.y - pMonitor->vecPosition.y, DECOBOX.w,
	
		auto TEXTBUF = DECOBOX.size();
	

		if (!m_tTextTex.get()) {
			renderMotionString(TEXTBUF, pMonitor->m_scale);
		}

    CBox       motionBox = {DECOBOX.x, DECOBOX.y, m_tTextTex->m_size.x, m_tTextTex->m_size.y};
    motionBox.translate(pMonitor->m_position*-1).scale(pMonitor->m_scale).round();

    if (motionBox.w < 1 || motionBox.h < 1)
    {
        return;
   }

    CRectPassElement::SRectData rectData;
    rectData.color = m_cBackgroundColor;
    rectData.box = motionBox;
    rectData.clipBox = motionBox;
    rectData.blur = m_iBlur;
    if (m_iBlur) {
      rectData.blurA = m_iBlurA;
      rectData.xray = m_iXray;
    }

    rectData.round = m_iRounding != 0;
    rectData.roundingPower = m_iRounding ;
    
		
    g_pHyprRenderer->m_renderPass.add(makeUnique<CRectPassElement>(rectData));
		if (m_iBorderSize) {
    	CBox       borderBox = {DECOBOX.x, DECOBOX.y, static_cast<double>(layoutWidth), static_cast<double>(layoutHeight)};
    	borderBox.translate(pMonitor->m_position*-1).scale(pMonitor->m_scale).round();
			if (borderBox.w >= 1 && borderBox.h >= 1) {
        CBorderPassElement::SBorderData borderData;
        borderData.box = borderBox;
        borderData.grad1 = m_cBorderGradient;
        borderData.round = m_iRounding != 0;
        borderData.roundingPower = m_iRounding;
        borderData.borderSize = m_iBorderSize;
        borderData.a = a;
        g_pHyprRenderer->m_renderPass.add(makeUnique<CBorderPassElement>(borderData));
				//g_pHyprOpenGL->renderBorder(borderBox, m_cBorderGradient, scaledRounding, m_iBorderSize * pMonitor->scale, a);
			}
		}
  
    CTexPassElement::SRenderData texData;
    motionBox.round();
    texData.tex = m_tTextTex;
    texData.box = motionBox;
  
  
    g_pHyprRenderer->m_renderPass.add(makeUnique<CTexPassElement>(texData));
}

eDecorationType CHyprEasyLabel::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CHyprEasyLabel::updateWindow(PHLWINDOW pWindow) {
    damageEntire();
}

void CHyprEasyLabel::damageEntire() {
  auto box = assignedBoxGlobal();
  box.translate(m_pWindow->m_floatingOffset);
  g_pHyprRenderer->damageBox(box);
}

eDecorationLayer CHyprEasyLabel::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

uint64_t CHyprEasyLabel::getDecorationFlags() {
    return DECORATION_PART_OF_MAIN_WINDOW;
}

CBox CHyprEasyLabel::assignedBoxGlobal() {

    const auto PWINDOW = m_pWindow.lock();
		double boxHeight, boxWidth;
		double boxSize;
		boxHeight = PWINDOW->m_realSize->value().y * 0.10;
		boxWidth = PWINDOW->m_realSize->value().x * 0.10;
		boxSize = std::min(boxHeight, boxWidth);
	  double boxX = PWINDOW->m_realPosition->value().x + (PWINDOW->m_realSize->value().x-boxSize)/2;
	  double boxY = PWINDOW->m_realPosition->value().y + (PWINDOW->m_realSize->value().y-boxSize)/2;
    CBox box = {boxX, boxY, boxSize, boxSize};

    const auto PWORKSPACE      = PWINDOW->m_workspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !PWINDOW->m_pinned ? PWORKSPACE->m_renderOffset->value() : Vector2D();

    return box.translate(WORKSPACEOFFSET);
}

PHLWINDOW CHyprEasyLabel::getOwner() {
    return m_pWindow.lock();
}
