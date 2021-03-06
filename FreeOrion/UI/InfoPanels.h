// -*- C++ -*-
#ifndef _InfoPanels_h_
#define _InfoPanels_h_

#include "../universe/Enums.h"
#include "../universe/Universe.h"
#include "CUIControls.h"

#include <GG/Button.h>
#include <GG/DropDownList.h>
#include <GG/BrowseInfoWnd.h>

class PopulationPanel;
class ResourcePanel;
class BuildingsPanel;
class BuildingIndicator;
class SpecialsPanel;
class MultiTurnProgressBar;
class MultiMeterStatusBar;
class MultiIconValueIndicator;
class Meter;
class ShipDesign;
class System;
class Planet;
class ResourceCenter;
class PopCenter;
class UniverseObject;
class BuildingType;
class StatisticIcon;
class CUIDropDownList;
namespace GG {
    class StaticGraphic;
}

/** Shows population with meter bars */
class PopulationPanel : public GG::Wnd {
public:
    /** \name Structors */ //@{
    PopulationPanel(GG::X w, int object_id);            ///< basic ctor
    ~PopulationPanel();
    //@}

    /** \name Accessors */ //@{
    int                 PopCenterID() const {return m_popcenter_id;}
    //@}

    /** \name Mutators */ //@{
    void                ExpandCollapse(bool expanded);  ///< expands or collapses panel to show details or just summary info

    virtual void        Render();
    virtual void        MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);
    virtual void        SizeMove(const GG::Pt& ul, const GG::Pt& lr);

    void                Update();                       ///< updates indicators with values of associated object.  Does not do layout and resizing.
    void                Refresh();                      ///< updates, redoes layout, resizes indicator

    /** Enables, or disables if \a enable is false, issuing orders via this PopulationPanel. */
    void                EnableOrderIssuing(bool enable = true);
    //@}

    mutable boost::signal<void ()> ExpandCollapseSignal;

private:
    void                ExpandCollapseButtonPressed();  ///< toggles panel expanded or collapsed

    void                DoExpandCollapseLayout();       ///< resizes panel and positions widgets according to present collapsed / expanded status

    const PopCenter*    GetPopCenter() const;           ///< returns the PopCenter object with id m_popcenter_id

    int                         m_popcenter_id;         ///< object id for the UniverseObject that is also a PopCenter which is being displayed in this panel

    StatisticIcon*              m_pop_stat;             ///< icon and number of population

    MultiIconValueIndicator*    m_multi_icon_value_indicator;   ///< textually / numerically indicates population
    MultiMeterStatusBar*        m_multi_meter_status_bar;       ///< graphically indicates meter values

    GG::Button*                 m_expand_button;        ///< at top right of panel, toggles the panel open/closed to show details or minimal summary

    static std::map<int, bool>  s_expanded_map;         ///< map indexed by popcenter ID indicating whether the PopulationPanel for each object is expanded (true) or collapsed (false)
};

/** Shows resource meters with meter-bars */
class ResourcePanel : public GG::Wnd {
public:
    /** \name Structors */ //@{
    ResourcePanel(GG::X w, int object_id);
    ~ResourcePanel();
    //@}

    /** \name Accessors */ //@{
    int             ResourceCenterID() const {return m_rescenter_id;}
    //@}

    /** \name Mutators */ //@{
    void            ExpandCollapse(bool expanded); ///< expands or collapses panel to show details or just summary info

    virtual void    Render();
    virtual void    MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);
    virtual void    SizeMove(const GG::Pt& ul, const GG::Pt& lr);

    void            Update();                       ///< updates indicators with values of associated object.  Does not do layout and resizing.
    void            Refresh();                      ///< updates, redoes layout, resizes indicator

    /** Enables, or disables if \a enable is false, issuing orders via this ResourcePanel. */
    void            EnableOrderIssuing(bool enable = true);
    //@}

    mutable boost::signal<void ()>                  ExpandCollapseSignal;
    mutable boost::signal<void (const std::string&)>FocusChangedSignal;

