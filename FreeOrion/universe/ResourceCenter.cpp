#include "ResourceCenter.h"

#include "../util/AppInterface.h"
#include "../universe/Building.h"
#include "../util/DataTable.h"
#include "../Empire/Empire.h"
#include "Fleet.h"
#include "../util/MultiplayerCommon.h"
#include "Planet.h"
#include "../universe/ShipDesign.h"
#include "System.h"
#include "../util/OptionsDB.h"

#include <boost/lexical_cast.hpp>

#include <stdexcept>

using boost::lexical_cast;

namespace {
    DataTableMap& ProductionDataTables()
    {
        static DataTableMap map;
        if (map.empty()) {
            std::string settings_dir = GetOptionsDB().Get<std::string>("settings-dir");
            if (!settings_dir.empty() && settings_dir[settings_dir.size() - 1] != '/')
                settings_dir += '/';

            LoadDataTables(settings_dir + "production_tables.txt", map);
        }
        return map;
    }

    DataTableMap& PlanetDataTables()
    {
        static DataTableMap map;
        if (map.empty()) {
            std::string settings_dir = GetOptionsDB().Get<std::string>("settings-dir");
            if (!settings_dir.empty() && settings_dir[settings_dir.size() - 1] != '/')
                settings_dir += '/';

            LoadDataTables(settings_dir + "planet_tables.txt", map);
        }
        return map;
    }

    double MaxFarmingModFromObject(const UniverseObject* object)
    {
        double retval = 0.0;
        if (const Planet* planet = universe_object_cast<const Planet*>(object)) {
            retval = PlanetDataTables()["PlanetEnvFarmingMod"][0][planet->Environment()];
        }
        return retval;
    }

    double MaxIndustryModFromObject(const UniverseObject* object)
    {
        double retval = 0.0;
        if (const Planet* planet = universe_object_cast<const Planet*>(object)) {
            retval = PlanetDataTables()["PlanetSizeIndustryMod"][0][planet->Environment()];
        }
        return retval;
    }

    void GrowResourceMeter(Meter* resource_meter, double updated_current_construction)
    {
        assert(resource_meter);
        double delta = updated_current_construction / (10.0 + resource_meter->Current());
        double new_cur = std::min(resource_meter->Max(), resource_meter->Current() + delta);
        resource_meter->SetCurrent(new_cur);
    }

    void GrowConstructionMeter(Meter* construction_meter, double updated_current_population)
    {
        assert(construction_meter);
        double cur = construction_meter->Current();
        double max = construction_meter->Max();
        double delta = (cur + 1) * ((max - cur) / (max + 1)) * (updated_current_population * 10.0) * 0.01;
        double new_cur = std::min(max, cur + delta);
        construction_meter->SetCurrent(new_cur);
    }
}

ResourceCenter::ResourceCenter()
{
}

ResourceCenter::~ResourceCenter()
{
}

void ResourceCenter::Init()
{
    InsertMeter(METER_FARMING, Meter());
    InsertMeter(METER_MINING, Meter());
    InsertMeter(METER_INDUSTRY, Meter());
    InsertMeter(METER_RESEARCH, Meter());
    InsertMeter(METER_TRADE, Meter());
    InsertMeter(METER_CONSTRUCTION, Meter());
    Reset();
}

double ResourceCenter::ProjectedCurrentMeter(MeterType type) const
{
    Meter construction = Meter(*GetMeter(METER_CONSTRUCTION));
    const Meter* original_meter = GetMeter(type);
    assert(original_meter);
    Meter meter = Meter(*original_meter);

    switch (type) {
    case METER_FARMING:
    case METER_MINING:
    case METER_INDUSTRY:
    case METER_RESEARCH:
    case METER_TRADE:
        GrowConstructionMeter(&construction, GetPopMeter()->Current());
        GrowResourceMeter(&meter, construction.Current());
        return meter.Current();
        break;
    case METER_CONSTRUCTION:
        GrowConstructionMeter(&construction, GetPopMeter()->Current());
        return construction.Current();
        break;
    default:
        const UniverseObject* obj = dynamic_cast<const UniverseObject*>(this);
        if (obj)
            return obj->ProjectedCurrentMeter(type);
        else
            throw std::runtime_error("ResourceCenter::ProjectedCurrentMeter couldn't convert this pointer to UniverseObject*");
    }
}

double ResourceCenter::MeterPoints(MeterType type) const
{
    switch (type) {
    case METER_FARMING:
    case METER_MINING:
    case METER_INDUSTRY:
    case METER_RESEARCH:
    case METER_TRADE:
        return GetPopMeter()->Current() / 10.0 * GetMeter(type)->Current();
        break;
    case METER_CONSTRUCTION:
        return GetMeter(METER_CONSTRUCTION)->Current();
        break;
    default:
        const UniverseObject* obj = dynamic_cast<const UniverseObject*>(this);
        if (obj)
            return obj->MeterPoints(type);
        else
            throw std::runtime_error("ResourceCenter::ProjectedCurrentMeter couldn't convert this pointer to UniverseObject*");
    }
}

