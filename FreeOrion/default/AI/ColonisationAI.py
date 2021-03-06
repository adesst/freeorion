import freeOrionAIInterface as fo
import FreeOrionAI as foAI
import AIstate
import FleetUtilsAI
from EnumsAI import AIFleetMissionType, AIExplorableSystemType, AITargetType
import AITarget
import PlanetUtilsAI

def getColonyFleets():
    "get colony fleets"

    allColonyFleetIDs = FleetUtilsAI.getEmpireFleetIDsByRole(AIFleetMissionType.FLEET_MISSION_COLONISATION)
    AIstate.colonyFleetIDs = FleetUtilsAI.extractFleetIDsWithoutMissionTypes(allColonyFleetIDs)

    # get suppliable systems and planets
    universe = fo.getUniverse()
    empire = fo.getEmpire()
    empireID = empire.empireID
    capitalID = empire.capitalID
    homeworld = universe.getPlanet(capitalID)
    speciesName = homeworld.speciesName
    species = fo.getSpecies(speciesName)

    fleetSupplyableSystemIDs = empire.fleetSupplyableSystemIDs
    fleetSupplyablePlanetIDs = PlanetUtilsAI.getPlanetsInSystemsIDs(fleetSupplyableSystemIDs)
    print ""
    print "    fleetSupplyableSystemIDs: " + str(list(fleetSupplyableSystemIDs))
    print "    fleetSupplyablePlanetIDs: " + str(fleetSupplyablePlanetIDs)
    print ""

    # get outpost and colonization planets
    exploredSystemIDs = empire.exploredSystemIDs
    print "Explored SystemIDs: " + str(list(exploredSystemIDs))

    exploredPlanetIDs = PlanetUtilsAI.getPlanetsInSystemsIDs(exploredSystemIDs)
    print "Explored PlanetIDs: " + str(exploredPlanetIDs)
    print ""

    allOwnedPlanetIDs = PlanetUtilsAI.getAllOwnedPlanetIDs(exploredPlanetIDs)
    print "All Owned and Populated PlanetIDs: " + str(allOwnedPlanetIDs)

    empireOwnedPlanetIDs = PlanetUtilsAI.getOwnedPlanetsByEmpire(universe.planetIDs, empireID)
    print "Empire Owned PlanetIDs:            " + str(empireOwnedPlanetIDs)

    unpopulatedPlanetIDs = list(set(exploredPlanetIDs) -set(allOwnedPlanetIDs))
    print "Unpopulated PlanetIDs:             " + str(unpopulatedPlanetIDs)

    print ""
    print "Colony Targeted SystemIDs:         " + str(AIstate.colonyTargetedSystemIDs)
    colonyTargetedPlanetIDs = getColonyTargetedPlanetIDs(universe.planetIDs, AIFleetMissionType.FLEET_MISSION_COLONISATION, empireID)
    allColonyTargetedSystemIDs = PlanetUtilsAI.getSystems(colonyTargetedPlanetIDs)

    # export colony targeted systems for other AI modules
    AIstate.colonyTargetedSystemIDs = allColonyTargetedSystemIDs
    print "Colony Targeted PlanetIDs:         " + str(colonyTargetedPlanetIDs)

    colonyFleetIDs = FleetUtilsAI.getEmpireFleetIDsByRole(AIFleetMissionType.FLEET_MISSION_COLONISATION)
    if not colonyFleetIDs:
        print "Available Colony Fleets:             0"
    else:
        print "Colony FleetIDs:                   " + str(FleetUtilsAI.getEmpireFleetIDsByRole(AIFleetMissionType.FLEET_MISSION_COLONISATION))

    numColonyFleets = len(FleetUtilsAI.extractFleetIDsWithoutMissionTypes(colonyFleetIDs))
    print "Colony Fleets Without Missions:      " + str(numColonyFleets)

    print ""
    print "Outpost Targeted SystemIDs:        " + str(AIstate.outpostTargetedSystemIDs)
    outpostTargetedPlanetIDs = getOutpostTargetedPlanetIDs(universe.planetIDs, AIFleetMissionType.FLEET_MISSION_OUTPOST, empireID)
    allOutpostTargetedSystemIDs = PlanetUtilsAI.getSystems(outpostTargetedPlanetIDs)

    # export outpost targeted systems for other AI modules
    AIstate.outpostTargetedSystemIDs = allOutpostTargetedSystemIDs
    print "Outpost Targeted PlanetIDs:        " + str(outpostTargetedPlanetIDs)

    outpostFleetIDs = FleetUtilsAI.getEmpireFleetIDsByRole(AIFleetMissionType.FLEET_MISSION_OUTPOST)
    if not outpostFleetIDs:
        print "Available Outpost Fleets:            0"
    else:
        print "Outpost FleetIDs:                  " + str(FleetUtilsAI.getEmpireFleetIDsByRole(AIFleetMissionType.FLEET_MISSION_OUTPOST))

    numOutpostFleets = len(FleetUtilsAI.extractFleetIDsWithoutMissionTypes(outpostFleetIDs))
    print "Outpost Fleets Without Missions:     " + str(numOutpostFleets)

    evaluatedColonyPlanetIDs = list(set(unpopulatedPlanetIDs) - set(colonyTargetedPlanetIDs))
    # print "Evaluated Colony PlanetIDs:        " + str(evaluatedColonyPlanetIDs)

    evaluatedOutpostPlanetIDs = list(set(unpopulatedPlanetIDs) - set(outpostTargetedPlanetIDs))
    # print "Evaluated Outpost PlanetIDs:       " + str(evaluatedOutpostPlanetIDs)

    evaluatedColonyPlanets = assignColonisationValues(evaluatedColonyPlanetIDs, AIFleetMissionType.FLEET_MISSION_COLONISATION, fleetSupplyablePlanetIDs, species, empire)
    removeLowValuePlanets(evaluatedColonyPlanets)

    sortedPlanets = evaluatedColonyPlanets.items()
    sortedPlanets.sort(lambda x, y: cmp(x[1], y[1]), reverse=True)

    print ""
    print "Settleable Colony PlanetIDs:"
    for evaluationPair in sortedPlanets:
        print "    ID|Score: " + str(evaluationPair)
    print ""

    # export planets for other AI modules
    AIstate.colonisablePlanetIDs = sortedPlanets

    # get outpost fleets
    allOutpostFleetIDs = FleetUtilsAI.getEmpireFleetIDsByRole(AIFleetMissionType.FLEET_MISSION_OUTPOST)
    AIstate.outpostFleetIDs = FleetUtilsAI.extractFleetIDsWithoutMissionTypes(allOutpostFleetIDs)

    evaluatedOutpostPlanets = assignColonisationValues(evaluatedOutpostPlanetIDs, AIFleetMissionType.FLEET_MISSION_OUTPOST, fleetSupplyablePlanetIDs, species, empire)
    removeLowValuePlanets(evaluatedOutpostPlanets)

    sortedOutposts = evaluatedOutpostPlanets.items()
    sortedOutposts.sort(lambda x, y: cmp(x[1], y[1]), reverse=True)

    print "Settleable Outpost PlanetIDs:"
    for evaluationPair in sortedOutposts:
        print "    ID|Score: " + str(evaluationPair)
    print ""

    # export outposts for other AI modules
    AIstate.colonisableOutpostIDs = sortedOutposts