private:
    void            ExpandCollapseButtonPressed();  ///< toggles panel expanded or collapsed
    void            DoExpandCollapseLayout();       ///< resizes panel and positions widgets according to present collapsed / expanded status

    void            FocusDropListSelectionChanged(GG::DropDownList::iterator selected); ///< called when droplist selection changes, emits FocusChangedSignal

    int                         m_rescenter_id;         ///< object id for the UniverseObject that is also a PopCenter which is being displayed in this panel

    StatisticIcon*              m_pop_mod_stat;         ///< indicator of modifiers to other objects' population
    StatisticIcon*              m_mining_stat;          ///< icon and number of minerals production
    StatisticIcon*              m_industry_stat;        ///< icon and number of industry production
    StatisticIcon*              m_research_stat;        ///< icon and number of research production
    StatisticIcon*              m_trade_stat;           ///< icon and number of trade production

    MultiIconValueIndicator*    m_multi_icon_value_indicator;   ///< textually / numerically indicates resource production and construction meter
    MultiMeterStatusBar*        m_multi_meter_status_bar;       ///< graphically indicates meter values

    CUIDropDownList*            m_focus_drop;                   ///< displays and allows selection of primary focus

    GG::Button*                 m_expand_button;                ///< at top right of panel, toggles the panel open/closed to show details or minimal summary

    static std::map<int, bool>  s_expanded_map;                 ///< map indexed by popcenter ID indicating whether the PopulationPanel for each object is expanded (true) or collapsed (false)
};

/** Shows military-related meters including stealth, detection, shields, defense; with meter bars */
class MilitaryPanel : public GG::Wnd {
public:
    /** \name Structors */ //@{
    MilitaryPanel(GG::X w, int planet_id);
    ~MilitaryPanel();
    //@}

    /** \name Accessors */ //@{
    int                     PlanetID() const {return m_planet_id;}
    //@}

    /** \name Mutators */ //@{
    void                    ExpandCollapse(bool expanded);  ///< expands or collapses panel to show details or just summary info

    virtual void            Render();
    virtual void            MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);
    virtual void            SizeMove(const GG::Pt& ul, const GG::Pt& lr);

    void                    Update();                       ///< updates indicators with values of associated object.  Does not do layout and resizing.
    void                    Refresh();                      ///< updates, redoes layout, resizes indicator

    /** Enables, or disables if \a enable is false, issuing orders via this MilitaryPanel. */
    void                    EnableOrderIssuing(bool enable = true);
    //@}

    mutable boost::signal<void ()> ExpandCollapseSignal;

private:
    void                    ExpandCollapseButtonPressed();  ///< toggles panel expanded or collapsed
    void                    DoExpandCollapseLayout();       ///< resizes panel and positions widgets according to present collapsed / expanded status

    int                         m_planet_id;                ///< object id for the UniverseObject that this panel display info about

    StatisticIcon*              m_fleet_supply_stat;
    StatisticIcon*              m_shield_stat;
    StatisticIcon*              m_defense_stat;
    StatisticIcon*              m_troops_stat;
    StatisticIcon*              m_detection_stat;
    StatisticIcon*              m_stealth_stat;

    MultiIconValueIndicator*    m_multi_icon_value_indicator;   ///< textually / numerically indicates resource production and construction meter
    MultiMeterStatusBar*        m_multi_meter_status_bar;       ///< graphically indicates meter values

    GG::Button*                 m_expand_button;            ///< at top right of panel, toggles the panel open/closed to show details or minimal summary

    static std::map<int, bool>  s_expanded_map;             ///< map indexed by popcenter ID indicating whether the PopulationPanel for each object is expanded (true) or collapsed (false)
};