double ResourceCenter::ProjectedMeterPoints(MeterType type) const
{
    switch (type) {
    case METER_FARMING:
    case METER_MINING:
    case METER_INDUSTRY:
    case METER_RESEARCH:
    case METER_TRADE:
        // TODO: get projected current population instead of using just current population
        return GetPopMeter()->Current() / 10.0 * ProjectedCurrentMeter(type);
        break;
    case METER_CONSTRUCTION:
        return ProjectedCurrentMeter(METER_CONSTRUCTION);
        break;
    default:
        const UniverseObject* obj = dynamic_cast<const UniverseObject*>(this);
        if (obj)
            return obj->ProjectedMeterPoints(type);
        else
            throw std::runtime_error("ResourceCenter::ProjectedCurrentMeter couldn't convert this pointer to UniverseObject*");
    }
}

void ResourceCenter::SetPrimaryFocus(FocusType focus)
{
    m_primary = focus;
    ResourceCenterChangedSignal();
}

void ResourceCenter::SetSecondaryFocus(FocusType focus)
{
    m_secondary = focus;
    ResourceCenterChangedSignal();
}

void ResourceCenter::ApplyUniverseTableMaxMeterAdjustments(MeterType meter_type)
{
    // determine meter maxes; they should have been previously reset to 0
    double primary_specialized_factor = ProductionDataTables()["FocusMods"][0][0];
    double secondary_specialized_factor = ProductionDataTables()["FocusMods"][1][0];
    double primary_balanced_factor = ProductionDataTables()["FocusMods"][2][0];
    double secondary_balanced_factor = ProductionDataTables()["FocusMods"][3][0];

    UniverseObject* object = GetObjectSignal();
    assert(object);

    // special cases for construction, farming and industry
    if (meter_type == INVALID_METER_TYPE || meter_type == METER_CONSTRUCTION)
        GetMeter(METER_CONSTRUCTION)->AdjustMax(10.0); // default construction max is 20
    if (meter_type == INVALID_METER_TYPE || meter_type == METER_FARMING)
        GetMeter(METER_FARMING)->AdjustMax(MaxFarmingModFromObject(object));
    if (meter_type == INVALID_METER_TYPE || meter_type == METER_INDUSTRY)
        GetMeter(METER_INDUSTRY)->AdjustMax(MaxIndustryModFromObject(object));

    // general-cases for all resource meters
    std::vector<MeterType> res_meter_types;
    res_meter_types.push_back(METER_FARMING);   res_meter_types.push_back(METER_MINING);    res_meter_types.push_back(METER_INDUSTRY);
    res_meter_types.push_back(METER_RESEARCH);  res_meter_types.push_back(METER_TRADE);

    // all meters matching parameter meter_type should be adjusted, depending on focus
    for (unsigned int i = 0; i < res_meter_types.size(); ++i) {
        const MeterType CUR_METER_TYPE = res_meter_types[i];

        if (meter_type == INVALID_METER_TYPE || meter_type == CUR_METER_TYPE) {
            Meter* meter = GetMeter(CUR_METER_TYPE);

            if (m_primary == MeterToFocus(CUR_METER_TYPE))
                meter->AdjustMax(primary_specialized_factor);
            else if (m_primary == FOCUS_BALANCED)
                meter->AdjustMax(primary_balanced_factor);

            if (m_secondary == MeterToFocus(CUR_METER_TYPE))
                meter->AdjustMax(secondary_specialized_factor);
            else if (m_secondary == FOCUS_BALANCED)
                meter->AdjustMax(secondary_balanced_factor);
        }
    }
}

void ResourceCenter::PopGrowthProductionResearchPhase()
{
    GrowConstructionMeter(GetMeter(METER_CONSTRUCTION), GetPopMeter()->Current());
    double new_current_construction = GetMeter(METER_CONSTRUCTION)->Current();
    GrowResourceMeter(GetMeter(METER_FARMING),  new_current_construction);
    GrowResourceMeter(GetMeter(METER_INDUSTRY), new_current_construction);
    GrowResourceMeter(GetMeter(METER_MINING),   new_current_construction);
    GrowResourceMeter(GetMeter(METER_RESEARCH), new_current_construction);
    GrowResourceMeter(GetMeter(METER_TRADE),    new_current_construction);
}

void ResourceCenter::Reset()
{
    m_primary = FOCUS_UNKNOWN;
    m_secondary = FOCUS_UNKNOWN;

    GetMeter(METER_FARMING)->Reset();
    GetMeter(METER_INDUSTRY)->Reset();
    GetMeter(METER_MINING)->Reset();
    GetMeter(METER_RESEARCH)->Reset();
    GetMeter(METER_TRADE)->Reset();
    GetMeter(METER_CONSTRUCTION)->Reset();

    double balanced_balanced_max = ProductionDataTables()["FocusMods"][2][0] + ProductionDataTables()["FocusMods"][3][0];
    GetMeter(METER_FARMING)->SetMax(balanced_balanced_max);
    GetMeter(METER_INDUSTRY)->SetMax(balanced_balanced_max);
    GetMeter(METER_MINING)->SetMax(balanced_balanced_max);
    GetMeter(METER_RESEARCH)->SetMax(balanced_balanced_max);
    GetMeter(METER_TRADE)->SetMax(balanced_balanced_max);
}
