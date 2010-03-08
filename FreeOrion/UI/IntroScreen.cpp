//IntroScreen.cpp
#include "IntroScreen.h"

#include "../client/human/HumanClientApp.h"

#include "About.h"
#include "ClientUI.h"
#include "CUIControls.h"
#include "../util/Directories.h"
#include "../network/Message.h"
#include "../util/MultiplayerCommon.h"
#include "OptionsWnd.h"
#include "../util/OptionsDB.h"
#include "../util/Serialize.h"
#include "../util/Version.h"

#include <GG/GUI.h>
#include <GG/DrawUtil.h>
#include <GG/StaticGraphic.h>
#include <GG/Texture.h>

#include <boost/filesystem/fstream.hpp>

#include <cstdlib>
#include <fstream>
#include <string>

namespace {
    const GG::X MAIN_MENU_WIDTH(200);
    const GG::Y MAIN_MENU_HEIGHT(380);

    void Options(OptionsDB& db)
    {
        db.AddFlag("force-external-server",             "OPTIONS_DB_FORCE_EXTERNAL_SERVER",     false);
        db.Add<std::string>("external-server-address",  "OPTIONS_DB_EXTERNAL_SERVER_ADDRESS",   "localhost");
        db.Add("UI.main-menu.x",                        "OPTIONS_DB_UI_MAIN_MENU_X",            0.75,   RangedStepValidator<double>(0.01, 0.0, 1.0));
        db.Add("UI.main-menu.y",                        "OPTIONS_DB_UI_MAIN_MENU_Y",            0.5,    RangedStepValidator<double>(0.01, 0.0, 1.0));

        db.Add("checked-gl-version",                    "OPTIONS_DB_CHECKED_GL_VERSION",        false);
    }
    bool foo_bool = RegisterOptions(&Options);

    void CheckGLVersion() {
        // only execute once
        if (GetOptionsDB().Get<bool>("checked-gl-version"))
            return;
        GetOptionsDB().Set<bool>("checked-gl-version", true);


        // get OpenGL version string and parse to get version number
        const GLubyte* gl_version = glGetString(GL_VERSION);
        std::string gl_version_string = boost::lexical_cast<std::string>(gl_version);
        Logger().debugStream() << "OpenGL version string: " << boost::lexical_cast<std::string>(gl_version);

        float version_number = 0.0;
        std::istringstream iss(gl_version_string);
        iss >> version_number;
        version_number += 0.05f;    // ensures proper rounding of 1.1 digit number

        Logger().debugStream() << "...extracted version number: " << DoubleToString(version_number, 2, false);    // combination of floating point precision and DoubleToString preferring to round down means the +0.05 is needed to round properly

        // if GL version is too low, set various map rendering options to
        // disabled, so as to prevent crashes when running on systems that
        // don't support these GL features
        if (true/*version_number < 2.0*/) {
            GetOptionsDB().Set<bool>("UI.galaxy-gas-background",        false);
            GetOptionsDB().Set<bool>("UI.galaxy-starfields",            false);
            GetOptionsDB().Set<bool>("UI.optimized-system-rendering",   false);
            GetOptionsDB().Set<bool>("UI.system-fog-of-war",            false);
        }
    }
}

/////////////////////////////////
// CreditsWnd
/////////////////////////////////

/** Displays scrolling credits. */
class CreditsWnd : public GG::Wnd
{
public:
    CreditsWnd(GG::X x, GG::Y y, GG::X w, GG::Y h,const XMLElement &credits,int cx, int cy, int cw, int ch,int co);
    ~CreditsWnd();

    virtual void    Render();
    virtual void    LClick(const GG::Pt& pt, GG::Flags<GG::ModKey> mod_keys) { StopRendering(); }

private:
    void            DrawCredits(GG::X x1, GG::Y y1, GG::X x2, GG::Y y2, int transparency);
    void            StopRendering();