/** Contains various BuildingIndicator to represent buildings on a planet. */
class BuildingsPanel : public GG::Wnd {
public:
    /** \name Structors */ //@{
    BuildingsPanel(GG::X w, int columns, int planet_id);    ///< basic ctor
    ~BuildingsPanel();
    //@}

    /** \name Accessors */ //@{
    int             PlanetID() const {return m_planet_id;}
    //@}

    /** \name Mutators */ //@{
    void            ExpandCollapse(bool expanded);          ///< expands or collapses panel to show details or just summary info

    virtual void    Render();
    virtual void    MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);
    virtual void    SizeMove(const GG::Pt& ul, const GG::Pt& lr);

    void            Refresh();                              ///< recreates indicators, redoes layout, resizes

    /** Enables, or disables if \a enable is false, issuing orders via this BuildingsPanel. */
    void            EnableOrderIssuing(bool enable = true);
    //@}

    mutable boost::signal<void ()> ExpandCollapseSignal;

private:
    void            ExpandCollapseButtonPressed();          ///< toggles panel expanded or collapsed
    void            DoExpandCollapseLayout();               ///< resizes panel and positions indicators, differently depending on collapsed / expanded status

    void            Update();                               ///< recreates building indicators for building on or being built at this planet

    int                             m_planet_id;            ///< object id for the Planet whose buildings this panel displays
    int                             m_columns;              ///< number of columns in which to display building indicators
    std::vector<BuildingIndicator*> m_building_indicators;
    GG::Button*                     m_expand_button;        ///< at top right of panel, toggles the panel open/closed to show details or minimal summary

    static std::map<int, bool>      s_expanded_map;         ///< map indexed by planet ID indicating whether the BuildingsPanel for each object is expanded (true) or collapsed (false)
};

/** Represents and allows some user interaction with a building */
class BuildingIndicator : public GG::Wnd {
public:
    /** Constructor for use when building is completed, shown without progress 
      * bar. */
    BuildingIndicator(GG::X w, int building_id);
    /** Constructor for use when building is partially complete, to show
      * progress bar. */
    BuildingIndicator(GG::X w, const std::string& building_type, double turns_completed);

    /** \name Mutators */ //@{
    virtual void    Render();
    void            Refresh();

    virtual void    SizeMove(const GG::Pt& ul, const GG::Pt& lr);
    virtual void    MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);
    virtual void    RClick(const GG::Pt& pt, GG::Flags<GG::ModKey> mod_keys);

    /** Enables, or disables if \a enable is false, issuing orders via this BuildingIndicator. */
    void            EnableOrderIssuing(bool enable = true);
    //@}

private:
    GG::StaticGraphic*          m_graphic;
    GG::StaticGraphic*          m_scrap_indicator;  ///< shown to indicate building was ordered scrapped
    MultiTurnProgressBar*       m_progress_bar;
    int                         m_building_id;
    bool                        m_order_issuing_enabled;
};

/** Displays a set of specials attached to an UniverseObject */
class SpecialsPanel : public GG::Wnd {
public:
    /** \name Structors */ //@{
    SpecialsPanel(GG::X w, int object_id);   ///< basic ctor
    //@}

    /** \name Accessors */ //@{
    bool                    InWindow(const GG::Pt& pt) const;
    int                     ObjectID() const {return m_object_id;}
    //@}

    /** \name Mutators */ //@{
    virtual void            Render();
    virtual void            MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);
    virtual void            SizeMove(const GG::Pt& ul, const GG::Pt& lr);

    void                    Update();          ///< regenerates indicators according to buildings on planets and on queue on planet and redoes layout

    /** Enables, or disables if \a enable is false, issuing orders via this SpecialsPanel. */
    void                    EnableOrderIssuing(bool enable = true);

    //@}

private:
    int                             m_object_id;        ///< id for the Object whose specials this panel displays
    std::vector<GG::StaticGraphic*> m_icons;
};

