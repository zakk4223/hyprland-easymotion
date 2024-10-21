#pragma once

#include <hyprland/src/config/ConfigDataValues.hpp>
#include <string>

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/devices/IKeyboard.hpp>
#include "globals.hpp"

class CHyprEasyLabel : public IHyprWindowDecoration {
  public:
    CHyprEasyLabel(PHLWINDOW, SMotionActionDesc *actionDesc);
    virtual ~CHyprEasyLabel();

    virtual SDecorationPositioningInfo getPositioningInfo();

    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply);

    virtual void                       draw(PHLMONITOR, float a);

    virtual eDecorationType            getDecorationType();

    virtual void                       updateWindow(PHLWINDOW);

    virtual void                       damageEntire();

    virtual eDecorationLayer           getDecorationLayer();

    virtual uint64_t                   getDecorationFlags();

    bool                               m_bButtonsDirty = true;

    virtual std::string                getDisplayName();

    PHLWINDOW                           getOwner();

		std::string												 m_szLabel;
		std::string  											 m_szActionCmd;
		std::string												 m_szTextFont;
	  std::string                        m_szWindowAddress;
		int																 m_iTextSize;
		int																 m_iPaddingTop;
		int																 m_iPaddingBottom;
		int																 m_iPaddingLeft;
		int																 m_iPaddingRight;
		int 															 m_iRounding;

		CColor		  											 m_cTextColor;
		CColor														 m_cBackgroundColor;
		int																 m_iBorderSize;
		CGradientValueData								 m_cBorderGradient;


	


  private:
		int			layoutWidth;
		int     layoutHeight;
    SBoxExtents m_seExtents;

    PHLWINDOW                 m_pWindow;

    SP<CTexture>                 m_tTextTex;

    bool                     m_bWindowSizeChanged = false;

    void                     renderText(CTexture& out, const std::string& text, const CColor& color, const Vector2D& bufferSize, const float scale, const int fontSize);
    CBox                     assignedBoxGlobal();
		void 										 renderMotionString(Vector2D& bufferSize, const float scale);

    // for dynamic updates
    int m_iLastHeight = 0;
};