    XMLElement                  m_credits;
    int                         m_cx, m_cy, m_cw, m_ch, m_co;
    int                         m_start_time;
    int                         m_bRender;
    int                         m_displayListID;
    int                         m_creditsHeight;
    boost::shared_ptr<GG::Font> m_font;
};

CreditsWnd::CreditsWnd(GG::X x, GG::Y y, GG::X w, GG::Y h,const XMLElement &credits,int cx, int cy, int cw, int ch,int co) :
    GG::Wnd(x, y, w, h, GG::INTERACTIVE),
    m_credits(credits),
    m_cx(cx),
    m_cy(cy),
    m_cw(cw),
    m_ch(ch),
    m_co(co),
    m_start_time(GG::GUI::GetGUI()->Ticks()),
    m_bRender(true),m_displayListID(0),
    m_creditsHeight(0)
{
    m_font = ClientUI::GetFont(static_cast<int>(ClientUI::Pts()*1.3));
}

CreditsWnd::~CreditsWnd() {
   if(m_displayListID != 0) {
      glDeleteLists(m_displayListID, 1);
   }
}

void CreditsWnd::StopRendering() {
    m_bRender=false;
    if(m_displayListID != 0) {
        glDeleteLists(m_displayListID, 1);
        m_displayListID = 0;
    }
}

void CreditsWnd::DrawCredits(GG::X x1, GG::Y y1, GG::X x2, GG::Y y2, int transparency)
{
    GG::Flags<GG::TextFormat> format = GG::FORMAT_CENTER | GG::FORMAT_TOP;

    //offset starts with 0, credits are place by transforming the viewport
    GG::Y offset(0);

    //start color is white (name), this color is valid outside the rgba tags
    glColor(GG::Clr(transparency, transparency, transparency, 255));

    std::string credit;
    for(int i = 0; i<m_credits.NumChildren();i++) {
        if(0==m_credits.Child(i).Tag().compare("GROUP"))
        {
            XMLElement group = m_credits.Child(i);
            for(int j = 0; j<group.NumChildren();j++) {
                if(0==group.Child(j).Tag().compare("PERSON"))
                {
                    XMLElement person = group.Child(j);
                    credit = "";
                    if(person.ContainsAttribute("name"))
                        credit+=person.Attribute("name");
                    if(person.ContainsAttribute("nick") && person.Attribute("nick").length()>0)
                    {
                        credit+=" <rgba 153 153 153 " + boost::lexical_cast<std::string>(transparency) +">(";
                        credit+=person.Attribute("nick");
                        credit+=")</rgba>";
                    }
                    if(person.ContainsAttribute("task") && person.Attribute("task").length()>0)
                    {
                        credit+=" - <rgba 204 204 204 " + boost::lexical_cast<std::string>(transparency) +">";
                        credit+=person.Attribute("task");
                        credit+="</rgba>";
                    }
                    m_font->RenderText(GG::Pt(x1, y1+offset),
                        GG::Pt(x2,y2),
                        credit, format, 0);
                    offset+=m_font->TextExtent(credit, format).y+2;
                }
            }
            offset+=m_font->Lineskip()+2;
        }
    }
    //store complete height for self destruction
    m_creditsHeight = Value(offset);
}