/** Represents a ShipDesign */
class ShipDesignPanel : public GG::Control {
public:
    /** \name Structors */ //@{
    ShipDesignPanel(GG::X w, GG::Y h, int design_id);   ///< basic ctor
    //@}

    /** \name Accessors */ //@{
    int                     DesignID() const {return m_design_id;}
    //@}

    /** \name Mutators */ //@{
    virtual void            SizeMove(const GG::Pt& ul, const GG::Pt& lr);
    virtual void            Render();

    void                    Update();
    //@}

private:
    int                     m_design_id;        ///< id for the ShipDesign this panel displays

protected:
    GG::StaticGraphic*      m_graphic;
    GG::TextControl*        m_name;
};

/** Display icon and number for various meter-related quantities associated
  * with objects.  Typical use would be to display the resource production
  * values or population for a planet.  Given a set of MeterType, the indicator
  * will present the appropriate values for each. */
class MultiIconValueIndicator : public GG::Wnd {
public:
    MultiIconValueIndicator(GG::X w, int object_id, const std::vector<std::pair<MeterType, MeterType> >& meter_types);
    MultiIconValueIndicator(GG::X w, const std::vector<int>& object_ids, const std::vector<std::pair<MeterType, MeterType> >& meter_types);
    MultiIconValueIndicator(GG::X w); ///< initializes with no icons shown
    virtual ~MultiIconValueIndicator();

    bool            Empty();

    virtual void    Render();
    virtual void    MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);

    void            Update();

    void            SetToolTip(MeterType meter_type, const boost::shared_ptr<GG::BrowseInfoWnd>& browse_wnd);
    void            ClearToolTip(MeterType meter_type);

private:
    void            Init();

    std::vector<StatisticIcon*>                         m_icons;
    const std::vector<std::pair<MeterType, MeterType> > m_meter_types;
    std::vector<int>                                    m_object_ids;
};

/** Graphically represets current value and projected changes to single meters
  * or pairs of meters, using horizontal indicators. */
class MultiMeterStatusBar : public GG::Wnd {
public:
    MultiMeterStatusBar(GG::X w, int object_id, const std::vector<std::pair<MeterType, MeterType> >& meter_types);

    virtual void    Render();
    virtual void    MouseWheel(const GG::Pt& pt, int move, GG::Flags<GG::ModKey> mod_keys);

    void            Update();

private:
    boost::shared_ptr<GG::Texture>                  m_bar_shading_texture;

    std::vector<std::pair<MeterType, MeterType> >   m_meter_types;      // list of <MeterType, MeterType> pairs, where .first is the "actual" meter value, and .second is the the "target" or "max" meter value.  Either may be INVALID_METER_TYPE, in which case nothing will be shown for that MeterType
    std::vector<double>                             m_initial_values;   // initial value of .first MeterTypes at the start of this turn
    std::vector<double>                             m_projected_values; // projected current value of .first MeterTypes for the start of next turn
    std::vector<double>                             m_target_max_values;// current values of the .second MeterTypes in m_meter_types

    int                                             m_object_id;

    std::vector<GG::Clr>                            m_bar_colours;
};

/** Shows a summary of meter modifications associated with a particular source
  * object. */
class MeterModifiersIndicator : public StatisticIcon {
public:
    MeterModifiersIndicator(GG::X x, GG::Y y, GG::X w, GG::Y h, int source_object_id, MeterType meter_type);
    MeterModifiersIndicator(GG::X x, GG::Y y, GG::X w, GG::Y h, int source_object_id, const std::string& empire_meter_type);

    /** \name Mutators */ //@{
    void            Update();
    //@}

private:
    int             m_source_object_id;
    MeterType       m_meter_type;
    std::string     m_empire_meter_type;
};

