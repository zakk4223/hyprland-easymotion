#include "easymotionDeco.hpp"

#include <cairo/cairo.h>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <pango/pangocairo.h>

#include "globals.hpp"

CHyprEasyLabel::CHyprEasyLabel(PHLWINDOW pWindow, SMotionActionDesc *actionDesc) : IHyprWindowDecoration(pWindow) {
    m_pWindow = pWindow;

    const auto PMONITOR       = pWindow->m_pMonitor.lock();
    PMONITOR->scheduledRecalc = true;
		m_szWindowAddress = std::format("0x{:x}", (uintptr_t)pWindow.get());
		m_szActionCmd = std::vformat(actionDesc->commandString, std::make_format_args(m_szWindowAddress));
		m_iTextSize = actionDesc->textSize;
	  m_cTextColor = actionDesc->textColor;
		m_cBackgroundColor = actionDesc->backgroundColor;
		m_szTextFont = actionDesc->textFont;
		m_iPaddingTop = actionDesc->boxPadding.top;
		m_iPaddingBottom = actionDesc->boxPadding.bottom;
		m_iPaddingRight = actionDesc->boxPadding.right;
		m_iPaddingLeft = actionDesc->boxPadding.left;
		m_iRounding = actionDesc->rounding;
		m_iBorderSize = actionDesc->borderSize;
		m_cBorderGradient = actionDesc->borderColor;
}

CHyprEasyLabel::~CHyprEasyLabel() {
    damageEntire();
    std::erase(g_pGlobalState->motionLabels, this);
}

SDecorationPositioningInfo CHyprEasyLabel::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.policy         = DECORATION_POSITION_ABSOLUTE;
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
  glBindTexture(GL_TEXTURE_2D, m_tTextTex->m_iTexID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferSize.x, bufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);

    // delete cairo
	cairo_destroy(LAYOUTCAIRO);
  cairo_destroy(CAIRO);
  cairo_surface_destroy(CAIROSURFACE);
}


void CHyprEasyLabel::draw(PHLMONITOR pMonitor, float const &a) {
    if (!validMapped(m_pWindow))
        return;

    if (!m_pWindow->m_sWindowData.decorate.valueOrDefault())
        return;

    const auto PWORKSPACE      = m_pWindow->m_pWorkspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !m_pWindow->m_bPinned ? PWORKSPACE->m_vRenderOffset->value() : Vector2D();

    const auto ROUNDING = m_iRounding; 

    const auto scaledRounding = ROUNDING > 0 ? ROUNDING * pMonitor->scale : 0;

    const auto DECOBOX = assignedBoxGlobal();

    const auto BARBUF = DECOBOX.size() * pMonitor->scale;

    //CBox       motionBox = {DECOBOX.x - pMonitor->vecPosition.x, DECOBOX.y - pMonitor->vecPosition.y, DECOBOX.w,
	
		auto TEXTBUF = DECOBOX.size() * pMonitor->scale;
	

		if (!m_tTextTex.get()) {
			renderMotionString(TEXTBUF, pMonitor->scale);
		}
    CBox       motionBox = {DECOBOX.x, DECOBOX.y, static_cast<double>(layoutWidth), static_cast<double>(layoutHeight)};
    motionBox.translate(pMonitor->vecPosition*-1).scale(pMonitor->scale).round();

    if (motionBox.w < 1 || motionBox.h < 1)
        return;
    g_pHyprOpenGL->scissor(motionBox);
    g_pHyprOpenGL->renderRect(motionBox, m_cBackgroundColor, scaledRounding);
		
		if (m_iBorderSize) {
    	CBox       borderBox = {DECOBOX.x, DECOBOX.y, static_cast<double>(layoutWidth), static_cast<double>(layoutHeight)};
    	borderBox.translate(pMonitor->vecPosition*-1).scale(pMonitor->scale).round();
			if (borderBox.w >= 1 && borderBox.h >= 1) {
				g_pHyprOpenGL->renderBorder(borderBox, m_cBorderGradient, scaledRounding, m_iBorderSize * pMonitor->scale, a);
			}
		}


		g_pHyprOpenGL->renderTexture(m_tTextTex, motionBox, a);

    g_pHyprOpenGL->scissor(nullptr);
}

eDecorationType CHyprEasyLabel::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CHyprEasyLabel::updateWindow(PHLWINDOW pWindow) {
    damageEntire();
}

void CHyprEasyLabel::damageEntire() {
    ; // ignored
}

eDecorationLayer CHyprEasyLabel::getDecorationLayer() {
    return DECORATION_LAYER_OVERLAY;
}

uint64_t CHyprEasyLabel::getDecorationFlags() {
    return DECORATION_PART_OF_MAIN_WINDOW; 
}

CBox CHyprEasyLabel::assignedBoxGlobal() {

		double boxHeight, boxWidth;
		double boxSize;
		boxHeight = m_pWindow->m_vRealSize->value().y * 0.10;
		boxWidth = m_pWindow->m_vRealSize->value().x * 0.10;
		boxSize = std::min(boxHeight, boxWidth);
	  double boxX = m_pWindow->m_vRealPosition->value().x + (m_pWindow->m_vRealSize->value().x-boxSize)/2;
	  double boxY = m_pWindow->m_vRealPosition->value().y + (m_pWindow->m_vRealSize->value().y-boxSize)/2;
    CBox box = {boxX, boxY, boxSize, boxSize};

    const auto PWORKSPACE      = m_pWindow->m_pWorkspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !m_pWindow->m_bPinned ? PWORKSPACE->m_vRenderOffset->value() : Vector2D();

    return box.translate(WORKSPACEOFFSET);
}

PHLWINDOW CHyprEasyLabel::getOwner() {
    return m_pWindow;
}