def getColonyTargetedPlanetIDs(planetIDs, missionType, empireID):
    "return list being settled with colony planets"

    universe = fo.getUniverse()
    colonyAIFleetMissions = foAI.foAIstate.getAIFleetMissionsWithAnyMissionTypes([missionType])

    colonyTargetedPlanets = []

    for planetID in planetIDs:
        planet = universe.getPlanet(planetID)
        # add planets that are target of a mission
        for colonyAIFleetMission in colonyAIFleetMissions:
            aiTarget = AITarget.AITarget(AITargetType.TARGET_PLANET, planetID)
            if colonyAIFleetMission.hasTarget(missionType, aiTarget):
                colonyTargetedPlanets.append(planetID)

    return colonyTargetedPlanets

def getOutpostTargetedPlanetIDs(planetIDs, missionType, empireID):
    "return list being settled with outposts planets"

    universe = fo.getUniverse()
    outpostAIFleetMissions = foAI.foAIstate.getAIFleetMissionsWithAnyMissionTypes([missionType])

    outpostTargetedPlanets = []

    for planetID in planetIDs:
        planet = universe.getPlanet(planetID)
        # add planets that are target of a mission
        for outpostAIFleetMission in outpostAIFleetMissions:
            aiTarget = AITarget.AITarget(AITargetType.TARGET_PLANET, planetID)
            if outpostAIFleetMission.hasTarget(missionType, aiTarget):
                outpostTargetedPlanets.append(planetID)

    return outpostTargetedPlanets