/** A popup tooltop for display when mousing over in-game icons.  Has an icon and title and some detail text.*/
class IconTextBrowseWnd : public GG::BrowseInfoWnd {
public:
    IconTextBrowseWnd(const boost::shared_ptr<GG::Texture> texture, const std::string& title_text,
                      const std::string& main_text);
    virtual bool WndHasBrowseInfo(const Wnd* wnd, std::size_t mode) const;
    virtual void Render();

private:
    GG::StaticGraphic*  m_icon;
    GG::TextControl*    m_title_text;
    GG::TextControl*    m_main_text;
};

/** Gives information about inporting and exporting of resources to and from this system when mousing
  * over the system resource production summary. */
class SystemResourceSummaryBrowseWnd : public GG::BrowseInfoWnd {
public:
    SystemResourceSummaryBrowseWnd(ResourceType resource_type, int system_id, int empire_id = ALL_EMPIRES);

    virtual bool    WndHasBrowseInfo(const Wnd* wnd, std::size_t mode) const;

    virtual void    Render();

private:
    void            Clear();
    void            Initialize();
    virtual void    UpdateImpl(std::size_t mode, const GG::Wnd* target);

    void            UpdateProduction(GG::Y& top);   // adds pairs of TextControl for ResourceCenter name and production of resource starting at vertical position \a top and updates \a top to the vertical position after the last entry
    void            UpdateAllocation(GG::Y& top);   // adds pairs of TextControl for allocation of resources in system, starting at vertical position \a top and updates \a top to be the vertical position after the last entry
    void            UpdateImportExport(GG::Y& top); // sets m_import_export_label and m_import_export text and amount to indicate how much resource is being imported or exported from this system, and moves them to vertical position \a top and updates \a top to be the vertical position below these labels

    ResourceType        m_resource_type;
    int                 m_system_id;
    int                 m_empire_id;

    double              m_production;               // set by UpdateProduction - used to store production in system so that import / export / unused can be more easily calculated
    double              m_allocation;               // set by UpdateAllocation - used like m_production

    GG::TextControl*    m_production_label;
    GG::TextControl*    m_allocation_label;
    GG::TextControl*    m_import_export_label;

    std::vector<std::pair<GG::TextControl*, GG::TextControl*> > m_production_labels_and_amounts;
    std::vector<std::pair<GG::TextControl*, GG::TextControl*> > m_allocation_labels_and_amounts;
    std::vector<std::pair<GG::TextControl*, GG::TextControl*> > m_import_export_labels_and_amounts;

    GG::Y               row_height;

    GG::Y               production_label_top;
    GG::Y               allocation_label_top;
    GG::Y               import_export_label_top;
};

/** Gives details about what effects contribute to a meter's maximum value (Effect Accounting) and
  * shows the current turn's current meter value and the predicted current meter value for next turn. */
class MeterBrowseWnd : public GG::BrowseInfoWnd {
public:
    MeterBrowseWnd(int object_id, MeterType primary_meter_type, MeterType secondary_meter_type = INVALID_METER_TYPE);

    virtual bool    WndHasBrowseInfo(const Wnd* wnd, std::size_t mode) const;
    virtual void    Render();

private:
    void            Initialize();

    virtual void    UpdateImpl(std::size_t mode, const Wnd* target);
    void            UpdateSummary();
    void            UpdateEffectLabelsAndValues(GG::Y& top);

    MeterType               m_primary_meter_type;
    MeterType               m_secondary_meter_type;
    int                     m_object_id;

    GG::TextControl*        m_summary_title;

    GG::TextControl*        m_current_label;
    GG::TextControl*        m_current_value;
    GG::TextControl*        m_next_turn_label;
    GG::TextControl*        m_next_turn_value;
    GG::TextControl*        m_change_label;
    GG::TextControl*        m_change_value;

    GG::TextControl*        m_meter_title;

    std::vector<std::pair<GG::TextControl*, GG::TextControl*> >
                            m_effect_labels_and_values;

    GG::Y                   m_row_height;
    bool                    m_initialized;
};

#endif