void CreditsWnd::Render()
{
    if(!m_bRender)
        return;
    GG::Pt ul = UpperLeft(), lr = LowerRight();
    if (m_displayListID == 0) {
        // compile credits
        m_displayListID = glGenLists(1);
        glNewList(m_displayListID, GL_COMPILE);
        DrawCredits(ul.x+m_cx, ul.y+m_cy, ul.x+m_cx+m_cw, ul.y+m_cy+m_ch, 255);
        glEndList();
    }
    //time passed
    int passedTicks = GG::GUI::GetGUI()->Ticks() - m_start_time;

    //draw background
    GG::FlatRectangle(ul,lr,GG::FloatClr(0.0,0.0,0.0,0.5),GG::CLR_ZERO,0);

    glPushAttrib(GL_ALL_ATTRIB_BITS ); // ***SAVE***
    glPushMatrix();                    // attributes and transformation matrix

    // define clip area
    glEnable(GL_SCISSOR_TEST);
    glScissor(Value(ul.x+m_cx), Value(GG::GUI::GetGUI()->AppHeight()- lr.y), m_cw, m_ch);

    // move credits
    glTranslatef(
        0,
        m_co + passedTicks / -40.0,
        0
        );
    if (m_displayListID != 0) {
        // draw credits using prepared display list
        // !!! in order for the display list to be valid, the font object (m_font) may not be destroyed !!!
        glCallList(m_displayListID);
    } else {
        // draw credits directly
        DrawCredits(ul.x+m_cx, ul.y+m_cy, ul.x+m_cx+m_cw, ul.y+m_cy+m_ch, 255);
    }

    glPopMatrix();              // ***RESTORE***             
    glPopAttrib();              // attributes and transformation matrix

    //check if we are done
    if (m_creditsHeight + m_ch < m_co + passedTicks / 40.0) {
        StopRendering();
    }
}


/////////////////////////////////
// IntroScreen
/////////////////////////////////
IntroScreen::IntroScreen() :
    CUIWnd(UserString("INTRO_WINDOW_TITLE"), 
           static_cast<GG::X>(GG::GUI::GetGUI()->AppWidth() * GetOptionsDB().Get<double>("UI.main-menu.x") - MAIN_MENU_WIDTH / 2),
           static_cast<GG::Y>(GG::GUI::GetGUI()->AppHeight() * GetOptionsDB().Get<double>("UI.main-menu.y") - MAIN_MENU_HEIGHT / 2),
           MAIN_MENU_WIDTH, MAIN_MENU_HEIGHT, GG::ONTOP | GG::INTERACTIVE),
    m_credits_wnd(0),
    m_splash(new GG::StaticGraphic(GG::X0, GG::Y0, GG::GUI::GetGUI()->AppWidth(), GG::GUI::GetGUI()->AppHeight(),
                                   ClientUI::GetTexture(ClientUI::ArtDir() / "splash.png"),
                                   GG::GRAPHIC_FITGRAPHIC, GG::INTERACTIVE)),
    m_logo(new GG::StaticGraphic(GG::X0, GG::Y0, GG::GUI::GetGUI()->AppWidth(), GG::GUI::GetGUI()->AppHeight() / 10,
                                 ClientUI::GetTexture(ClientUI::ArtDir() / "logo.png"),
                                 GG::GRAPHIC_FITGRAPHIC | GG::GRAPHIC_PROPSCALE)),
    m_version(new GG::TextControl(GG::X0, GG::Y0, FreeOrionVersionString(),
                                  ClientUI::GetFont(),
                                  ClientUI::TextColor()))
{
    CheckGLVersion();

    m_splash->AttachChild(m_logo);
    GG::GUI::GetGUI()->Register(m_splash);

    m_version->MoveTo(GG::Pt(GG::GUI::GetGUI()->AppWidth() -  m_version->Size().x,
                             GG::GUI::GetGUI()->AppHeight() - m_version->Size().y));

    //create buttons
    m_single_player =   new CUIButton(GG::X(15), GG::Y(12),  GG::X(160), UserString("INTRO_BTN_SINGLE_PLAYER"));
    m_quick_start =     new CUIButton(GG::X(15), GG::Y(52),  GG::X(160), UserString("INTRO_BTN_QUICK_START"));
    m_multi_player =    new CUIButton(GG::X(15), GG::Y(92),  GG::X(160), UserString("INTRO_BTN_MULTI_PLAYER"));
    m_load_game =       new CUIButton(GG::X(15), GG::Y(132), GG::X(160), UserString("INTRO_BTN_LOAD_GAME"));
    m_options =         new CUIButton(GG::X(15), GG::Y(172), GG::X(160), UserString("INTRO_BTN_OPTIONS"));
    m_about =           new CUIButton(GG::X(15), GG::Y(212), GG::X(160), UserString("INTRO_BTN_ABOUT"));
    m_credits =         new CUIButton(GG::X(15), GG::Y(252), GG::X(160), UserString("INTRO_BTN_CREDITS"));
    m_exit_game =       new CUIButton(GG::X(15), GG::Y(322), GG::X(160), UserString("INTRO_BTN_EXIT"));

    //attach buttons
    AttachChild(m_single_player);
    AttachChild(m_quick_start);
    AttachChild(m_multi_player);
    AttachChild(m_load_game);
    AttachChild(m_options);
    AttachChild(m_about);
    AttachChild(m_credits);
    AttachChild(m_exit_game);

    //connect signals and slots
    GG::Connect(m_single_player->ClickedSignal, &IntroScreen::OnSinglePlayer,   this);
    GG::Connect(m_quick_start->ClickedSignal,   &IntroScreen::OnQuickStart,     this);
    GG::Connect(m_multi_player->ClickedSignal,  &IntroScreen::OnMultiPlayer,    this);
    GG::Connect(m_load_game->ClickedSignal,     &IntroScreen::OnLoadGame,       this);
    GG::Connect(m_options->ClickedSignal,       &IntroScreen::OnOptions,        this);
    GG::Connect(m_about->ClickedSignal,         &IntroScreen::OnAbout,          this);
    GG::Connect(m_credits->ClickedSignal,       &IntroScreen::OnCredits,        this);
    GG::Connect(m_exit_game->ClickedSignal,     &IntroScreen::OnExitGame,       this);
}