def assignColonyFleetsToColonise():
    # assign fleet targets to colonisable planets
    sendColonyShips(AIstate.colonyFleetIDs, AIstate.colonisablePlanetIDs, AIFleetMissionType.FLEET_MISSION_COLONISATION)

    # assign fleet targets to colonisable outposts
    sendColonyShips(AIstate.outpostFleetIDs, AIstate.colonisableOutpostIDs, AIFleetMissionType.FLEET_MISSION_OUTPOST)

def assignColonisationValues(planetIDs, missionType, fleetSupplyablePlanetIDs, species, empire):
    "creates a dictionary that takes planetIDs as key and their colonisation score as value"

    planetValues = {}

    for planetID in planetIDs:
        planetValues[planetID] = evaluatePlanet(planetID, missionType, fleetSupplyablePlanetIDs, species, empire)

    return planetValues

def evaluatePlanet(planetID, missionType, fleetSupplyablePlanetIDs, species, empire):
    "returns the colonisation value of a planet"
    # TODO: in planet evaluation consider specials and distance

    universe = fo.getUniverse()

    planet = universe.getPlanet(planetID)
    if (planet == None): return 0

    # give preference to closest worlds
    empireID = empire.empireID
    capitalID = empire.capitalID
    homeworld = universe.getPlanet(capitalID)
    homeSystemID = homeworld.systemID
    evalSystemID = planet.systemID
    leastJumpsPath = len(universe.leastJumpsPath(homeSystemID, evalSystemID, empireID))
    distanceFactor = 1.001 / (leastJumpsPath + 1)

    # print ">>> evaluatePlanet ID:" + str(planetID) + "/" + str(planet.type) + "/" + str(planet.size) + "/" + str(leastJumpsPath) + "/" + str(distanceFactor)
    if missionType == AIFleetMissionType.FLEET_MISSION_COLONISATION:
        # planet size ranges from 1-5
        if (planetID in fleetSupplyablePlanetIDs):
            return getPlanetHospitality(planetID, species) * planet.size + distanceFactor
        else:
            return getPlanetHospitality(planetID, species) * planet.size - distanceFactor
    elif missionType == AIFleetMissionType.FLEET_MISSION_OUTPOST:
        planetEnvironment = species.getPlanetEnvironment(planet.type)
        if planetEnvironment == fo.planetEnvironment.uninhabitable:
            # prevent outposts from being built when they cannot get food
            if (planetID in fleetSupplyablePlanetIDs):
                return AIstate.minimalColoniseValue + distanceFactor
            elif (str("GRO_ORBIT_FARMING") in empire.availableTechs):
                return AIstate.minimalColoniseValue + distanceFactor
            else:
                return AIstate.minimalColoniseValue - distanceFactor

def getPlanetHospitality(planetID, species):
    "returns a value depending on the planet type"

    universe = fo.getUniverse()

    planet = universe.getPlanet(planetID)
    if planet == None: return 0

    planetEnvironment = species.getPlanetEnvironment(planet.type)
    # print ":: planet:" + str(planetID) + " type:" + str(planet.type) + " size:" + str(planet.size) + " env:" + str(planetEnvironment)

    # reworked with races
    if planetEnvironment == fo.planetEnvironment.good: return 2.75
    if planetEnvironment == fo.planetEnvironment.adequate: return 1
    if planetEnvironment == fo.planetEnvironment.poor: return 0.5
    if planetEnvironment == fo.planetEnvironment.hostile: return 0.25
    if planetEnvironment == fo.planetEnvironment.uninhabitable: return 0.1

    return 0

def removeLowValuePlanets(evaluatedPlanets):
    "removes all planets with a colonisation value < minimalColoniseValue"

    removeIDs = []

    # print ":: min:" + str(AIstate.minimalColoniseValue)
    for planetID in evaluatedPlanets.iterkeys():
        # print ":: eval:" + str(planetID) + " val:" + str(evaluatedPlanets[planetID])
        if (evaluatedPlanets[planetID] < AIstate.minimalColoniseValue):
            removeIDs.append(planetID)

    for ID in removeIDs: del evaluatedPlanets[ID]

def sendColonyShips(colonyFleetIDs, evaluatedPlanets, missionType):
    "sends a list of colony ships to a list of planet_value_pairs"

    i = 0

    for planetID_value_pair in evaluatedPlanets:
        if i >= len(colonyFleetIDs): return

        fleetID = colonyFleetIDs[i]
        planetID = planetID_value_pair[0]

        aiTarget = AITarget.AITarget(AITargetType.TARGET_PLANET, planetID)
        aiFleetMission = foAI.foAIstate.getAIFleetMission(fleetID)
        aiFleetMission.addAITarget(missionType, aiTarget)

        i = i + 1
