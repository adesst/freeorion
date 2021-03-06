// -*- C++ -*-
#ifndef _TechTreeWnd_h_
#define _TechTreeWnd_h_

#include <GG/Wnd.h>
#include "../universe/Enums.h"

class Tech;
class EncyclopediaDetailPanel;

/** Contains the tech graph layout, some controls to control what is visible in
  * the tech layout, the tech navigator, and the tech detail window. */
class TechTreeWnd : public GG::Wnd
{
public:
    /** \name Signal Types */ //@{
    typedef boost::signal<void (const std::string&)>                TechBrowsedSignalType;              ///< emitted when a technology is single-clicked
    typedef boost::signal<void (const std::string&)>                TechClickedSignalType;              ///< emitted when the mouse rolls over a technology
    typedef boost::signal<void (const std::string&)>                TechDoubleClickedSignalType;        ///< emitted when a technology is double-clicked
    typedef boost::signal<void (const std::vector<std::string>&)>   AddMultipleTechsToQueueSignalType;  ///< emitted to enqueue multiple techs simultaneously, without updating the GUI after each
    //@}

    /** \name Structors */ //@{
    TechTreeWnd(GG::X w, GG::Y h);
    ~TechTreeWnd();
    //@}

    /** \name Accessors */ //@{
    double                  Scale() const;
    std::set<std::string>   GetCategoriesShown() const;
    std::set<TechType>      GetTechTypesShown() const;
    std::set<TechStatus>    GetTechStatusesShown() const;
    //@}

    //! \name Mutators //@{
    void                    Update();
    void                    Clear();
    void                    Reset();
    void                    SetScale(double scale);

    void                    ShowCategory(const std::string& category);
    void                    ShowAllCategories();
    void                    HideCategory(const std::string& category);
    void                    HideAllCategories();
    void                    ToggleCategory(const std::string& category);
    void                    ToggleAllCategories();

    void                    ShowStatus(const TechStatus status);
    void                    HideStatus(const TechStatus status);
    void                    ToggleStatus(const TechStatus status);

    void                    ShowType(const TechType type);
    void                    HideType(const TechType type);
    void                    ToggleType(const TechType type);

    void                    ShowTreeView();
    void                    ShowListView();

    void                    CenterOnTech(const std::string& tech_name);
    void                    SetEncyclopediaTech(const std::string& tech_name);
    void                    SelectTech(const std::string& tech_name);
    //@}

    static const GG::Y          NAVIGATOR_AND_DETAIL_HEIGHT;

    mutable TechBrowsedSignalType               TechBrowsedSignal;
    mutable TechClickedSignalType               TechSelectedSignal;
    mutable TechDoubleClickedSignalType         AddTechToQueueSignal;
    mutable AddMultipleTechsToQueueSignalType   AddMultipleTechsToQueueSignal;

private:
    class TechTreeControls;
    class TechNavigator;
    class LayoutPanel;
    class TechListBox;

    void                    TechBrowsedSlot(const std::string& tech_name);
    void                    TechClickedSlot(const std::string& tech_name);
    void                    TechDoubleClickedSlot(const std::string& tech_name);

    TechTreeControls*           m_tech_tree_controls;
    EncyclopediaDetailPanel*    m_enc_detail_panel;
    TechNavigator*              m_tech_navigator;
    LayoutPanel*                m_layout_panel;
    TechListBox*                m_tech_list;
};

#endif // _TechTreeWnd_h_