IntroScreen::~IntroScreen()
{
    delete m_credits_wnd;
    delete m_logo;
    delete m_splash;
    delete m_version;
}

void IntroScreen::OnSinglePlayer()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;
    HumanClientApp::GetApp()->NewSinglePlayerGame();
}

void IntroScreen::OnQuickStart()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;
    HumanClientApp::GetApp()->NewSinglePlayerGame(true);
}

void IntroScreen::OnMultiPlayer()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;
    HumanClientApp::GetApp()->MulitplayerGame();
}

void IntroScreen::OnLoadGame()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;
    HumanClientApp::GetApp()->LoadSinglePlayerGame();
}

void IntroScreen::OnOptions()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;

    OptionsWnd options_wnd;
    options_wnd.Run();
}

void IntroScreen::OnAbout()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;

    About about_wnd;
    about_wnd.Run();
}

void IntroScreen::OnCredits()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;


    XMLDoc doc;
    boost::filesystem::ifstream ifs(GetResourceDir() / "credits.xml");
    doc.ReadDoc(ifs);
    ifs.close();


    if (!doc.root_node.ContainsChild("CREDITS"))
        return;

    XMLElement credits = doc.root_node.Child("CREDITS");
    // only the area between the upper and lower line of the splash screen should be darkend
    // if we use another splash screen we have the change the following values
    GG::Y nUpperLine = ( 79 * GG::GUI::GetGUI()->AppHeight()) / 768;
    GG::Y nLowerLine = (692 * GG::GUI::GetGUI()->AppHeight()) / 768;

    m_credits_wnd = new CreditsWnd(GG::X0, nUpperLine, GG::GUI::GetGUI()->AppWidth(), nLowerLine-nUpperLine,
                                   credits, 60, 0, 600,
                                   Value(nLowerLine-nUpperLine), Value((nLowerLine-nUpperLine))/2);

    GG::GUI::GetGUI()->Register(m_credits_wnd);
}

void IntroScreen::OnExitGame()
{
    delete m_credits_wnd;
    m_credits_wnd = 0;

    GG::GUI::GetGUI()->Exit(0);
}

void IntroScreen::KeyPress(GG::Key key, boost::uint32_t key_code_point, GG::Flags<GG::ModKey> mod_keys)
{
    if (key == GG::GGK_ESCAPE)
        OnExitGame();
}

void IntroScreen::Close()
{ OnExitGame(); }

void IntroScreen::Render()
{
    CUIWnd::Render();
    m_version->Render();
}
